/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stratosphere.hpp>
#include "ams_bpc.h"

namespace ams {

    namespace {

        inline u64 GetPc() {
            u64 pc;
            __asm__ __volatile__ ("adr %[pc], ." : [pc]"=&r"(pc) :: );
            return pc;
        }

        struct StackFrame {
            u64 fp;
            u64 lr;
        };

    }

    void InitializeForBoot() {
        R_ABORT_UNLESS(amsBpcInitialize());
    }

    void SetInitialRebootPayload(const void *src, size_t src_size) {
        R_ABORT_UNLESS(amsBpcSetRebootPayload(src, src_size));
    }

    void WEAK_SYMBOL ExceptionHandler(FatalErrorContext *ctx) {
        R_ABORT_UNLESS(amsBpcInitialize());
        R_ABORT_UNLESS(amsBpcRebootToFatalError(ctx));
        while (1) { /* ... */ }
    }

    void CrashHandler(ThreadExceptionDump *ctx) {
        FatalErrorContext ams_ctx;

        /* Convert thread dump to atmosphere dump. */
        {
            ams_ctx.magic = FatalErrorContext::Magic;
            ams_ctx.error_desc = ctx->error_desc;
            ams_ctx.program_id = os::GetCurrentProgramId().value;
            for (size_t i = 0; i < FatalErrorContext::NumGprs; i++) {
                ams_ctx.gprs[i] = ctx->cpu_gprs[i].x;
            }
            if (ams_ctx.error_desc == FatalErrorContext::DataAbortErrorDesc &&
                ams_ctx.gprs[27] == FatalErrorContext::StdAbortMagicAddress &&
                ams_ctx.gprs[28] == FatalErrorContext::StdAbortMagicValue)
            {
                /* Detect std::abort(). */
                ams_ctx.error_desc = FatalErrorContext::StdAbortErrorDesc;
            }

            ams_ctx.fp = ctx->fp.x;
            ams_ctx.lr = ctx->lr.x;
            ams_ctx.sp = ctx->sp.x;
            ams_ctx.pc = ctx->pc.x;
            ams_ctx.pstate = ctx->pstate;
            ams_ctx.afsr0 = static_cast<u32>(::ams::exosphere::GetVersion(ATMOSPHERE_RELEASE_VERSION));
            ams_ctx.afsr1 = static_cast<u32>(hos::GetVersion());
            ams_ctx.far = ctx->far.x;
            ams_ctx.report_identifier = armGetSystemTick();

            /* Detect stack overflow. */
            if (ams_ctx.error_desc == FatalErrorContext::DataAbortErrorDesc) {
                svc::lp64::MemoryInfo mem_info;
                svc::PageInfo page_info;

                if (/* Check if stack pointer is in guard page. */
                    R_SUCCEEDED(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), ams_ctx.sp)) &&
                    mem_info.state == svc::MemoryState_Free &&
                    /* Check if stack pointer fell off stack. */
                    R_SUCCEEDED(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), ams_ctx.sp + 0x1000)) &&
                    mem_info.state == svc::MemoryState_Stack) {
                    ams_ctx.error_desc = FatalErrorContext::StackOverflowErrorDesc;
                }
            }
            /* Grab module base. */
            {
                svc::lp64::MemoryInfo mem_info;
                svc::PageInfo page_info;
                if (R_SUCCEEDED(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), GetPc()))) {
                    ams_ctx.module_base = mem_info.base_address;
                } else {
                    ams_ctx.module_base = 0;
                }
            }
            ams_ctx.stack_trace_size = 0;
            u64 cur_fp = ams_ctx.fp;
            for (size_t i = 0; i < FatalErrorContext::MaxStackTrace; i++) {
                /* Validate current frame. */
                if (cur_fp == 0 || (cur_fp & 0xF)) {
                    break;
                }

                /* Read a new frame. */
                StackFrame cur_frame;
                svc::lp64::MemoryInfo mem_info;
                svc::PageInfo page_info;
                if (R_SUCCEEDED(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), cur_fp)) && (mem_info.permission & svc::MemoryPermission_Read) == svc::MemoryPermission_Read) {
                    std::memcpy(std::addressof(cur_frame), reinterpret_cast<void *>(cur_fp), sizeof(cur_frame));
                } else {
                    break;
                }

                /* Advance to the next frame. */
                ams_ctx.stack_trace[ams_ctx.stack_trace_size++] = cur_frame.lr;
                cur_fp = cur_frame.fp;
            }
            /* Clear unused parts of stack trace. */
            for (size_t i = ams_ctx.stack_trace_size; i < FatalErrorContext::MaxStackTrace; i++) {
                ams_ctx.stack_trace[i] = 0;
            }

            /* Grab up to 0x100 of stack. */
            {
                svc::lp64::MemoryInfo mem_info;
                svc::PageInfo page_info;
                if (R_SUCCEEDED(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), ams_ctx.sp)) && (mem_info.permission & svc::MemoryPermission_Read) == svc::MemoryPermission_Read) {
                    size_t copy_size = std::min(FatalErrorContext::MaxStackDumpSize, static_cast<size_t>(mem_info.base_address + mem_info.size - ams_ctx.sp));
                    ams_ctx.stack_dump_size = copy_size;
                    std::memcpy(ams_ctx.stack_dump, reinterpret_cast<void *>(ams_ctx.sp), copy_size);
                } else {
                    ams_ctx.stack_dump_size = 0;
                }
            }

            /* Grab 0x100 of tls. */
            std::memcpy(ams_ctx.tls, svc::GetThreadLocalRegion(), sizeof(ams_ctx.tls));
        }

        /* Just call the user exception handler. */
        ::ams::ExceptionHandler(std::addressof(ams_ctx));
    }

    NORETURN void AbortImpl();

}

extern "C" {

    /* Redefine C++ exception handlers. Requires wrap linker flag. */
    #define WRAP_ABORT_FUNC(func) void NORETURN __wrap_##func(void) { ::ams::AbortImpl(); __builtin_unreachable(); }
    WRAP_ABORT_FUNC(__cxa_throw)
    WRAP_ABORT_FUNC(__cxa_rethrow)
    WRAP_ABORT_FUNC(__cxa_allocate_exception)
    WRAP_ABORT_FUNC(__cxa_free_exception)
    WRAP_ABORT_FUNC(__cxa_begin_catch)
    WRAP_ABORT_FUNC(__cxa_end_catch)
    WRAP_ABORT_FUNC(__cxa_call_unexpected)
    WRAP_ABORT_FUNC(__cxa_call_terminate)
    WRAP_ABORT_FUNC(__gxx_personality_v0)
    WRAP_ABORT_FUNC(_ZSt19__throw_logic_errorPKc)
    WRAP_ABORT_FUNC(_ZSt20__throw_length_errorPKc)
    WRAP_ABORT_FUNC(_ZNSt11logic_errorC2EPKc)

    /* TODO: We may wish to consider intentionally not defining an _Unwind_Resume wrapper. */
    /* This would mean that a failure to wrap all exception functions is a linker error. */
    WRAP_ABORT_FUNC(_Unwind_Resume)
    #undef WRAP_ABORT_FUNC

}
