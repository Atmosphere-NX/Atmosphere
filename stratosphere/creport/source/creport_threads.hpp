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
            ThreadContext context = {};
            u64 thread_id = 0;
            u64 stack_top = 0;
            u64 stack_bottom = 0;
            u64 stack_trace[StackTraceSizeMax] = {};
            size_t stack_trace_size = 0;
            u64 tls_address = 0;
            u8 tls[0x100] = {};
            u64 stack_dump_base = 0;
            u8 stack_dump[0x100] = {};
            char name[NameLengthMax + 1] = {};
            ModuleList *module_list = nullptr;
        public:
            u64 GetGeneralPurposeRegister(size_t i) const {
                return this->context.cpu_gprs[i].x;
            }

            u64 GetPC() const {
                return this->context.pc.x;
            }

            u64 GetLR() const {
                return this->context.lr;
            }

            u64 GetFP() const {
                return this->context.fp;
            }

            u64 GetSP() const {
                return this->context.sp;
            }

            u64 GetThreadId() const {
                return this->thread_id;
            }

            size_t GetStackTraceSize() const {
                return this->stack_trace_size;
            }

            u64 GetStackTrace(size_t i) const {
                return this->stack_trace[i];
            }

            void SetModuleList(ModuleList *ml) {
                this->module_list = ml;
            }

            bool ReadFromProcess(Handle debug_handle, ThreadTlsMap &tls_map, u64 thread_id, bool is_64_bit);
            void SaveToFile(ScopedFile &file);
            void DumpBinary(ScopedFile &file);
        private:
            void TryGetStackInfo(Handle debug_handle);
    };

    class ThreadList {
        private:
            size_t thread_count = 0;
            ThreadInfo threads[ThreadCountMax];
        public:
            size_t GetThreadCount() const {
                return this->thread_count;
            }

            const ThreadInfo &GetThreadInfo(size_t i) const {
                return this->threads[i];
            }

            void SetModuleList(ModuleList *ml) {
                for (size_t i = 0; i < this->thread_count; i++) {
                    this->threads[i].SetModuleList(ml);
                }
            }

            void ReadFromProcess(Handle debug_handle, ThreadTlsMap &tls_map, bool is_64_bit);
            void SaveToFile(ScopedFile &file);
            void DumpBinary(ScopedFile &file, u64 crashed_thread_id);
    };

}
