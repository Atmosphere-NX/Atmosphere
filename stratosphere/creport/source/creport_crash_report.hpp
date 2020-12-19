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
#include "creport_threads.hpp"
#include "creport_modules.hpp"

namespace ams::creport {

    class CrashReport {
        private:
            static constexpr size_t DyingMessageSizeMax = os::MemoryPageSize;
            static constexpr size_t MemoryHeapSize = 512_KB;
            static_assert(MemoryHeapSize >= DyingMessageSizeMax + sizeof(ModuleList) + sizeof(ThreadList) + os::MemoryPageSize);
        private:
            Handle debug_handle = INVALID_HANDLE;
            bool has_extra_info = true;
            Result result = ResultIncompleteReport();

            /* Meta, used for building module/thread list. */
            std::map<u64, u64> thread_tls_map;

            /* Attach process info. */
            svc::DebugInfoCreateProcess process_info = {};
            u64 dying_message_address = 0;
            u64 dying_message_size = 0;
            u8 *dying_message = nullptr;

            /* Exception info. */
            svc::DebugInfoException exception_info = {};
            u64 module_base_address = 0;
            u64 crashed_thread_id = 0;
            ThreadInfo crashed_thread;

            /* Lists. */
            ModuleList *module_list = nullptr;
            ThreadList *thread_list = nullptr;

            /* Memory heap. */
            lmem::HeapHandle heap_handle;
            u8 heap_storage[MemoryHeapSize];
        public:
            Result GetResult() const {
                return this->result;
            }

            bool IsComplete() const {
                return !ResultIncompleteReport::Includes(this->result);
            }

            bool IsOpen() const {
                return this->debug_handle != INVALID_HANDLE;
            }

            bool IsApplication() const {
                return (this->process_info.flags & svc::CreateProcessFlag_IsApplication) != 0;
            }

            bool Is64Bit() const {
                return (this->process_info.flags & svc::CreateProcessFlag_Is64Bit) != 0;
            }

            bool IsUserBreak() const {
                return this->exception_info.type == svc::DebugException_UserBreak;
            }

            bool OpenProcess(os::ProcessId process_id) {
                return R_SUCCEEDED(svcDebugActiveProcess(&this->debug_handle, static_cast<u64>(process_id)));
            }

            void Close() {
                if (this->IsOpen()) {
                    svcCloseHandle(this->debug_handle);
                    this->debug_handle = INVALID_HANDLE;
                }
            }

            void Initialize();

            void BuildReport(os::ProcessId process_id, bool has_extra_info);
            void GetFatalContext(::FatalCpuContext *out) const;
            void SaveReport(bool enable_screenshot);
        private:
            void ProcessExceptions();
            void ProcessDyingMessage();
            void HandleDebugEventInfoCreateProcess(const svc::DebugEventInfo &d);
            void HandleDebugEventInfoCreateThread(const svc::DebugEventInfo &d);
            void HandleDebugEventInfoException(const svc::DebugEventInfo &d);

            void SaveToFile(ScopedFile &file);
    };

}
