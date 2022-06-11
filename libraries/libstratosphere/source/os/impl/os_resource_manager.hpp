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
#include "os_rng_manager_impl.hpp"
#include "os_thread_manager_types.hpp"
#include "os_stack_guard_manager_types.hpp"
#include "os_tick_manager_impl.hpp"
#include "os_aslr_space_manager_types.hpp"
#include "os_tls_manager_types.hpp"
#include "os_giant_lock_types.hpp"
#include "os_memory_heap_manager_types.hpp"
#include "os_vamm_manager_types.hpp"

namespace ams::os::impl {

    class OsResourceManager {
        private:
            RngManager  m_rng_manager{};
            AslrSpaceManager m_aslr_space_manager{};
            StackGuardManager m_stack_guard_manager;
            ThreadManager m_thread_manager{};
            //TlsManager m_tls_manager{};
            TickManager m_tick_manager{};
            MemoryHeapManager m_memory_heap_manager;
            VammManager m_vamm_manager;
            GiantLock m_giant_lock{};
        public:
            OsResourceManager() = default;

            constexpr ALWAYS_INLINE RngManager &GetRngManager() { return m_rng_manager; }
            constexpr ALWAYS_INLINE AslrSpaceManager &GetAslrSpaceManager() { return m_aslr_space_manager; }
            constexpr ALWAYS_INLINE ThreadManager &GetThreadManager() { return m_thread_manager; }
            constexpr ALWAYS_INLINE StackGuardManager &GetStackGuardManager() { return m_stack_guard_manager; }
            //constexpr ALWAYS_INLINE TlsManager &GetTlsManager() { return m_tls_manager; }
            constexpr ALWAYS_INLINE TickManager &GetTickManager() { return m_tick_manager; }
            constexpr ALWAYS_INLINE MemoryHeapManager &GetMemoryHeapManager() { return m_memory_heap_manager; }
            constexpr ALWAYS_INLINE VammManager &GetVammManager() { return m_vamm_manager; }
            constexpr ALWAYS_INLINE GiantLock &GetGiantLock() { return m_giant_lock; }
    };

    class ResourceManagerHolder {
        private:
            static constinit util::TypedStorage<OsResourceManager> s_resource_manager_storage;
        private:
            constexpr ResourceManagerHolder() { /* ... */ }
        public:
            static ALWAYS_INLINE void InitializeResourceManagerInstance() {
                /* Construct the resource manager instance. */
                util::ConstructAt(s_resource_manager_storage);
            }

            static ALWAYS_INLINE OsResourceManager &GetResourceManagerInstance() {
                return GetReference(s_resource_manager_storage);
            }
    };

    ALWAYS_INLINE OsResourceManager &GetResourceManager() {
        return ResourceManagerHolder::GetResourceManagerInstance();
    }

}
