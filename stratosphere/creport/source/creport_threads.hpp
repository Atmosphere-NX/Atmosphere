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
#pragma once
#include <stratosphere.hpp>
#include "creport_scoped_file.hpp"

namespace ams::creport {

    /* Forward declare ModuleList class. */
    class ModuleList;

    static constexpr size_t ThreadCountMax = 0x60;

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

    using ThreadTlsMap = ThreadTlsMapImpl<ThreadCountMax>;

    class ThreadInfo {
        private:
            static constexpr size_t StackTraceSizeMax = 0x20;
            static constexpr size_t NameLengthMax = 0x20;
        private:
            svc::ThreadContext m_context = {};
            u64 m_thread_id = 0;
            u64 m_stack_top = 0;
            u64 m_stack_bottom = 0;
            u64 m_stack_trace[StackTraceSizeMax] = {};
            size_t m_stack_trace_size = 0;
            u64 m_tls_address = 0;
            u8 m_tls[0x100] = {};
            u64 m_stack_dump_base = 0;
            u8 m_stack_dump[0x100] = {};
            char m_name[NameLengthMax + 1] = {};
            ModuleList *m_module_list = nullptr;
        public:
            u64 GetGeneralPurposeRegister(size_t i) const {
                return m_context.r[i];
            }

            u64 GetPC() const {
                return m_context.pc;
            }

            u64 GetLR() const {
                return m_context.lr;
            }

            u64 GetFP() const {
                return m_context.fp;
            }

            u64 GetSP() const {
                return m_context.sp;
            }

            u64 GetThreadId() const {
                return m_thread_id;
            }

            size_t GetStackTraceSize() const {
                return m_stack_trace_size;
            }

            u64 GetStackTrace(size_t i) const {
                return m_stack_trace[i];
            }

            void SetModuleList(ModuleList *ml) {
                m_module_list = ml;
            }

            bool ReadFromProcess(os::NativeHandle debug_handle, ThreadTlsMap &tls_map, u64 thread_id, bool is_64_bit);
            void SaveToFile(ScopedFile &file);
            void DumpBinary(ScopedFile &file);
        private:
            void TryGetStackInfo(os::NativeHandle debug_handle);
    };

    class ThreadList {
        private:
            size_t m_thread_count = 0;
            ThreadInfo m_threads[ThreadCountMax];
        public:
            size_t GetThreadCount() const {
                return m_thread_count;
            }

            const ThreadInfo &GetThreadInfo(size_t i) const {
                return m_threads[i];
            }

            void SetModuleList(ModuleList *ml) {
                for (size_t i = 0; i < m_thread_count; i++) {
                    m_threads[i].SetModuleList(ml);
                }
            }

            void ReadFromProcess(os::NativeHandle debug_handle, ThreadTlsMap &tls_map, bool is_64_bit);
            void SaveToFile(ScopedFile &file);
            void DumpBinary(ScopedFile &file, u64 crashed_thread_id);
    };

}
