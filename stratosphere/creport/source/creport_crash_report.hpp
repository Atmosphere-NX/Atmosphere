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
#include "creport_threads.hpp"
#include "creport_modules.hpp"

namespace ams::creport {

    class CrashReport {
        private:
            static constexpr size_t DyingMessageSizeMax = os::MemoryPageSize;
            static constexpr size_t MemoryHeapSize = 512_KB;
            static_assert(MemoryHeapSize >= DyingMessageSizeMax + sizeof(ModuleList) + sizeof(ThreadList) + os::MemoryPageSize);
        private:
            os::NativeHandle m_debug_handle = os::InvalidNativeHandle;
            bool m_has_extra_info = true;
            Result m_result = creport::ResultIncompleteReport();

            /* Meta, used for building module/thread list. */
            ThreadTlsMap m_thread_tls_map = {};

            /* Attach process info. */
            svc::DebugInfoCreateProcess m_process_info = {};
            u64 m_dying_message_address = 0;
            u64 m_dying_message_size = 0;
            u8 *m_dying_message = nullptr;

            /* Exception info. */
            svc::DebugInfoException m_exception_info = {};
            u64 m_module_base_address = 0;
            u64 m_crashed_thread_id = 0;
            ThreadInfo m_crashed_thread;

            /* Lists. */
            ModuleList *m_module_list = nullptr;
            ThreadList *m_thread_list = nullptr;

            /* Memory heap. */
            lmem::HeapHandle m_heap_handle = nullptr;
            u8 m_heap_storage[MemoryHeapSize] = {};
        public:
            constexpr CrashReport() = default;

            Result GetResult() const {
                return m_result;
            }

            bool IsComplete() const {
                return !ResultIncompleteReport::Includes(m_result);
            }

            bool IsOpen() const {
                return m_debug_handle != os::InvalidNativeHandle;
            }

            bool IsApplication() const {
                return (m_process_info.flags & svc::CreateProcessFlag_IsApplication) != 0;
            }

            bool Is64Bit() const {
                return (m_process_info.flags & svc::CreateProcessFlag_Is64Bit) != 0;
            }

            bool IsUserBreak() const {
                return m_exception_info.type == svc::DebugException_UserBreak;
            }

            bool OpenProcess(os::ProcessId process_id) {
                return R_SUCCEEDED(svc::DebugActiveProcess(std::addressof(m_debug_handle), process_id.value));
            }

            void Close() {
                os::CloseNativeHandle(m_debug_handle);
                m_debug_handle = os::InvalidNativeHandle;
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
