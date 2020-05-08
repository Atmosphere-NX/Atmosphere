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
#include "creport_threads.hpp"
#include "creport_modules.hpp"

namespace ams::creport {

    namespace {

        /* Convenience definitions. */
        constexpr u32 LibnxThreadVarMagic   = util::FourCC<'!','T','V','$'>::Code;
        constexpr u32 DumpedThreadInfoMagic = util::FourCC<'D','T','I','2'>::Code;

        /* Types. */
        template<typename T>
        struct StackFrame {
            T fp;
            T lr;
        };

        /* Helpers. */
        template<typename T>
        void ReadStackTrace(size_t *out_trace_size, u64 *out_trace, size_t max_out_trace_size, Handle debug_handle, u64 fp) {
            size_t trace_size = 0;
            u64 cur_fp = fp;

            for (size_t i = 0; i < max_out_trace_size; i++) {
                /* Validate the current frame. */
                if (cur_fp == 0 || (cur_fp % sizeof(T) != 0)) {
                    break;
                }

                /* Read a new frame. */
                StackFrame<T> cur_frame;
                if (R_FAILED(svcReadDebugProcessMemory(&cur_frame, debug_handle, cur_fp, sizeof(cur_frame)))) {
                    break;
                }

                /* Advance to the next frame. */
                out_trace[trace_size++] = cur_frame.lr;
                cur_fp = cur_frame.fp;
            }

            *out_trace_size = trace_size;
        }

    }

    void ThreadList::SaveToFile(ScopedFile &file) {
        file.WriteFormat("Number of Threads:               %02zu\n", this->thread_count);
        for (size_t i = 0; i < this->thread_count; i++) {
            file.WriteFormat("Threads[%02zu]:\n", i);
            this->threads[i].SaveToFile(file);
        }
    }

    void ThreadInfo::SaveToFile(ScopedFile &file) {
        file.WriteFormat("    Thread ID:                   %016lx\n", this->thread_id);
        if (std::strcmp(this->name, "") != 0) {
            file.WriteFormat("    Thread Name:                 %s\n", this->name);
        }
        if (this->stack_top != 0) {
            file.WriteFormat("    Stack Region:                %016lx-%016lx\n", this->stack_bottom, this->stack_top);
        }
        file.WriteFormat("    Registers:\n");
        {
            for (unsigned int i = 0; i <= 28; i++) {
                file.WriteFormat("        X[%02u]:                   %s\n", i, this->module_list->GetFormattedAddressString(this->context.cpu_gprs[i].x));
            }
            file.WriteFormat("        FP:                      %s\n", this->module_list->GetFormattedAddressString(this->context.fp));
            file.WriteFormat("        LR:                      %s\n", this->module_list->GetFormattedAddressString(this->context.lr));
            file.WriteFormat("        SP:                      %s\n", this->module_list->GetFormattedAddressString(this->context.sp));
            file.WriteFormat("        PC:                      %s\n", this->module_list->GetFormattedAddressString(this->context.pc.x));
        }
        if (this->stack_trace_size != 0) {
            file.WriteFormat("    Stack Trace:\n");
            for (size_t i = 0; i < this->stack_trace_size; i++) {
                file.WriteFormat("        ReturnAddress[%02zu]:       %s\n", i, this->module_list->GetFormattedAddressString(this->stack_trace[i]));
            }
        }
        if (this->stack_dump_base != 0) {
            file.WriteFormat("    Stack Dump:                               00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
            for (size_t i = 0; i < 0x10; i++) {
                const size_t ofs = i * 0x10;
                file.WriteFormat("                                 %012lx %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    this->stack_dump_base + ofs, this->stack_dump[ofs + 0], this->stack_dump[ofs + 1], this->stack_dump[ofs + 2], this->stack_dump[ofs + 3], this->stack_dump[ofs + 4], this->stack_dump[ofs + 5], this->stack_dump[ofs + 6], this->stack_dump[ofs + 7],
                    this->stack_dump[ofs + 8], this->stack_dump[ofs + 9], this->stack_dump[ofs + 10], this->stack_dump[ofs + 11], this->stack_dump[ofs + 12], this->stack_dump[ofs + 13], this->stack_dump[ofs + 14], this->stack_dump[ofs + 15]);
            }
        }
        if (this->tls_address != 0) {
            file.WriteFormat("    TLS Address:                 %016lx\n", this->tls_address);
            file.WriteFormat("    TLS Dump:                                 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
            for (size_t i = 0; i < 0x10; i++) {
                const size_t ofs = i * 0x10;
                file.WriteFormat("                                 %012lx %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    this->tls_address + ofs, this->tls[ofs + 0], this->tls[ofs + 1], this->tls[ofs + 2], this->tls[ofs + 3], this->tls[ofs + 4], this->tls[ofs + 5], this->tls[ofs + 6], this->tls[ofs + 7],
                    this->tls[ofs + 8], this->tls[ofs + 9], this->tls[ofs + 10], this->tls[ofs + 11], this->tls[ofs + 12], this->tls[ofs + 13], this->tls[ofs + 14], this->tls[ofs + 15]);
            }
        }
    }

    bool ThreadInfo::ReadFromProcess(Handle debug_handle, std::map<u64, u64> &tls_map, u64 thread_id, bool is_64_bit) {
        /* Set thread id. */
        this->thread_id = thread_id;

        /* Verify that the thread is running or waiting. */
        {
            u64 _;
            u32 _thread_state;
            if (R_FAILED(svcGetDebugThreadParam(&_, &_thread_state, debug_handle, this->thread_id, DebugThreadParam_State))) {
                return false;
            }

            const svc::ThreadState thread_state = static_cast<svc::ThreadState>(_thread_state);
            if (thread_state != svc::ThreadState_Waiting && thread_state != svc::ThreadState_Running) {
                return false;
            }
        }

        /* Get the thread context. */
        if (R_FAILED(svcGetDebugThreadContext(&this->context, debug_handle, this->thread_id, svc::ThreadContextFlag_All))) {
            return false;
        }

        /* In aarch32 mode svcGetDebugThreadContext does not set the LR, FP, and SP registers correctly. */
        if (!is_64_bit) {
            this->context.fp = this->context.cpu_gprs[11].x;
            this->context.sp = this->context.cpu_gprs[13].x;
            this->context.lr = this->context.cpu_gprs[14].x;
        }

        /* Read TLS, if present. */
        /* TODO: struct definitions for nnSdk's ThreadType/TLS Layout? */
        if (tls_map.find(thread_id) != tls_map.end()) {
            this->tls_address = tls_map[thread_id];
            u8 thread_tls[0x200];
            if (R_SUCCEEDED(svcReadDebugProcessMemory(thread_tls, debug_handle, this->tls_address, sizeof(thread_tls)))) {
                std::memcpy(this->tls, thread_tls, sizeof(this->tls));
                /* Try to detect libnx threads, and skip name parsing then. */
                if (*(reinterpret_cast<u32 *>(&thread_tls[0x1E0])) != LibnxThreadVarMagic) {
                    u8 thread_type[0x1D0];
                    const u64 thread_type_addr = *(reinterpret_cast<u64 *>(&thread_tls[0x1F8]));
                    if (R_SUCCEEDED(svcReadDebugProcessMemory(thread_type, debug_handle, thread_type_addr, sizeof(thread_type)))) {
                        /* Check thread name is actually at thread name. */
                        static_assert(0x1A8 - 0x188 == NameLengthMax, "NameLengthMax definition!");
                        if (*(reinterpret_cast<u64 *>(&thread_type[0x1A8])) == thread_type_addr + 0x188) {
                            std::memcpy(this->name, thread_type + 0x188, NameLengthMax);
                        }
                    }
                }
            }
        }

        /* Parse stack extents and dump stack. */
        this->TryGetStackInfo(debug_handle);

        /* Dump stack trace. */
        if (is_64_bit) {
            ReadStackTrace<u64>(&this->stack_trace_size, this->stack_trace, StackTraceSizeMax, debug_handle, this->context.fp);
        } else {
            ReadStackTrace<u32>(&this->stack_trace_size, this->stack_trace, StackTraceSizeMax, debug_handle, this->context.fp);
        }

        return true;
    }

    void ThreadInfo::TryGetStackInfo(Handle debug_handle) {
        /* Query stack region. */
        MemoryInfo mi;
        u32 pi;
        if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, this->context.sp))) {
            return;
        }

        /* Check if sp points into the stack. */
        if (mi.type != MemType_MappedMemory) {
            /* It's possible that sp is below the stack... */
            if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, mi.addr + mi.size)) || mi.type != MemType_MappedMemory) {
                return;
            }
        }

        /* Save stack extents. */
        this->stack_bottom = mi.addr;
        this->stack_top = mi.addr + mi.size;

        /* We always want to dump 0x100 of stack, starting from the lowest 0x10-byte aligned address below the stack pointer. */
        /* Note: if the stack pointer is below the stack bottom, we will start dumping from the stack bottom. */
        this->stack_dump_base = std::min(std::max(this->context.sp & ~0xFul, this->stack_bottom), this->stack_top - sizeof(this->stack_dump));

        /* Try to read stack. */
        if (R_FAILED(svcReadDebugProcessMemory(this->stack_dump, debug_handle, this->stack_dump_base, sizeof(this->stack_dump)))) {
            this->stack_dump_base = 0;
        }
    }

    void ThreadInfo::DumpBinary(ScopedFile &file) {
        /* Dump id and context. */
        file.Write(&this->thread_id, sizeof(this->thread_id));
        file.Write(&this->context, sizeof(this->context));

        /* Dump TLS info and name. */
        file.Write(&this->tls_address, sizeof(this->tls_address));
        file.Write(&this->tls, sizeof(this->tls));
        file.Write(&this->name, sizeof(this->name));

        /* Dump stack extents and stack dump. */
        file.Write(&this->stack_bottom, sizeof(this->stack_bottom));
        file.Write(&this->stack_top, sizeof(this->stack_top));
        file.Write(&this->stack_dump_base, sizeof(this->stack_dump_base));
        file.Write(&this->stack_dump, sizeof(this->stack_dump));

        /* Dump stack trace. */
        {
            const u64 sts = this->stack_trace_size;
            file.Write(&sts, sizeof(sts));
        }
        file.Write(this->stack_trace, this->stack_trace_size);
    }

    void ThreadList::DumpBinary(ScopedFile &file, u64 crashed_thread_id) {
        const u32 magic = DumpedThreadInfoMagic;
        const u32 count = this->thread_count;
        file.Write(&magic, sizeof(magic));
        file.Write(&count, sizeof(count));
        file.Write(&crashed_thread_id, sizeof(crashed_thread_id));
        for (size_t i = 0; i < this->thread_count; i++) {
            this->threads[i].DumpBinary(file);
        }
    }

    void ThreadList::ReadFromProcess(Handle debug_handle, std::map<u64, u64> &tls_map, bool is_64_bit) {
        this->thread_count = 0;

        /* Get thread list. */
        s32 num_threads;
        u64 thread_ids[ThreadCountMax];
        {
            if (R_FAILED(svc::GetThreadList(&num_threads, thread_ids, ThreadCountMax, debug_handle))) {
                return;
            }
            num_threads = std::min(size_t(num_threads), ThreadCountMax);
        }

        /* Parse thread infos. */
        for (s32 i = 0; i < num_threads; i++) {
            if (this->threads[this->thread_count].ReadFromProcess(debug_handle, tls_map, thread_ids[i], is_64_bit)) {
                this->thread_count++;
            }
        }
    }

}
