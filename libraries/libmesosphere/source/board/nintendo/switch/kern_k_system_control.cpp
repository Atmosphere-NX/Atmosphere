/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <mesosphere.hpp>
#include "kern_secure_monitor.hpp"

namespace ams::kern {

    namespace {

        /* Convenience definitions. */
        constexpr size_t FourGigabytes  = 0x100000000ul;
        constexpr size_t SixGigabytes   = 0x180000000ul;
        constexpr size_t EightGigabytes = 0x200000000ul;

        size_t GetRealMemorySize() {
            /* TODO: Move this into a header for the MC in general. */
            constexpr u32 MemoryControllerConfigurationRegister = 0x70019050;
            u32 config_value;
            MESOSPHERE_ABORT_UNLESS(smc::ReadWriteRegister(&config_value, MemoryControllerConfigurationRegister, 0, 0));
            return static_cast<size_t>(config_value & 0x3FFF) << 20;
        }

        inline u64 GetKernelConfiguration() {
            u64 value = 0;
            smc::GetConfig(&value, 1, smc::ConfigItem::KernelConfiguration);
            return value;
        }

        inline smc::MemoryMode GetMemoryMode() {
            return static_cast<smc::MemoryMode>((GetKernelConfiguration() >> 10) & 0x3);
        }

        size_t GetIntendedMemorySize() {
            const smc::MemoryMode memory_mode = GetMemoryMode();
            switch (memory_mode) {
                case smc::MemoryMode_4GB:
                default: /* All invalid modes should go to 4GB. */
                    return FourGigabytes;
                case smc::MemoryMode_6GB:
                    return SixGigabytes;
                case smc::MemoryMode_8GB:
                    return EightGigabytes;
            }
        }

    }

    /* Initialization. */
    KPhysicalAddress KSystemControl::GetKernelPhysicalBaseAddress(uintptr_t base_address) {
        const size_t real_dram_size     = GetRealMemorySize();
        const size_t intended_dram_size = GetIntendedMemorySize();
        if (intended_dram_size * 2 < real_dram_size) {
            return base_address;
        } else {
            return base_address + ((real_dram_size - intended_dram_size) / 2);
        }
    }

    bool KSystemControl::ShouldIncreaseResourceRegionSize() {
        return (GetKernelConfiguration() >> 3) & 1;
    }

    void KSystemControl::StopSystem() {
        /* Display a panic screen via exosphere. */
        smc::Panic(0xF00);
        while (true) { /* ... */ }
    }

}