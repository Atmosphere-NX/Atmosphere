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

namespace ams::wdt {

    namespace {

        volatile uintptr_t g_register_address = secmon::MemoryRegionPhysicalDeviceTimer.GetAddress();

        #if defined(ATMOSPHERE_ARCH_ARM64)
            NOINLINE void Reboot(uintptr_t registers) {
                __asm__ __volatile__(
                    /* Get the current core. */
                    "mrs  x12, mpidr_el1\n"
                    "and  x12, x12, #0xFF\n"

                    /* Get the offsets of the registers we want to write */
                    "mov  x10, #0x8\n"
                    "mov  x11, #0x20\n"
                    "madd x10, x10, x12, %[registers]\n"
                    "madd x11, x11, x12, %[registers]\n"
                    "add  x10, x10, #0x60\n"
                    "add  x11, x11, #0x100\n"

                    /* Write the magic unlock pattern. */
                    "mov  w9, #0xC45A\n"
                    "str  w9, [x11, #0xC]\n"

                    /* Disable the counters. */
                    "mov  w9, #0x2\n"
                    "str  w9, [x11, #0x8]\n"

                    /* Start periodic timer. */
                    "mov  w9, #0xC0000000\n"
                    "str  w9, [x10]\n"

                    /* Set reboot source to the timer we started. */
                    "mov  w9, #0x8015\n"
                    "add  w9, w9, w12\n"
                    "str  w9, [x11]\n"

                    /* Enable the counters. */
                    "mov  w9, #0x1\n"
                    "str  w9, [x11, #0x8]\n"

                    /* Wait forever. */
                    "1: b 1b"
                    : [registers]"=&r"(registers)
                    :
                    : "x9", "x10", "x11", "x12", "memory"
                );
            }
        #elif defined(ATMOSPHERE_ARCH_ARM)
            NOINLINE void Reboot(uintptr_t registers) {
                /* Write the magic unlock pattern. */
                reg::Write(registers + 0x18C, 0xC45A);

                /* Disable the counters. */
                reg::Write(registers + 0x188, 0x2);

                /* Start periodic timer. */
                reg::Write(registers + 0x080, 0xC0000000);

                /* Set reboot source to the timer we started. */
                reg::Write(registers + 0x180, 0x8019);

                /* Enable the counters. */
                reg::Write(registers + 0x188, 0x1);

                /* Wait forever until the reboot takes. */
                AMS_INFINITE_LOOP();
            }
        #endif

    }

    void SetRegisterAddress(uintptr_t address) {
        g_register_address = address;
    }

    NOINLINE void Reboot() {
        const uintptr_t registers = g_register_address;
        Reboot(registers);
    }

}