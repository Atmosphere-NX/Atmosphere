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
#include <mesosphere.hpp>
#include "kern_secure_monitor.hpp"

namespace ams::kern {

    namespace {

        /* Global variables for randomness. */
        /* Incredibly, N really does use std:: randomness... */
        bool         g_initialized_random_generator;
        std::mt19937 g_random_generator;
        KSpinLock    g_random_lock;

        ALWAYS_INLINE size_t GetRealMemorySizeForInit() {
            /* TODO: Move this into a header for the MC in general. */
            constexpr u32 MemoryControllerConfigurationRegister = 0x70019050;
            u32 config_value;
            MESOSPHERE_INIT_ABORT_UNLESS(smc::init::ReadWriteRegister(&config_value, MemoryControllerConfigurationRegister, 0, 0));
            return static_cast<size_t>(config_value & 0x3FFF) << 20;
        }

        ALWAYS_INLINE util::BitPack32 GetKernelConfigurationForInit() {
            u64 value = 0;
            smc::init::GetConfig(&value, 1, smc::ConfigItem::KernelConfiguration);
            return util::BitPack32{static_cast<u32>(value)};
        }

        ALWAYS_INLINE u32 GetMemoryModeForInit() {
            u64 value = 0;
            smc::init::GetConfig(&value, 1, smc::ConfigItem::MemoryMode);
            return static_cast<u32>(value);
        }

        ALWAYS_INLINE smc::MemoryArrangement GetMemoryArrangeForInit() {
            switch(GetMemoryModeForInit() & 0x3F) {
                case 0x01:
                default:
                    return smc::MemoryArrangement_4GB;
                case 0x02:
                    return smc::MemoryArrangement_4GBForAppletDev;
                case 0x03:
                    return smc::MemoryArrangement_4GBForSystemDev;
                case 0x11:
                    return smc::MemoryArrangement_6GB;
                case 0x12:
                    return smc::MemoryArrangement_6GBForAppletDev;
                case 0x21:
                    return smc::MemoryArrangement_8GB;
            }
        }

        ALWAYS_INLINE u64 GenerateRandomU64ForInit() {
            u64 value;
            smc::init::GenerateRandomBytes(&value, sizeof(value));
            return value;
        }

    }

    /* Initialization. */
    size_t KSystemControl::Init::GetIntendedMemorySize() {
        switch (GetKernelConfigurationForInit().Get<smc::KernelConfiguration::MemorySize>()) {
            case smc::MemorySize_4GB:
            default: /* All invalid modes should go to 4GB. */
                return 4_GB;
            case smc::MemorySize_6GB:
                return 6_GB;
            case smc::MemorySize_8GB:
                return 8_GB;
        }
    }

    KPhysicalAddress KSystemControl::Init::GetKernelPhysicalBaseAddress(uintptr_t base_address) {
        const size_t real_dram_size     = GetRealMemorySizeForInit();
        const size_t intended_dram_size = KSystemControl::Init::GetIntendedMemorySize();
        if (intended_dram_size * 2 < real_dram_size) {
            return base_address;
        } else {
            return base_address + ((real_dram_size - intended_dram_size) / 2);
        }
    }

    bool KSystemControl::Init::ShouldIncreaseThreadResourceLimit() {
        return GetKernelConfigurationForInit().Get<smc::KernelConfiguration::IncreaseThreadResourceLimit>();
    }

    size_t KSystemControl::Init::GetApplicationPoolSize() {
        switch (GetMemoryArrangeForInit()) {
            case smc::MemoryArrangement_4GB:
            default:
                return 3285_MB;
            case smc::MemoryArrangement_4GBForAppletDev:
                return 2048_MB;
            case smc::MemoryArrangement_4GBForSystemDev:
                return 3285_MB;
            case smc::MemoryArrangement_6GB:
                return 4916_MB;
            case smc::MemoryArrangement_6GBForAppletDev:
                return 3285_MB;
            case smc::MemoryArrangement_8GB:
                return 4916_MB;
        }
    }

    size_t KSystemControl::Init::GetAppletPoolSize() {
        switch (GetMemoryArrangeForInit()) {
            case smc::MemoryArrangement_4GB:
            default:
                return 507_MB;
            case smc::MemoryArrangement_4GBForAppletDev:
                return 1554_MB;
            case smc::MemoryArrangement_4GBForSystemDev:
                return 448_MB;
            case smc::MemoryArrangement_6GB:
                return 562_MB;
            case smc::MemoryArrangement_6GBForAppletDev:
                return 2193_MB;
            case smc::MemoryArrangement_8GB:
                return 2193_MB;
        }
    }

    size_t KSystemControl::Init::GetMinimumNonSecureSystemPoolSize() {
        /* TODO: Where does this constant actually come from? */
        return 0x29C8000;
    }

    void KSystemControl::Init::CpuOn(u64 core_id, uintptr_t entrypoint, uintptr_t arg) {
        smc::init::CpuOn(core_id, entrypoint, arg);
    }

    /* Randomness for Initialization. */
    void KSystemControl::Init::GenerateRandomBytes(void *dst, size_t size) {
        MESOSPHERE_INIT_ABORT_UNLESS(size <= 0x38);
        smc::init::GenerateRandomBytes(dst, size);
    }

    u64 KSystemControl::Init::GenerateRandomRange(u64 min, u64 max) {
        const u64 range_size    = ((max + 1) - min);
        const u64 effective_max = (std::numeric_limits<u64>::max() / range_size) * range_size;
        while (true) {
            if (const u64 rnd = GenerateRandomU64ForInit(); rnd < effective_max) {
                return min + (rnd % range_size);
            }
        }
    }

    /* Randomness. */
    void KSystemControl::GenerateRandomBytes(void *dst, size_t size) {
        MESOSPHERE_INIT_ABORT_UNLESS(size <= 0x38);
        smc::GenerateRandomBytes(dst, size);
    }

    u64 KSystemControl::GenerateRandomRange(u64 min, u64 max) {
        KScopedInterruptDisable intr_disable;
        KScopedSpinLock lk(g_random_lock);

        if (AMS_UNLIKELY(!g_initialized_random_generator)) {
            u64 seed;
            GenerateRandomBytes(&seed, sizeof(seed));
            g_random_generator.seed(seed);
            g_initialized_random_generator = true;
        }

        return (std::uniform_int_distribution<u64>(min, max))(g_random_generator);
    }

    void KSystemControl::StopSystem() {
        /* Display a panic screen via exosphere. */
        smc::Panic(0xF00);
        while (true) { /* ... */ }
    }

}