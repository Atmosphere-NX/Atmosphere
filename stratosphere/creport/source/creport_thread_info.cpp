#include <switch.h>

#include "creport_thread_info.hpp"

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
    if (R_FAILED(svcGetDebugThreadContext((u8 *)&this->context, debug_handle, this->thread_id, 0xF))) {
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