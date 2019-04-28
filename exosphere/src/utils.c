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
 
#include <stdbool.h>
#include <string.h>
#include "utils.h"
#include "se.h"
#include "fuse.h"
#include "pmc.h"
#include "timers.h"

#define SAVE_SYSREG64(reg, ofs) do { __asm__ __volatile__ ("mrs %0, " #reg : "=r"(temp_reg) :: "memory"); MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_DEBUG_IRAM) + ofs) = (uint32_t)((temp_reg >> 0) & 0xFFFFFFFFULL); MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_DEBUG_IRAM) + ofs + 4) = (uint32_t)((temp_reg >> 32) & 0xFFFFFFFFULL); } while(false)

__attribute__ ((noreturn)) void panic(uint32_t code) {
    /* Set Panic Code for NX_BOOTLOADER. */
    if (APBDEV_PMC_SCRATCH200_0 == 0) {
        APBDEV_PMC_SCRATCH200_0 = code;
    }
    
    /* // Uncomment for Debugging.
    uint64_t temp_reg;
    MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_DEBUG_IRAM)) = APBDEV_PMC_SCRATCH200_0;
    SAVE_SYSREG64(ESR_EL3, 0x10);
    SAVE_SYSREG64(ELR_EL3, 0x18);
    SAVE_SYSREG64(FAR_EL3, 0x20);
    MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x450ull) = 0x2;
    MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x400ull) = 0x10; */
    
    
    /* TODO: Custom Panic Driver, which displays to screen without rebooting. */
    /* For now, just use NX BOOTLOADER's panic. */
    fuse_disable_programming();
    APBDEV_PMC_CRYPTO_OP_0 = 1; /* Disable all SE operations. */
    // while (1) { }
    watchdog_reboot();
}

__attribute__ ((noreturn)) void generic_panic(void) {
    /* //Uncomment for Debugging.
    uint64_t temp_reg;    
    do { __asm__ __volatile__ ("mov %0, LR" : "=r"(temp_reg) :: "memory"); } while (false);
    MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_DEBUG_IRAM) + 0x28) = (uint32_t)((temp_reg >> 0) & 0xFFFFFFFFULL);
    MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_DEBUG_IRAM) + 0x28 + 4) = (uint32_t)((temp_reg >> 32) & 0xFFFFFFFFULL);
    do { __asm__ __volatile__ ("mov %0, SP" : "=r"(temp_reg) :: "memory"); } while (false);
    for (unsigned int i = 0; i < 0x80; i += 4) {
         MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_DEBUG_IRAM) + 0x40 + i) = *((volatile uint32_t *)(temp_reg + i));
    } */
    panic(0xFF000006);
}

__attribute__ ((noreturn)) void panic_predefined(uint32_t which) {
    static const uint32_t codes[0x10] = {COLOR_0, COLOR_1, COLOR_2, COLOR_3, COLOR_4, COLOR_5, COLOR_6, COLOR_7, COLOR_8, COLOR_9, COLOR_A, COLOR_B, COLOR_C, COLOR_D, COLOR_E, COLOR_F};
    panic(codes[which & 0xF]);
}

__attribute__((noinline)) bool overlaps(uint64_t as, uint64_t ae, uint64_t bs, uint64_t be)
{
    if(as <= bs && bs < ae)
        return true;
    if(bs <= as && as < be)
        return true;
    return false;
}

uintptr_t get_iram_address_for_debug(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_DEBUG_IRAM);
}
