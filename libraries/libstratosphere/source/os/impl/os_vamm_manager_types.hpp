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

namespace ams::os::impl {

    class AddressRegionManager;

    class VammManager {
        NON_COPYABLE(VammManager);
        NON_MOVEABLE(VammManager);
        private:
            uintptr_t m_reserved_region_start;
            uintptr_t m_reserved_region_size;
            ReaderWriterLock m_lock;
            AddressRegionManager *m_region_manager;
        public:
            VammManager();

            void InitializeIfEnabled();

            Result AllocateAddressRegion(uintptr_t *out, size_t size);
            Result AllocateMemory(uintptr_t *out, size_t size);
            Result AllocateMemoryPages(uintptr_t address, size_t size);

            Result FreeAddressRegion(uintptr_t address);
            Result FreeMemoryPages(uintptr_t address, size_t size);

            VirtualAddressMemoryResourceUsage GetVirtualAddressMemoryResourceUsage();
        public:
            static bool IsVirtualAddressMemoryEnabled();
    };

}