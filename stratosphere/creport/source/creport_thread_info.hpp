#pragma once
#include <switch.h>
#include <cstdio>

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
        ThreadInfo() : context{0}, thread_id(0), stack_top(0), stack_bottom(0), stack_trace{0}, stack_trace_size(0) { }
        
        u64 GetPC() { return context.pc; }
        u64 GetLR() { return context.lr; }
        u64 GetId() { return thread_id; }
        
        bool ReadFromProcess(Handle debug_handle, u64 thread_id, bool is_64_bit);
        void SaveToFile(FILE *f_report);
        void DumpBinary(FILE *f_bin);
    private:
        void TryGetStackInfo(Handle debug_handle);
};

class ThreadList {
    private:
        static const size_t max_thread_count = 0x60;
        u32 thread_count;
        ThreadInfo thread_infos[max_thread_count];
    public:
        ThreadList() : thread_count(0) { }
        
        void SaveToFile(FILE *f_report);
        void DumpBinary(FILE *f_bin, u64 crashed_id);
        void ReadThreadsFromProcess(Handle debug_handle, bool is_64_bit);
};
