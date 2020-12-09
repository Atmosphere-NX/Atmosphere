/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "fatal_debug.hpp"
#include "fatal_config.hpp"

namespace ams::fatal::srv {

    namespace {

        constexpr u32 SvcSendSyncRequestInstruction = 0xD4000421;

        struct StackFrame {
            u64 fp;
            u64 lr;
        };

        bool IsThreadFatalCaller(Result result, u32 debug_handle, u64 thread_id, u64 thread_tls_addr, ThreadContext *thread_ctx) {
            /* Verify that the thread is running or waiting. */
            {
                u64 _;
                u32 _thread_state;
                if (R_FAILED(svcGetDebugThreadParam(&_, &_thread_state, debug_handle, thread_id, DebugThreadParam_State))) {
                    return false;
                }

                const svc::ThreadState thread_state = static_cast<svc::ThreadState>(_thread_state);
                if (thread_state != svc::ThreadState_Waiting && thread_state != svc::ThreadState_Running) {
                    return false;
                }
            }

            /* Get the thread context. */
            if (R_FAILED(svcGetDebugThreadContext(thread_ctx, debug_handle, thread_id, svc::ThreadContextFlag_All))) {
                return false;
            }

            /* Try to read the current instruction. */
            u32 insn;
            if (R_FAILED(svcReadDebugProcessMemory(&insn, debug_handle, thread_ctx->pc.x, sizeof(insn)))) {
                return false;
            }

            /* If the instruction isn't svcSendSyncRequest, it's not the fatal caller. */
            if (insn != SvcSendSyncRequestInstruction) {
                return false;
            }

            /* Read in the fatal caller's TLS. */
            u8 thread_tls[0x100];
            if (R_FAILED(svcReadDebugProcessMemory(thread_tls, debug_handle, thread_tls_addr, sizeof(thread_tls)))) {
                return false;
            }

            /* We want to parse the command the fatal caller sent. */
            {
                const auto request = hipcParseRequest(thread_tls);

                const struct {
                    CmifInHeader header;
                    Result result;
                } *in_data = decltype(in_data)(request.data.data_words);
                static_assert(sizeof(*in_data) == 0x14, "InData!");

                /* Fatal command takes in a PID, only one buffer max. */
                if ((request.meta.type != CmifCommandType_Request && request.meta.type != CmifCommandType_RequestWithContext) ||
                    !request.meta.send_pid ||
                    request.meta.num_send_statics ||
                    request.meta.num_recv_statics ||
                    request.meta.num_recv_buffers ||
                    request.meta.num_exch_buffers ||
                    request.meta.num_copy_handles ||
                    request.meta.num_move_handles ||
                    request.meta.num_data_words < ((sizeof(*in_data) + 0x10) / sizeof(u32)))
                {
                    return false;
                }

                if (in_data->header.magic != CMIF_IN_HEADER_MAGIC) {
                    return false;
                }

                if (in_data->header.version > 1) {
                    return false;
                }

                switch (in_data->header.command_id) {
                    case 0:
                    case 1:
                        if (request.meta.num_send_buffers != 0) {
                            return false;
                        }
                        break;
                    case 2:
                        if (request.meta.num_send_buffers != 1) {
                            return false;
                        }
                    default:
                        return false;
                }

                if (in_data->result.GetValue() != result.GetValue()) {
                    return false;
                }
            }

            /* We found our caller. */
            return true;
        }

        bool TryGuessBaseAddress(u64 *out_base_address, u32 debug_handle, u64 guess) {
            MemoryInfo mi;
            u32 pi;
            if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, guess)) || mi.perm != Perm_Rx) {
                return false;
            }

            /* Iterate backwards until we find the memory before the code region. */
            while (mi.addr > 0) {
                if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, guess))) {
                    return false;
                }

                if (mi.type == MemType_Unmapped) {
                    /* Code region will be at the end of the unmapped region preceding it. */
                    *out_base_address = mi.addr + mi.size;
                    return true;
                }

                guess = mi.addr - 4;
            }

            return false;
        }

        u64 GetBaseAddress(const ThrowContext *throw_ctx, const ThreadContext *thread_ctx, u32 debug_handle) {
            u64 base_address = 0;

            if (TryGuessBaseAddress(&base_address, debug_handle, thread_ctx->pc.x)) {
                return base_address;
            }

            if (TryGuessBaseAddress(&base_address, debug_handle, thread_ctx->lr)) {
                return base_address;
            }

            for (size_t i = 0; i < throw_ctx->cpu_ctx.aarch64_ctx.stack_trace_size; i++) {
                if (TryGuessBaseAddress(&base_address, debug_handle, throw_ctx->cpu_ctx.aarch64_ctx.stack_trace[i])) {
                    return base_address;
                }
            }

            return base_address;
        }

    }

    void TryCollectDebugInformation(ThrowContext *ctx, os::ProcessId process_id) {
        /* Try to debug the process. This may fail, if we called into ourself. */
        Handle debug_handle;
        if (R_FAILED(svcDebugActiveProcess(std::addressof(debug_handle), static_cast<u64>(process_id)))) {
            return;
        }
        ON_SCOPE_EXIT { R_ABORT_UNLESS(svc::CloseHandle(debug_handle)); };

        /* First things first, check if process is 64 bits, and get list of thread infos. */
        std::unordered_map<u64, u64> thread_id_to_tls;
        {
            bool got_create_process = false;
            svc::DebugEventInfo d;
            while (R_SUCCEEDED(svcGetDebugEvent(reinterpret_cast<u8 *>(&d), debug_handle))) {
                switch (d.type) {
                    case svc::DebugEvent_CreateProcess:
                        ctx->cpu_ctx.architecture = (d.info.create_process.flags & 1) ? CpuContext::Architecture_Aarch64 : CpuContext::Architecture_Aarch32;
                        std::memcpy(ctx->proc_name, d.info.create_process.name, sizeof(d.info.create_process.name));
                        got_create_process = true;
                        break;
                    case svc::DebugEvent_CreateThread:
                        thread_id_to_tls[d.info.create_thread.thread_id] = d.info.create_thread.tls_address;
                        break;
                    case svc::DebugEvent_Exception:
                    case svc::DebugEvent_ExitProcess:
                    case svc::DebugEvent_ExitThread:
                        break;
                }
            }

            if (!got_create_process) {
                return;
            }
        }

        /* TODO: Try to collect information on 32-bit fatals. This shouldn't really matter for any real use case. */
        if (ctx->cpu_ctx.architecture == CpuContext::Architecture_Aarch32) {
            return;
        }

        /* Welcome to hell. Here, we try to identify which thread called into fatal. */
        bool found_fatal_caller = false;
        u64 thread_id = 0;
        u64 thread_tls = 0;
        ThreadContext thread_ctx;
        {
            /* We start by trying to get a list of threads. */
            s32 thread_count;
            u64 thread_ids[0x60];
            if (R_FAILED(svc::GetThreadList(&thread_count, thread_ids, 0x60, debug_handle))) {
                return;
            }

            /* We need to locate the thread that's called fatal. */
            for (s32 i = 0; i < thread_count; i++) {
                const u64 cur_thread_id = thread_ids[i];
                if (thread_id_to_tls.find(cur_thread_id) == thread_id_to_tls.end()) {
                    continue;
                }

                if (IsThreadFatalCaller(ctx->result, debug_handle, cur_thread_id, thread_id_to_tls[cur_thread_id], &thread_ctx)) {
                    thread_id = cur_thread_id;
                    thread_tls = thread_id_to_tls[thread_id];
                    found_fatal_caller = true;
                    break;
                }
            }
            if (!found_fatal_caller) {
                return;
            }
        }
        if (R_FAILED(svcGetDebugThreadContext(&thread_ctx, debug_handle, thread_id, svc::ThreadContextFlag_All))) {
            return;
        }

        /* Set register states. */
        ctx->cpu_ctx.aarch64_ctx.SetRegisterValue(aarch64::RegisterName_FP, thread_ctx.fp);
        ctx->cpu_ctx.aarch64_ctx.SetRegisterValue(aarch64::RegisterName_LR, thread_ctx.lr);
        ctx->cpu_ctx.aarch64_ctx.SetRegisterValue(aarch64::RegisterName_SP, thread_ctx.sp);
        ctx->cpu_ctx.aarch64_ctx.SetRegisterValue(aarch64::RegisterName_PC, thread_ctx.pc.x);

        /* Parse a stack trace. */
        u64 cur_fp = thread_ctx.fp;
        ctx->cpu_ctx.aarch64_ctx.stack_trace_size = 0;
        for (unsigned int i = 0; i < aarch64::CpuContext::MaxStackTraceDepth; i++) {
            /* Validate the current frame. */
            if (cur_fp == 0 || (cur_fp & 0xF)) {
                break;
            }

            /* Read a new frame. */
            StackFrame cur_frame = {};
            if (R_FAILED(svcReadDebugProcessMemory(&cur_frame, debug_handle, cur_fp, sizeof(StackFrame)))) {
                break;
            }

            /* Advance to the next frame. */
            ctx->cpu_ctx.aarch64_ctx.stack_trace[ctx->cpu_ctx.aarch64_ctx.stack_trace_size++] = cur_frame.lr;
            cur_fp = cur_frame.fp;
        }

        /* Try to read up to 0x100 of stack. */
        ctx->stack_dump_base = 0;
        for (size_t sz = 0x100; sz > 0; sz -= 0x10) {
            if (R_SUCCEEDED(svcReadDebugProcessMemory(ctx->stack_dump, debug_handle, thread_ctx.sp, sz))) {
                ctx->stack_dump_base = thread_ctx.sp;
                ctx->stack_dump_size = sz;
                break;
            }
        }

        /* Try to read the first 0x100 of TLS. */
        if (R_SUCCEEDED(svcReadDebugProcessMemory(ctx->tls_dump, debug_handle, thread_tls, sizeof(ctx->tls_dump)))) {
            ctx->tls_address = thread_tls;
        } else {
            ctx->tls_address = 0;
            std::memset(ctx->tls_dump, 0xCC, sizeof(ctx->tls_dump));
        }

        /* Parse the base address. */
        ctx->cpu_ctx.aarch64_ctx.SetBaseAddress(GetBaseAddress(ctx, &thread_ctx, debug_handle));
    }

}
