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
        void ReadStackTrace(size_t *out_trace_size, u64 *out_trace, size_t max_out_trace_size, os::NativeHandle debug_handle, u64 fp) {
            size_t trace_size = 0;
            u64 cur_fp = fp;

            for (size_t i = 0; i < max_out_trace_size; i++) {
                /* Validate the current frame. */
                if (cur_fp == 0 || (cur_fp % sizeof(T) != 0)) {
                    break;
                }

                /* Read a new frame. */
                StackFrame<T> cur_frame;
                if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(cur_frame)), debug_handle, cur_fp, sizeof(cur_frame)))) {
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
        file.WriteFormat("Number of Threads:               %02zu\n", m_thread_count);
        for (size_t i = 0; i < m_thread_count; i++) {
            file.WriteFormat("Threads[%02zu]:\n", i);
            m_threads[i].SaveToFile(file);
        }
    }

    void ThreadInfo::SaveToFile(ScopedFile &file) {
        file.WriteFormat("    Thread ID:                   %016lx\n", m_thread_id);
        if (std::strcmp(m_name, "") != 0) {
            file.WriteFormat("    Thread Name:                 %s\n", m_name);
        }
        if (m_stack_top != 0) {
            file.WriteFormat("    Stack Region:                %016lx-%016lx\n", m_stack_bottom, m_stack_top);
        }
        file.WriteFormat("    Registers:\n");
        {
            for (unsigned int i = 0; i <= 28; i++) {
                file.WriteFormat("        X[%02u]:                   %s\n", i, m_module_list->GetFormattedAddressString(m_context.r[i]));
            }
            file.WriteFormat("        FP:                      %s\n", m_module_list->GetFormattedAddressString(m_context.fp));
            file.WriteFormat("        LR:                      %s\n", m_module_list->GetFormattedAddressString(m_context.lr));
            file.WriteFormat("        SP:                      %s\n", m_module_list->GetFormattedAddressString(m_context.sp));
            file.WriteFormat("        PC:                      %s\n", m_module_list->GetFormattedAddressString(m_context.pc));
        }
        if (m_stack_trace_size != 0) {
            file.WriteFormat("    Stack Trace:\n");
            for (size_t i = 0; i < m_stack_trace_size; i++) {
                file.WriteFormat("        ReturnAddress[%02zu]:       %s\n", i, m_module_list->GetFormattedAddressString(m_stack_trace[i]));
            }
        }
        if (m_stack_dump_base != 0) {
            file.WriteFormat("    Stack Dump:                               00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
            for (size_t i = 0; i < 0x10; i++) {
                const size_t ofs = i * 0x10;
                file.WriteFormat("                                 %012lx %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    m_stack_dump_base + ofs, m_stack_dump[ofs + 0], m_stack_dump[ofs + 1], m_stack_dump[ofs + 2], m_stack_dump[ofs + 3], m_stack_dump[ofs + 4], m_stack_dump[ofs + 5], m_stack_dump[ofs + 6], m_stack_dump[ofs + 7],
                    m_stack_dump[ofs + 8], m_stack_dump[ofs + 9], m_stack_dump[ofs + 10], m_stack_dump[ofs + 11], m_stack_dump[ofs + 12], m_stack_dump[ofs + 13], m_stack_dump[ofs + 14], m_stack_dump[ofs + 15]);
            }
        }
        if (m_tls_address != 0) {
            file.WriteFormat("    TLS Address:                 %016lx\n", m_tls_address);
            file.WriteFormat("    TLS Dump:                                 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
            for (size_t i = 0; i < 0x10; i++) {
                const size_t ofs = i * 0x10;
                file.WriteFormat("                                 %012lx %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    m_tls_address + ofs, m_tls[ofs + 0], m_tls[ofs + 1], m_tls[ofs + 2], m_tls[ofs + 3], m_tls[ofs + 4], m_tls[ofs + 5], m_tls[ofs + 6], m_tls[ofs + 7],
                    m_tls[ofs + 8], m_tls[ofs + 9], m_tls[ofs + 10], m_tls[ofs + 11], m_tls[ofs + 12], m_tls[ofs + 13], m_tls[ofs + 14], m_tls[ofs + 15]);
            }
        }
    }

    bool ThreadInfo::ReadFromProcess(os::NativeHandle debug_handle, ThreadTlsMap &tls_map, u64 thread_id, bool is_64_bit) {
        /* Set thread id. */
        m_thread_id = thread_id;

        /* Verify that the thread is running or waiting. */
        {
            u64 _;
            u32 _thread_state;
            if (R_FAILED(svc::GetDebugThreadParam(&_, &_thread_state, debug_handle, m_thread_id, svc::DebugThreadParam_State))) {
                return false;
            }

            const svc::ThreadState thread_state = static_cast<svc::ThreadState>(_thread_state);
            if (thread_state != svc::ThreadState_Waiting && thread_state != svc::ThreadState_Running) {
                return false;
            }
        }

        /* Get the thread context. */
        if (R_FAILED(svc::GetDebugThreadContext(std::addressof(m_context), debug_handle, m_thread_id, svc::ThreadContextFlag_All))) {
            return false;
        }

        /* In aarch32 mode svc::GetDebugThreadContext does not set the LR, FP, and SP registers correctly. */
        if (!is_64_bit) {
            m_context.fp = m_context.r[11];
            m_context.sp = m_context.r[13];
            m_context.lr = m_context.r[14];
        }

        /* Read TLS, if present. */
        /* TODO: struct definitions for nnSdk's ThreadType/TLS Layout? */
        m_tls_address = 0;
        if (tls_map.GetThreadTls(std::addressof(m_tls_address), thread_id)) {
            u8 thread_tls[sizeof(svc::ThreadLocalRegion)];
            if (R_SUCCEEDED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(thread_tls), debug_handle, m_tls_address, sizeof(thread_tls)))) {
                std::memcpy(m_tls, thread_tls, sizeof(m_tls));
                /* Try to detect libnx threads, and skip name parsing then. */
                if (*(reinterpret_cast<u32 *>(std::addressof(thread_tls[0x1E0]))) != LibnxThreadVarMagic) {
                    u8 thread_type[0x1C0];
                    const u64 thread_type_addr = *(reinterpret_cast<u64 *>(std::addressof(thread_tls[0x1F8])));
                    if (R_SUCCEEDED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(thread_type), debug_handle, thread_type_addr, sizeof(thread_type)))) {
                        /* Get the thread version. */
                        const u16 thread_version = *reinterpret_cast<u16 *>(std::addressof(thread_type[0x46]));
                        if (thread_version == 0 || thread_version == 0xFFFF) {
                            /* Check thread name is actually at thread name. */
                            static_assert(0x1A8 - 0x188 == NameLengthMax, "NameLengthMax definition!");
                            if (*(reinterpret_cast<u64 *>(std::addressof(thread_type[0x1A8]))) == thread_type_addr + 0x188) {
                                std::memcpy(m_name, thread_type + 0x188, NameLengthMax);
                            }
                        } else if (thread_version == 1) {
                            static_assert(0x1A0 - 0x180 == NameLengthMax, "NameLengthMax definition!");
                            if (*(reinterpret_cast<u64 *>(std::addressof(thread_type[0x1A0]))) == thread_type_addr + 0x180) {
                                std::memcpy(m_name, thread_type + 0x180, NameLengthMax);
                            }
                        }
                    }
                }
            }
        }

        /* Parse stack extents and dump stack. */
        this->TryGetStackInfo(debug_handle);

        /* Dump stack trace. */
        if (is_64_bit) {
            ReadStackTrace<u64>(std::addressof(m_stack_trace_size), m_stack_trace, StackTraceSizeMax, debug_handle, m_context.fp);
        } else {
            ReadStackTrace<u32>(std::addressof(m_stack_trace_size), m_stack_trace, StackTraceSizeMax, debug_handle, m_context.fp);
        }

        return true;
    }

    void ThreadInfo::TryGetStackInfo(os::NativeHandle debug_handle) {
        /* Query stack region. */
        svc::MemoryInfo mi;
        svc::PageInfo pi;
        if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), debug_handle, m_context.sp))) {
            return;
        }

        /* Check if sp points into the stack. */
        if (mi.state != svc::MemoryState_Stack) {
            /* It's possible that sp is below the stack... */
            if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), debug_handle, mi.base_address + mi.size)) || mi.state != svc::MemoryState_Stack) {
                return;
            }
        }

        /* Save stack extents. */
        m_stack_bottom = mi.base_address;
        m_stack_top    = mi.base_address + mi.size;

        /* We always want to dump 0x100 of stack, starting from the lowest 0x10-byte aligned address below the stack pointer. */
        /* Note: if the stack pointer is below the stack bottom, we will start dumping from the stack bottom. */
        m_stack_dump_base = std::min(std::max(m_context.sp & ~0xFul, m_stack_bottom), m_stack_top - sizeof(m_stack_dump));

        /* Try to read stack. */
        if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(m_stack_dump), debug_handle, m_stack_dump_base, sizeof(m_stack_dump)))) {
            m_stack_dump_base = 0;
        }
    }

    void ThreadInfo::DumpBinary(ScopedFile &file) {
        /* Dump id and context. */
        file.Write(std::addressof(m_thread_id), sizeof(m_thread_id));
        file.Write(std::addressof(m_context), sizeof(m_context));

        /* Dump TLS info and name. */
        file.Write(std::addressof(m_tls_address), sizeof(m_tls_address));
        file.Write(std::addressof(m_tls), sizeof(m_tls));
        file.Write(std::addressof(m_name), sizeof(m_name));

        /* Dump stack extents and stack dump. */
        file.Write(std::addressof(m_stack_bottom), sizeof(m_stack_bottom));
        file.Write(std::addressof(m_stack_top), sizeof(m_stack_top));
        file.Write(std::addressof(m_stack_dump_base), sizeof(m_stack_dump_base));
        file.Write(std::addressof(m_stack_dump), sizeof(m_stack_dump));

        /* Dump stack trace. */
        {
            const u64 sts = m_stack_trace_size;
            file.Write(std::addressof(sts), sizeof(sts));
        }
        file.Write(m_stack_trace, m_stack_trace_size);
    }

    void ThreadList::DumpBinary(ScopedFile &file, u64 crashed_thread_id) {
        const u32 magic = DumpedThreadInfoMagic;
        const u32 count = m_thread_count;
        file.Write(std::addressof(magic), sizeof(magic));
        file.Write(std::addressof(count), sizeof(count));
        file.Write(std::addressof(crashed_thread_id), sizeof(crashed_thread_id));
        for (size_t i = 0; i < m_thread_count; i++) {
            m_threads[i].DumpBinary(file);
        }
    }

    void ThreadList::ReadFromProcess(os::NativeHandle debug_handle, ThreadTlsMap &tls_map, bool is_64_bit) {
        m_thread_count = 0;

        /* Get thread list. */
        s32 num_threads;
        u64 thread_ids[ThreadCountMax];
        {
            if (R_FAILED(svc::GetThreadList(std::addressof(num_threads), thread_ids, ThreadCountMax, debug_handle))) {
                return;
            }
            num_threads = std::min(size_t(num_threads), ThreadCountMax);
        }

        /* Parse thread infos. */
        for (s32 i = 0; i < num_threads; i++) {
            if (m_threads[m_thread_count].ReadFromProcess(debug_handle, tls_map, thread_ids[i], is_64_bit)) {
                m_thread_count++;
            }
        }
    }

}
