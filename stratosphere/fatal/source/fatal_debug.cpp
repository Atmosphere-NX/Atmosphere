/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <map>
#include <switch.h>
#include "fatal_debug.hpp"
#include "fatal_config.hpp"

static bool IsAddressReadable(Handle debug_handle, u64 address, u64 size, MemoryInfo *o_mi) {
    MemoryInfo mi;
    u32 pi;
    
    if (o_mi == NULL) {
        o_mi = &mi;
    }
    
    if (R_FAILED(svcQueryDebugProcessMemory(o_mi, &pi, debug_handle, address))) {
        return false;
    }
    
    /* Must be readable */
    if ((o_mi->perm & Perm_R) != Perm_R) {
        return false;
    }
    
    /* Must have space for both userdata address and userdata size. */
    if (address < o_mi->addr || o_mi->addr + o_mi->size < address + size) {
        return false;
    }

    return true;
}

static bool CheckThreadIsFatalCaller(FatalThrowContext *ctx, u64 debug_handle, u64 thread_id, u64 thread_tls_addr, ThreadContext *thread_ctx) {
    /* Verify that the thread is running or waiting. */
    {
        u64 _;
        u32 thread_state;
        if (R_FAILED(svcGetDebugThreadParam(&_, &thread_state, debug_handle, thread_id, DebugThreadParam_State))) {
            return false;
        }
        
        if (thread_state > 1) {
            return false;
        }
    }
        
    /* Get the thread context. */
    if (R_FAILED(svcGetDebugThreadContext(thread_ctx, debug_handle, thread_id, 0xF))) {
        return false;
    }
    
    /* Check if PC is readable. */
    if (!IsAddressReadable(debug_handle, thread_ctx->pc.x, sizeof(u32), NULL)) {
        return false;
    }
    
    /* Try to read the current instruction. */
    u32 insn;
    if (R_FAILED(svcReadDebugProcessMemory(&insn, debug_handle, thread_ctx->pc.x, sizeof(insn)))) {
        return false;
    }
    
    /* If the instruction isn't svcSendSyncRequest, it's not the fatal caller. */
    if (insn != 0xD4000421) {
        return false;
    }
    
    /* The fatal caller will have readable tls. */
    if (!IsAddressReadable(debug_handle, thread_tls_addr, 0x100, NULL)) {
        return false;
    }
    
    /* Read in the fatal caller's tls. */
    u8 thread_tls[0x100];
    if (R_FAILED(svcReadDebugProcessMemory(thread_tls, debug_handle, thread_tls_addr, sizeof(thread_tls)))) {
        return false;
    }
    
    /* Replace our tls with the fatal caller's. */
    std::memcpy(armGetTls(), thread_tls, sizeof(thread_tls));
    
    /* Parse the command that the thread sent. */
    {
        IpcParsedCommand r;
        if (R_FAILED(ipcParse(&r))) {
            return false;
        }
        
        /* Fatal command takes in a PID, only one buffer max. */
        if (!r.HasPid || r.NumStatics || r.NumStaticsOut || r.NumHandles) {
            return false;
        }
        
        struct {
            u32 magic;
            u32 version;
            u64 cmd_id;
            u32 err_code;
        } *raw = (decltype(raw))(r.Raw);
        
        if (raw->magic != SFCI_MAGIC) {
            return false;
        }
        
        if (raw->cmd_id > 2) {
            return false;
        }
        
        if (raw->cmd_id != 2 && r.NumBuffers) {
            return false;
        }
        
        if (raw->err_code != ctx->error_code) {
            return false;
        }
    }
    
    /* We found our caller. */
    return true;
}

void TryCollectDebugInformation(FatalThrowContext *ctx, u64 pid) {
    Handle debug_handle;
    if (R_SUCCEEDED(svcDebugActiveProcess(&debug_handle, pid))) {
        /* Ensure we close the debugged process. */
        ON_SCOPE_EXIT { svcCloseHandle(debug_handle); };
        
        /* First things first, check if process is 64 bits, and get list of thread infos. */
        std::unordered_map<u64, u64> thread_id_to_tls;
        {
            bool got_attach_process = false;
            DebugEventInfo d;
            while (R_SUCCEEDED(svcGetDebugEvent((u8 *)&d, debug_handle))) {
                if (d.type == DebugEventType::AttachProcess) {
                    ctx->cpu_ctx.is_aarch32 = (d.info.attach_process.flags & 1) == 0;
                    memcpy(ctx->proc_name, d.info.attach_process.name, sizeof(d.info.attach_process.name));
                    got_attach_process = true;
                } else if (d.type == DebugEventType::AttachThread) {
                    thread_id_to_tls[d.info.attach_thread.thread_id] = d.info.attach_thread.tls_address;
                }
            }
            
            if (!got_attach_process) {
                return;
            }
        }
        
        /* TODO: Try to collect information on 32-bit fatals. This shouldn't really matter for any real use case. */
        if (ctx->cpu_ctx.is_aarch32) {
            return;
        }
        
        /* Welcome to hell. */
        bool found_fatal_caller = false;
        u64 thread_id = 0;
        ThreadContext thread_ctx;
        {
            /* We start by trying to get a list of threads. */
            u32 thread_count;
            u64 thread_ids[0x60];
            if (R_FAILED(svcGetThreadList(&thread_count, thread_ids, 0x60, debug_handle))) {
                return;
            }
            
            /* We need to locate the thread that's called fatal. */
            for (u32 i = 0; i < thread_count; i++) {
                const u64 cur_thread_id = thread_ids[i];
                if (thread_id_to_tls.find(cur_thread_id) == thread_id_to_tls.end()) {
                    continue;
                }
                
                if (CheckThreadIsFatalCaller(ctx, debug_handle, cur_thread_id, thread_id_to_tls[cur_thread_id], &thread_ctx)) {
                    thread_id = cur_thread_id;
                    found_fatal_caller = true;
                    break;
                }
            }
            if (!found_fatal_caller) {
                return;
            }
        }
        if (R_FAILED(svcGetDebugThreadContext(&thread_ctx, debug_handle, thread_id, 0xF))) {
            return;
        }
        
        /* So we found our caller. */
        for (u32 i = 0; i < 29; i++) {
            /* GetDebugThreadContext won't give us any of these registers, because thread is in SVC :( */
            ctx->has_gprs[i] = false;
        }
        for (u32 i = 29; i < NumAarch64Gprs; i++) {
            ctx->has_gprs[i] = true;
        }
        ctx->cpu_ctx.aarch64_ctx.fp = thread_ctx.fp;
        ctx->cpu_ctx.aarch64_ctx.lr = thread_ctx.lr;
        ctx->cpu_ctx.aarch64_ctx.sp = thread_ctx.sp;
        ctx->cpu_ctx.aarch64_ctx.pc = thread_ctx.pc.x;
        

        /* Parse a stack trace. */
        u64 cur_fp = thread_ctx.fp;
        for (unsigned int i = 0; i < sizeof(ctx->cpu_ctx.aarch64_ctx.stack_trace)/sizeof(u64); i++) {
            /* Validate the current frame. */
            if (cur_fp == 0 || (cur_fp & 0xF)) {
                break;
            }
            
            /* Read a new frame. */
            StackFrame cur_frame;
            if (R_FAILED(svcReadDebugProcessMemory(&cur_frame, debug_handle, cur_fp, sizeof(StackFrame)))) {
                break;
            }
            
            /* Advance to the next frame. */
            ctx->cpu_ctx.aarch64_ctx.stack_trace[ctx->cpu_ctx.aarch64_ctx.stack_trace_size++] = cur_frame.lr;
            cur_fp = cur_frame.fp;
        }
        
        /* Try to read up to 0x100 of stack. */
        for (size_t sz = 0x100; sz > 0; sz -= 0x10) {
            if (IsAddressReadable(debug_handle, ctx->cpu_ctx.aarch64_ctx.sp, sz, nullptr)) {
                if (R_SUCCEEDED(svcReadDebugProcessMemory(ctx->stack_dump, debug_handle, ctx->cpu_ctx.aarch64_ctx.sp, sz))) {
                    ctx->stack_dump_size = sz;
                }
                break;
            }
        }
        
        /* Parse the starting address. */
        {
            u64 guess = thread_ctx.pc.x;
            MemoryInfo mi;
            u32 pi;
            if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, guess)) || mi.perm != Perm_Rx) {
                return;
            }
            
            /* Iterate backwards until we find the memory before the code region. */
            while (mi.addr > 0) {
                if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, guess))) {
                    return;
                }
                
                if (mi.type == MemType_Unmapped) {
                    /* Code region will be at the end of the unmapped region preceding it. */
                    ctx->cpu_ctx.aarch64_ctx.start_address = mi.addr + mi.size;
                    break;
                }
                
                guess -= 4;
            }
        }
    }
}