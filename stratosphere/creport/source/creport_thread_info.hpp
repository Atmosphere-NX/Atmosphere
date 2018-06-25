#pragma once
#include <switch.h>

#include "creport_debug_types.hpp"

struct FpuReg {
    u64 _[2]; /* TODO: uint128? */
};

struct DebugThreadContext {
    union {
        u64 x[0x20];
        struct {
            u64 _x[29];
            u64 fp;
            u64 lr;
            u64 sp;
        };
    };
    u64 pc;
    u32 psr;
    /* 32-bits of padding. */
    FpuReg fpu_reg[0x20];
    u32 fpcr;
    u32 fpsr;
    u64 tpidr;
};

static_assert(sizeof(DebugThreadContext) == 0x320, "Incorrect DebugThreadContext Definition!");

class ThreadInfo {
    private:
        DebugThreadContext context;
        u64 thread_id;
        u64 stack_top;
        u64 stack_bottom;
        u64 stack_trace[0x20];
        u32 stack_trace_size;
    public:
        ThreadInfo() {
            (*this) = {};
        }
        
        bool ReadFromProcess(Handle debug_handle, u64 thread_id, bool is_64_bit);
};