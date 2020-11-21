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
#include "os_rng_manager_impl.hpp"
#include "os_thread_manager_types.hpp"
#include "os_tick_manager_impl.hpp"
#include "os_aslr_space_manager_types.hpp"

namespace ams::os::impl {

    class OsResourceManager {
        private:
            RngManager  rng_manager{};
            AslrSpaceManager aslr_space_manager;
            /* TODO */
            ThreadManager thread_manager{};
            /* TODO */
            TickManager tick_manager{};
            /* TODO */
        public:
            OsResourceManager() = default;

            constexpr ALWAYS_INLINE RngManager &GetRngManager() { return this->rng_manager; }
            constexpr ALWAYS_INLINE AslrSpaceManager &GetAslrSpaceManager() { return this->aslr_space_manager; }
            constexpr ALWAYS_INLINE ThreadManager &GetThreadManager() { return this->thread_manager; }
            constexpr ALWAYS_INLINE TickManager &GetTickManager() { return this->tick_manager; }
    };

    class ResourceManagerHolder {
        private:
            static TYPED_STORAGE(OsResourceManager) s_resource_manager_storage;
        private:
            constexpr ResourceManagerHolder() { /* ... */ }
        public:
            static ALWAYS_INLINE void InitializeResourceManagerInstance() {
                /* Construct the resource manager instance. */
                new (GetPointer(s_resource_manager_storage)) OsResourceManager;
            }

            static ALWAYS_INLINE OsResourceManager &GetResourceManagerInstance() {
                return GetReference(s_resource_manager_storage);
            }
    };

    ALWAYS_INLINE OsResourceManager &GetResourceManager() {
        return ResourceManagerHolder::GetResourceManagerInstance();
    }

}
