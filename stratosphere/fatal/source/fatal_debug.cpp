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
#include "fatal_debug.hpp"
#include "fatal_config.hpp"

namespace ams::fatal::srv {

    namespace {

        constexpr u32 SvcSendSyncRequestInstruction = 0xD4000421;

        struct StackFrame {
            u64 fp;
            u64 lr;
        };

        constexpr inline size_t MaxThreads = 0x60;

        template<size_t MaxThreadCount>
        class ThreadTlsMapImpl {
            private:
                std::pair<u64, u64> m_map[MaxThreadCount];
                size_t m_index;
            public:
                constexpr ThreadTlsMapImpl() : m_map(), m_index(0) { /* ... */ }

                constexpr void ResetThreadTlsMap() {
                    m_index = 0;
                }

                constexpr void SetThreadTls(u64 thread_id, u64 tls) {
                    if (m_index < util::size(m_map)) {
                        m_map[m_index++] = std::make_pair(thread_id, tls);
                    }
                }

                constexpr bool GetThreadTls(u64 *out, u64 thread_id) const {
                    for (size_t i = 0; i < m_index; ++i) {
                        if (m_map[i].first == thread_id) {
                            *out = m_map[i].second;
                            return true;
                        }
                    }
                    return false;
                }
        };

        using ThreadTlsMap = ThreadTlsMapImpl<MaxThreads>;

        constinit ThreadTlsMap g_thread_id_to_tls_map;

        bool IsThreadFatalCaller(Result result, os::NativeHandle debug_handle, u64 thread_id, u64 thread_tls_addr, svc::ThreadContext *thread_ctx) {
            /* Verify that the thread is running or waiting. */
            {
                u64 _;
                u32 _thread_state;
                if (R_FAILED(svc::GetDebugThreadParam(&_, &_thread_state, debug_handle, thread_id, svc::DebugThreadParam_State))) {
                    return false;
                }

                const auto thread_state = static_cast<svc::ThreadState>(_thread_state);
                if (thread_state != svc::ThreadState_Waiting && thread_state != svc::ThreadState_Running) {
                    return false;
                }
            }

            /* Get the thread context. */
            if (R_FAILED(svc::GetDebugThreadContext(thread_ctx, debug_handle, thread_id, svc::ThreadContextFlag_All))) {
                return false;
            }

            /* Try to read the current instruction. */
            u32 insn;
            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(insn)), debug_handle, thread_ctx->pc, sizeof(insn)))) {
                return false;
            }

            /* If the instruction isn't svc::SendSyncRequest, it's not the fatal caller. */
            if (insn != SvcSendSyncRequestInstruction) {
                return false;
            }

            /* Read in the fatal caller's TLS. */
            u8 thread_tls[sizeof(svc::ThreadLocalRegion::message_buffer)];
            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(thread_tls), debug_handle, thread_tls_addr, sizeof(thread_tls)))) {
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
                        break;
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

        bool TryGuessBaseAddress(u64 *out_base_address, os::NativeHandle debug_handle, u64 guess) {
            svc::MemoryInfo mi;
            svc::PageInfo pi;
            if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), debug_handle, guess)) || mi.permission != svc::MemoryPermission_ReadExecute) {
                return false;
            }

            /* Iterate backwards until we find the memory before the code region. */
            while (mi.base_address > 0) {
                if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), debug_handle, guess))) {
                    return false;
                }

                if (mi.state == svc::MemoryState_Free) {
                    /* Code region will be at the end of the unmapped region preceding it. */
                    *out_base_address = mi.base_address + mi.size;
                    return true;
                }

                guess = mi.base_address - 4;
            }

            return false;
        }

        u64 GetBaseAddress(const ThrowContext *throw_ctx, const svc::ThreadContext *thread_ctx, os::NativeHandle debug_handle) {
            u64 base_address = 0;

            if (TryGuessBaseAddress(std::addressof(base_address), debug_handle, thread_ctx->pc)) {
                return base_address;
            }

            if (TryGuessBaseAddress(std::addressof(base_address), debug_handle, thread_ctx->lr)) {
                return base_address;
            }

            for (size_t i = 0; i < throw_ctx->cpu_ctx.aarch64_ctx.stack_trace_size; i++) {
                if (TryGuessBaseAddress(std::addressof(base_address), debug_handle, throw_ctx->cpu_ctx.aarch64_ctx.stack_trace[i])) {
                    return base_address;
                }
            }

            return base_address;
        }

    }

    void TryCollectDebugInformation(ThrowContext *ctx, os::ProcessId process_id) {
        /* Try to debug the process. This may fail, if we called into ourself. */
        os::NativeHandle debug_handle;
        if (R_FAILED(svc::DebugActiveProcess(std::addressof(debug_handle), process_id.value))) {
            return;
        }
        ON_SCOPE_EXIT { os::CloseNativeHandle(debug_handle); };

        /* First things first, check if process is 64 bits, and get list of thread infos. */
        g_thread_id_to_tls_map.ResetThreadTlsMap();
        {
            bool got_create_process = false;
            svc::DebugEventInfo d;
            while (R_SUCCEEDED(svc::GetDebugEvent(std::addressof(d), debug_handle))) {
                switch (d.type) {
                    case svc::DebugEvent_CreateProcess:
                        ctx->cpu_ctx.architecture = (d.info.create_process.flags & 1) ? CpuContext::Architecture_Aarch64 : CpuContext::Architecture_Aarch32;
                        std::memcpy(ctx->proc_name, d.info.create_process.name, sizeof(d.info.create_process.name));
                        got_create_process = true;
                        break;
                    case svc::DebugEvent_CreateThread:
                        g_thread_id_to_tls_map.SetThreadTls(d.info.create_thread.thread_id, d.info.create_thread.tls_address);
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
        svc::ThreadContext thread_ctx;
        {
            /* We start by trying to get a list of threads. */
            s32 thread_count;
            u64 thread_ids[0x60];
            if (R_FAILED(svc::GetThreadList(std::addressof(thread_count), thread_ids, 0x60, debug_handle))) {
                return;
            }

            /* We need to locate the thread that's called fatal. */
            for (s32 i = 0; i < thread_count; i++) {
                const u64 cur_thread_id = thread_ids[i];
                u64 cur_thread_tls;
                if (!g_thread_id_to_tls_map.GetThreadTls(std::addressof(cur_thread_tls), cur_thread_id)) {
                    continue;
                }

                if (IsThreadFatalCaller(ctx->result, debug_handle, cur_thread_id, cur_thread_tls, std::addressof(thread_ctx))) {
                    thread_id  = cur_thread_id;
                    thread_tls = cur_thread_tls;
                    found_fatal_caller = true;
                    break;
                }
            }
            if (!found_fatal_caller) {
                return;
            }
        }
        if (R_FAILED(svc::GetDebugThreadContext(std::addressof(thread_ctx), debug_handle, thread_id, svc::ThreadContextFlag_All))) {
            return;
        }

        /* Set register states. */
        ctx->cpu_ctx.aarch64_ctx.SetRegisterValue(aarch64::RegisterName_FP, thread_ctx.fp);
        ctx->cpu_ctx.aarch64_ctx.SetRegisterValue(aarch64::RegisterName_LR, thread_ctx.lr);
        ctx->cpu_ctx.aarch64_ctx.SetRegisterValue(aarch64::RegisterName_SP, thread_ctx.sp);
        ctx->cpu_ctx.aarch64_ctx.SetRegisterValue(aarch64::RegisterName_PC, thread_ctx.pc);

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
            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(cur_frame)), debug_handle, cur_fp, sizeof(StackFrame)))) {
                break;
            }

            /* Advance to the next frame. */
            ctx->cpu_ctx.aarch64_ctx.stack_trace[ctx->cpu_ctx.aarch64_ctx.stack_trace_size++] = cur_frame.lr;
            cur_fp = cur_frame.fp;
        }

        /* Try to read up to 0x100 of stack. */
        ctx->stack_dump_base = 0;
        for (size_t sz = 0x100; sz > 0; sz -= 0x10) {
            if (R_SUCCEEDED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(ctx->stack_dump), debug_handle, thread_ctx.sp, sz))) {
                ctx->stack_dump_base = thread_ctx.sp;
                ctx->stack_dump_size = sz;
                break;
            }
        }

        /* Try to read the first 0x100 of TLS. */
        if (R_SUCCEEDED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(ctx->tls_dump), debug_handle, thread_tls, sizeof(ctx->tls_dump)))) {
            ctx->tls_address = thread_tls;
        } else {
            ctx->tls_address = 0;
            std::memset(ctx->tls_dump, 0xCC, sizeof(ctx->tls_dump));
        }

        /* Parse the base address. */
        ctx->cpu_ctx.aarch64_ctx.SetBaseAddress(GetBaseAddress(ctx, std::addressof(thread_ctx), debug_handle));
    }

}
