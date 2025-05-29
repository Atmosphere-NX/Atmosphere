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
#include "pm_process_info.hpp"

namespace ams::pm::impl {

    namespace {

        template<size_t MaxProcessInfos>
        class ProcessInfoAllocator {
            NON_COPYABLE(ProcessInfoAllocator);
            NON_MOVEABLE(ProcessInfoAllocator);
            static_assert(MaxProcessInfos >= 0x40, "MaxProcessInfos is too small.");
            private:
                util::TypedStorage<ProcessInfo> m_process_info_storages[MaxProcessInfos]{};
                bool m_process_info_allocated[MaxProcessInfos]{};
                os::SdkMutex m_lock{};
            private:
                constexpr inline size_t GetProcessInfoIndex(ProcessInfo *process_info) const {
                    return process_info - GetPointer(m_process_info_storages[0]);
                }
            public:
                constexpr ProcessInfoAllocator() = default;

                template<typename... Args>
                ProcessInfo *AllocateProcessInfo(Args &&... args) {
                    std::scoped_lock lk(m_lock);

                    for (size_t i = 0; i < MaxProcessInfos; i++) {
                        if (!m_process_info_allocated[i]) {
                            m_process_info_allocated[i] = true;

                            std::memset(m_process_info_storages + i, 0, sizeof(m_process_info_storages[i]));

                            return util::ConstructAt(m_process_info_storages[i], std::forward<Args>(args)...);
                        }
                    }

                    return nullptr;
                }

                void FreeProcessInfo(ProcessInfo *process_info) {
                    std::scoped_lock lk(m_lock);

                    const size_t index = this->GetProcessInfoIndex(process_info);
                    AMS_ABORT_UNLESS(index < MaxProcessInfos);
                    AMS_ABORT_UNLESS(m_process_info_allocated[index]);

                    util::DestroyAt(m_process_info_storages[index]);
                    m_process_info_allocated[index] = false;
                }
        };

        /* Process lists. */
        constinit ProcessList g_process_list;
        constinit ProcessList g_exit_list;

        /* Process Info Allocation. */
        /* Note: The kernel slabheap is size 0x50 -- we allow slightly larger to account for the dead process list. */
        constexpr size_t MaxProcessCount = 0x60;
        constinit ProcessInfoAllocator<MaxProcessCount> g_process_info_allocator;

    }

    ProcessInfo::ProcessInfo(os::NativeHandle h, os::ProcessId pid, ldr::PinId pin, const ncm::ProgramLocation &l, const cfg::OverrideStatus &s, const ProcessAttributes &attrs) : m_process_id(pid), m_pin_id(pin), m_loc(l), m_status(s), m_handle(h), m_state(svc::ProcessState_Created), m_flags(0), m_attrs(attrs) {
        os::InitializeMultiWaitHolder(std::addressof(m_multi_wait_holder), m_handle);
        os::SetMultiWaitHolderUserData(std::addressof(m_multi_wait_holder), reinterpret_cast<uintptr_t>(this));
    }

    ProcessInfo::~ProcessInfo() {
        this->Cleanup();
    }

    void ProcessInfo::Cleanup() {
        if (m_handle != os::InvalidNativeHandle) {
            /* Unregister the process. */
            fsprUnregisterProgram(m_process_id.value);
            sm::manager::UnregisterProcess(m_process_id);
            ldr::pm::UnpinProgram(m_pin_id);

            /* Close the process's handle. */
            os::CloseNativeHandle(m_handle);
            m_handle = os::InvalidNativeHandle;
        }
    }

    ProcessListAccessor GetProcessList() {
        return ProcessListAccessor(g_process_list);
    }

    ProcessListAccessor GetExitList() {
        return ProcessListAccessor(g_exit_list);
    }

    ProcessInfo *AllocateProcessInfo(svc::Handle process_handle, os::ProcessId process_id, ldr::PinId pin_id, const ncm::ProgramLocation &location, const cfg::OverrideStatus &override_status, const ProcessAttributes &attrs) {
        return g_process_info_allocator.AllocateProcessInfo(process_handle, process_id, pin_id, location, override_status, attrs);
    }

    void CleanupProcessInfo(ProcessListAccessor &list, ProcessInfo *process_info) {
        /* Remove the process from the list. */
        list->Remove(process_info);

        /* Delete the process. */
        g_process_info_allocator.FreeProcessInfo(process_info);
    }

}
