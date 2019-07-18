/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <mutex>
#include <switch.h>
#include <stratosphere.hpp>

WEAK sts::ncm::TitleId __stratosphere_title_id = sts::ncm::TitleId::Invalid;

extern "C" {
    void WEAK __libstratosphere_exception_handler(AtmosphereFatalErrorContext *ctx);

    /* Redefine abort, so that it triggers these handlers. */
    void abort();
};

static inline u64 GetPc() {
    u64 pc;
    __asm__ __volatile__ ("adr %[pc], ." : [pc]"=&r"(pc) :: );
    return pc;
}

struct StackFrame {
    u64 fp;
    u64 lr;
};

void StratosphereCrashHandler(ThreadExceptionDump *ctx) {
    AtmosphereFatalErrorContext ams_ctx;
    /* Convert thread dump to atmosphere dump. */
    {
        ams_ctx.magic = AtmosphereFatalErrorMagic;
        ams_ctx.error_desc = ctx->error_desc;
        ams_ctx.title_id = static_cast<u64>(__stratosphere_title_id);
        for (size_t i = 0; i < AtmosphereFatalErrorNumGprs; i++) {
            ams_ctx.gprs[i] = ctx->cpu_gprs[i].x;
        }
        if (ams_ctx.error_desc == DATA_ABORT_ERROR_DESC &&
            ams_ctx.gprs[2] == STD_ABORT_ADDR_MAGIC &&
            ams_ctx.gprs[3] == STD_ABORT_VALUE_MAGIC) {
            /* Detect std::abort(). */
            ams_ctx.error_desc = STD_ABORT_ERROR_DESC;
        }

        ams_ctx.fp = ctx->fp.x;
        ams_ctx.lr = ctx->lr.x;
        ams_ctx.sp = ctx->sp.x;
        ams_ctx.pc = ctx->pc.x;
        ams_ctx.pstate = ctx->pstate;
        ams_ctx.afsr0 = ctx->afsr0;
        ams_ctx.afsr1 = ctx->afsr1;
        ams_ctx.far = ctx->far.x;
        ams_ctx.report_identifier = armGetSystemTick();
        /* Grab module base. */
        {
            MemoryInfo mem_info;
            u32 page_info;
            if (R_SUCCEEDED(svcQueryMemory(&mem_info, &page_info, GetPc()))) {
                ams_ctx.module_base = mem_info.addr;
            } else {
                ams_ctx.module_base = 0;
            }
        }
        ams_ctx.stack_trace_size = 0;
        u64 cur_fp = ams_ctx.fp;
        for (size_t i = 0; i < AMS_FATAL_ERROR_MAX_STACKTRACE; i++) {
            /* Validate current frame. */
            if (cur_fp == 0 || (cur_fp & 0xF)) {
                break;
            }

            /* Read a new frame. */
            StackFrame cur_frame;
            MemoryInfo mem_info;
            u32 page_info;
            if (R_SUCCEEDED(svcQueryMemory(&mem_info, &page_info, cur_fp)) && (mem_info.perm & Perm_R) == Perm_R) {
                std::memcpy(&cur_frame, reinterpret_cast<void *>(cur_fp), sizeof(cur_frame));
            } else {
                break;
            }

            /* Advance to the next frame. */
            ams_ctx.stack_trace[ams_ctx.stack_trace_size++] = cur_frame.lr;
            cur_fp = cur_frame.fp;
        }
        /* Clear unused parts of stack trace. */
        for (size_t i = ams_ctx.stack_trace_size; i < AMS_FATAL_ERROR_MAX_STACKTRACE; i++) {
            ams_ctx.stack_trace[i] = 0;
        }

        /* Grab up to 0x100 of stack. */
        {
            MemoryInfo mem_info;
            u32 page_info;
            if (R_SUCCEEDED(svcQueryMemory(&mem_info, &page_info, ams_ctx.sp)) && (mem_info.perm & Perm_R) == Perm_R) {
                size_t copy_size = std::min(static_cast<size_t>(AMS_FATAL_ERROR_MAX_STACKDUMP), static_cast<size_t>(mem_info.addr + mem_info.size - ams_ctx.sp));
                ams_ctx.stack_dump_size = copy_size;
                std::memcpy(ams_ctx.stack_dump, reinterpret_cast<void *>(ams_ctx.sp), copy_size);
            } else {
                ams_ctx.stack_dump_size = 0;
            }
        }
    }

    /* Just call the user exception handler. */
    __libstratosphere_exception_handler(&ams_ctx);
}

/* Default exception handler behavior. */
void WEAK __libstratosphere_exception_handler(AtmosphereFatalErrorContext *ctx) {
    R_ASSERT(bpcAmsInitialize());
    R_ASSERT(bpcAmsRebootToFatalError(ctx));
    bpcAmsExit();
    while (1) { }
}

/* Custom abort handler, so that std::abort will trigger these. */
void abort() {
    /* Just perform a data abort. */
    register u64 addr __asm__("x2") = STD_ABORT_ADDR_MAGIC;
    register u64 val __asm__("x3") = STD_ABORT_VALUE_MAGIC;
    while (true) {
        __asm__ __volatile__ (
            "str %[val], [%[addr]]"
            :
            : [val]"r"(val), [addr]"r"(addr)
        );
    }
}