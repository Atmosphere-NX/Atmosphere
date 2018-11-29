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
 
#pragma once
#include <switch.h>
#include <cstdio>

#include "creport_debug_types.hpp"

class CodeList;

class ThreadInfo {
    public:
        ThreadContext context{};
        u64 thread_id = 0;
        u64 stack_top = 0;
        u64 stack_bottom = 0;
        u64 stack_trace[0x20]{};
        u32 stack_trace_size = 0;
        CodeList *code_list;
    public:        
        u64 GetPC() const { return context.pc.x; }
        u64 GetLR() const { return context.lr; }
        u64 GetId() const { return thread_id; }
        u32 GetStackTraceSize() const { return stack_trace_size; }
        u64 GetStackTrace(u32 i) const { return stack_trace[i]; }
        
        bool ReadFromProcess(Handle debug_handle, u64 thread_id, bool is_64_bit);
        void SaveToFile(FILE *f_report);
        void DumpBinary(FILE *f_bin);
        void SetCodeList(CodeList *cl) { this->code_list = cl; }
    private:
        void TryGetStackInfo(Handle debug_handle);
};

class ThreadList {
    private:
        static const size_t max_thread_count = 0x60;
        u32 thread_count = 0;
        ThreadInfo thread_infos[max_thread_count];
    public:
        u32 GetThreadCount() const { return thread_count; }
        const ThreadInfo *GetThreadInfo(u32 i) const { return &thread_infos[i]; }
        
        void SaveToFile(FILE *f_report);
        void DumpBinary(FILE *f_bin, u64 crashed_id);
        void ReadThreadsFromProcess(Handle debug_handle, bool is_64_bit);
        void SetCodeList(CodeList *cl) { 
            for (u32 i = 0; i < thread_count; i++) {
                thread_infos[i].SetCodeList(cl);
            }
        }
};
