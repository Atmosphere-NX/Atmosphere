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

namespace ams::secmon {

    namespace {

        constexpr inline uintptr_t PMC = MemoryRegionVirtualDevicePmc.GetAddress();

        constinit std::atomic_bool g_is_locked = false;

    }

    void ExceptionHandlerImpl(uintptr_t lr, uintptr_t sp) {
        /* On release config, we won't actually use the passed parameters. */
        AMS_UNUSED(lr, sp);

        /* Ensure that previous logs have been flushed. */
        AMS_LOG_FLUSH();

        /* Get system registers. */
        uintptr_t far_el1, far_el3, elr_el3;
        util::BitPack32 esr_el3;

        HW_CPU_GET_FAR_EL1(far_el1);
        HW_CPU_GET_FAR_EL3(far_el3);
        HW_CPU_GET_ELR_EL3(elr_el3);
        HW_CPU_GET_ESR_EL3(esr_el3);

        /* Print some whitespace before the exception handler. */
        AMS_LOG("\n\n");
        AMS_SECMON_LOG("ExceptionHandler\n");
        AMS_SECMON_LOG("----------------\n");
        AMS_SECMON_LOG("esr:     0x%08X\n", esr_el3.value);
        AMS_SECMON_LOG("    Exception Class:               0x%02X\n", esr_el3.Get<hw::EsrEl3::Ec>());
        AMS_SECMON_LOG("    Instruction Length:            %d\n", esr_el3.Get<hw::EsrEl3::Il>() ? 32 : 16);
        AMS_SECMON_LOG("    Instruction Specific Syndrome: 0x%07X\n", esr_el3.Get<hw::EsrEl3::Iss>());

        AMS_SECMON_LOG("far_el1: 0x%016lX\n", far_el1);
        AMS_SECMON_LOG("far_el3: 0x%016lX\n", far_el3);
        AMS_SECMON_LOG("elr_el3: 0x%016lX\n", elr_el3);

        AMS_SECMON_LOG("lr:      0x%016lX\n", lr);
        AMS_SECMON_LOG("sp:      0x%016lX\n", sp);

        AMS_DUMP(reinterpret_cast<void *>(sp), util::AlignUp(sp, mmu::PageSize) - sp);

        AMS_LOG_FLUSH();
    }

    NORETURN void ExceptionHandler() {
        /* Get link register and stack pointer. */
        u64 lr, sp;
        {
            __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
            __asm__ __volatile__("mov %0, sp" : "=r"(sp) :: "memory");
        }

        /* Acquire exclusive access to exception handling logic. */
        if (!g_is_locked.exchange(true)) {
            /* Invoke the exception handler impl. */
            ExceptionHandlerImpl(lr, sp);

            /* Lockout the security engine. */
            se::Lockout();

            /* Lockout fuses. */
            fuse::Lockout();

            /* Disable crypto operations after reboot. */
            reg::Write(PMC + APBDEV_PMC_CRYPTO_OP, 0);

            /* Perform an error reboot. */
            secmon::SetError(pkg1::ErrorInfo_UnknownAbort);
            secmon::ErrorReboot();
        } else {
            /* Wait forever while the first core prints the exception and reboots. */
            while (true) {
                util::WaitMicroSeconds(1000);
            }
        }
    }

}