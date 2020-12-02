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
#include "secmon_error.hpp"

namespace ams {

    namespace {

        constexpr bool SaveSystemStateForDebug = false;

    }
}

namespace ams::diag {

    namespace {

        ALWAYS_INLINE void SaveSystemStateForDebugAbort() {
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x00) = 0xDDDDDDDD;

            u64 temp_reg;
            __asm__ __volatile__("mov %0, lr" : "=r"(temp_reg) :: "memory");
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x10) = static_cast<u32>(temp_reg >>  0);
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x14) = static_cast<u32>(temp_reg >> 32);


            __asm__ __volatile__("mov %0, sp" : "=r"(temp_reg) :: "memory");
            for (int i = 0; i < 0x100; i += 4) {
                *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x20 + i) = *(volatile u32 *)(temp_reg + i);
            }
            *(volatile u32 *)(secmon::MemoryRegionVirtualDevicePmc.GetAddress() + 0x50) = 0x02;
            *(volatile u32 *)(secmon::MemoryRegionVirtualDevicePmc.GetAddress() + 0x00) = 0x10;

            util::WaitMicroSeconds(1000);
        }

    }

    void AbortImpl() {
        /* Perform any necessary (typically none) debugging. */
        if constexpr (SaveSystemStateForDebug) {
            SaveSystemStateForDebugAbort();
        }

        secmon::SetError(pkg1::ErrorInfo_UnknownAbort);
        secmon::ErrorReboot();
    }

    #include <exosphere/diag/diag_detailed_assertion_impl.inc>

}

namespace ams::secmon {

    namespace {

        constexpr inline uintptr_t PMC = MemoryRegionVirtualDevicePmc.GetAddress();

        ALWAYS_INLINE void SaveSystemStateForDebugErrorReboot() {
            u64 temp_reg;
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x00) = 0x5A5A5A5A;

            __asm__ __volatile__("mrs %0, esr_el3" : "=r"(temp_reg) :: "memory");
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x08) = static_cast<u32>(temp_reg >>  0);
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x0C) = static_cast<u32>(temp_reg >> 32);

            __asm__ __volatile__("mrs %0, elr_el3" : "=r"(temp_reg) :: "memory");
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x10) = static_cast<u32>(temp_reg >>  0);
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x14) = static_cast<u32>(temp_reg >> 32);

            __asm__ __volatile__("mrs %0, far_el3" : "=r"(temp_reg) :: "memory");
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x18) = static_cast<u32>(temp_reg >>  0);
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x1C) = static_cast<u32>(temp_reg >> 32);

            __asm__ __volatile__("mov %0, lr" : "=r"(temp_reg) :: "memory");
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x20) = static_cast<u32>(temp_reg >>  0);
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x24) = static_cast<u32>(temp_reg >> 32);

            __asm__ __volatile__("mov %0, sp" : "=r"(temp_reg) :: "memory");
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x30) = static_cast<u32>(temp_reg >>  0);
            *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x34) = static_cast<u32>(temp_reg >> 32);

            for (int i = 0; i < 0x100; i += 4) {
                *(volatile u32 *)(secmon::MemoryRegionVirtualDebug.GetAddress() + 0x40 + i) = *(volatile u32 *)(temp_reg + i);
            }
            *(volatile u32 *)(secmon::MemoryRegionVirtualDevicePmc.GetAddress() + 0x50) = 0x02;
            *(volatile u32 *)(secmon::MemoryRegionVirtualDevicePmc.GetAddress() + 0x00) = 0x10;

            util::WaitMicroSeconds(1000);
        }

    }

    void SetError(pkg1::ErrorInfo info) {
        const uintptr_t address = secmon::MemoryRegionVirtualDevicePmc.GetAddress() + PKG1_SECURE_MONITOR_PMC_ERROR_SCRATCH;

        if (reg::Read(address) == pkg1::ErrorInfo_None) {
            reg::Write(address, info);
        }
    }

    NORETURN void ErrorReboot() {
        /* Perform any necessary (typically none) debugging. */
        if constexpr (SaveSystemStateForDebug) {
            SaveSystemStateForDebugErrorReboot();
        }

        /* Lockout the security engine. */
        se::Lockout();

        /* Lockout fuses. */
        fuse::Lockout();

        /* Disable crypto operations after reboot. */
        reg::Write(PMC + APBDEV_PMC_CRYPTO_OP, 0);

        while (true) {
            wdt::Reboot();
        }
    }

}