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
#include <exosphere.hpp>
#include "secmon_boot.hpp"
#include "../secmon_setup.hpp"

extern "C" void __libc_init_array();

namespace ams::secmon::boot {

    void Initialize(uintptr_t bss_start, size_t bss_end, uintptr_t boot_bss_start, uintptr_t boot_bss_end) {
        /* Set our start time. */
        auto &secmon_params = *MemoryRegionPhysicalDeviceBootloaderParams.GetPointer<pkg1::SecureMonitorParameters>();
        secmon_params.secmon_start_time = *reinterpret_cast<volatile u32 *>(MemoryRegionPhysicalDeviceTimer.GetAddress() + 0x10);

        /* Setup DMA controllers. */
        SetupSocDmaControllers();

        /* Make the page table. */
        MakePageTable();

        /* Setup memory controllers the MMU. */
        SetupCpuMemoryControllersEnableMmu();

        /* Clear bss. */
        std::memset(reinterpret_cast<void *>(bss_start),      0, bss_end - bss_start);
        std::memset(reinterpret_cast<void *>(boot_bss_start), 0, boot_bss_end - boot_bss_start);

        /* Call init array. */
        __libc_init_array();
    }

}