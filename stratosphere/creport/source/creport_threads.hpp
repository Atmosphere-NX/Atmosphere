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

            bool ReadFromProcess(Handle debug_handle, std::map<u64, u64> &tls_map, u64 thread_id, bool is_64_bit);
            void SaveToFile(ScopedFile &file);
            void DumpBinary(ScopedFile &file);
        private:
            void TryGetStackInfo(Handle debug_handle);
    };

    class ThreadList {
        private:
            static constexpr size_t ThreadCountMax = 0x60;
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

            void ReadFromProcess(Handle debug_handle, std::map<u64, u64> &tls_map, bool is_64_bit);
            void SaveToFile(ScopedFile &file);
            void DumpBinary(ScopedFile &file, u64 crashed_thread_id);
    };

}
