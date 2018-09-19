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
 
#include <switch.h>
#include <cstring>

#include "creport_thread_info.hpp"
#include "creport_crash_report.hpp"

void ThreadInfo::SaveToFile(FILE *f_report) {
    fprintf(f_report, "    Thread ID:                   %016lx\n", this->thread_id);
    if (stack_top) {  
        fprintf(f_report, "    Stack:                       %016lx-%016lx\n", this->stack_bottom, this->stack_top);
    }
    fprintf(f_report, "    Registers:\n");
    {
        for (unsigned int i = 0; i <= 28; i++) {  
            fprintf(f_report, "        X[%02u]:                   %s\n", i, this->code_list->GetFormattedAddressString(this->context.cpu_gprs[i].x));
        }
        fprintf(f_report, "        FP:                      %s\n", this->code_list->GetFormattedAddressString(this->context.fp));
        fprintf(f_report, "        LR:                      %s\n", this->code_list->GetFormattedAddressString(this->context.lr));
        fprintf(f_report, "        SP:                      %s\n", this->code_list->GetFormattedAddressString(this->context.sp));
        fprintf(f_report, "        PC:                      %s\n", this->code_list->GetFormattedAddressString(this->context.pc.x));
    }
    fprintf(f_report, "    Stack Trace:\n");
    for (unsigned int i = 0; i < this->stack_trace_size; i++) {
        fprintf(f_report, "        ReturnAddress[%02u]:       %s\n", i, this->code_list->GetFormattedAddressString(this->stack_trace[i]));
    }
}

bool ThreadInfo::ReadFromProcess(Handle debug_handle, u64 thread_id, bool is_64_bit) {
    this->thread_id = thread_id;
    
    /* Verify that the thread is running or waiting. */
    {
        u64 _;
        u32 thread_state;
        if (R_FAILED(svcGetDebugThreadParam(&_, &thread_state, debug_handle, this->thread_id, DebugThreadParam_State))) {
            return false;
        }
        
        if (thread_state > 1) {
            return false;
        }
    }
    
    /* Get the thread context. */
    if (R_FAILED(svcGetDebugThreadContext(&this->context, debug_handle, this->thread_id, 0xF))) {
        return false;
    }
    
    /* Don't try to parse stack frames if 32-bit. */
    if (!is_64_bit) {
        return true;
    }
    
    /* Try to locate stack top/bottom. */
    TryGetStackInfo(debug_handle);
    
    u64 cur_fp = this->context.fp;
    for (unsigned int i = 0; i < sizeof(this->stack_trace)/sizeof(u64); i++) {
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
        this->stack_trace[this->stack_trace_size++] = cur_frame.lr;
        cur_fp = cur_frame.fp;
    }
    
    return true;
}

void ThreadInfo::TryGetStackInfo(Handle debug_handle) {
    MemoryInfo mi;
    u32 pi;
    if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, this->context.sp))) {
        return;
    }
    
    /* Check if sp points into the stack. */
    if (mi.type == MemType_MappedMemory) {
        this->stack_bottom = mi.addr;
        this->stack_top = mi.addr + mi.size;
        return;
    }
    
    /* It's possible that sp is below the stack... */
    if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, mi.addr + mi.size))) {
        return;
    }
    
    if (mi.type == MemType_MappedMemory) {
        this->stack_bottom = mi.addr;
        this->stack_top = mi.addr + mi.size;
    }
}

void ThreadInfo::DumpBinary(FILE *f_bin) {
    fwrite(&this->thread_id, sizeof(this->thread_id), 1, f_bin);
    fwrite(&this->context, sizeof(this->context), 1, f_bin);
    
    u64 sts = this->stack_trace_size;
    fwrite(&sts, sizeof(sts), 1, f_bin);
    fwrite(this->stack_trace, sizeof(u64), this->stack_trace_size, f_bin);
    fwrite(&this->stack_bottom, sizeof(this->stack_bottom), 1, f_bin);
    fwrite(&this->stack_top, sizeof(this->stack_top), 1, f_bin);
}

void ThreadList::DumpBinary(FILE *f_bin, u64 crashed_id) {
    u32 magic = 0x30495444; /* 'DTI0' */
    fwrite(&magic, sizeof(magic), 1, f_bin);
    fwrite(&this->thread_count, sizeof(u32), 1, f_bin);
    fwrite(&crashed_id, sizeof(crashed_id), 1, f_bin);
    for (unsigned int i = 0; i < this->thread_count; i++) {
        this->thread_infos[i].DumpBinary(f_bin);
    }
}

void ThreadList::SaveToFile(FILE *f_report) {
    fprintf(f_report, "Number of Threads:               %02u\n", this->thread_count);
    for (unsigned int i = 0; i < this->thread_count; i++) {
        fprintf(f_report, "Threads[%02u]:\n", i);
        this->thread_infos[i].SaveToFile(f_report);
    }
}

void ThreadList::ReadThreadsFromProcess(Handle debug_handle, bool is_64_bit) {
    u32 thread_count;
    u64 thread_ids[max_thread_count];
    
    if (R_FAILED(svcGetThreadList(&thread_count, thread_ids, max_thread_count, debug_handle))) {
        this->thread_count = 0;
        return;
    }
    
    if (thread_count > max_thread_count) {
        thread_count = max_thread_count;
    }
    
    for (unsigned int i = 0; i < thread_count; i++) {
        if (this->thread_infos[this->thread_count].ReadFromProcess(debug_handle, thread_ids[this->thread_count], is_64_bit)) {
            this->thread_count++;
        }
    }
}