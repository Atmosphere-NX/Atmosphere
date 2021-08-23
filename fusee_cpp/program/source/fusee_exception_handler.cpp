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
#include "fusee_exception_handler.hpp"

namespace ams::nxboot {

    NORETURN void ErrorStop() {
        /* ABORT? */
        *reinterpret_cast<volatile u32 *>(0x40038000) = 0xDEADDEAD;
        *reinterpret_cast<volatile u32 *>(0x7000E400) = 0x10;

        /* Halt ourselves. */
        while (true) {
            reg::Write(secmon::MemoryRegionPhysicalDeviceFlowController.GetAddress() + FLOW_CTLR_HALT_COP_EVENTS, FLOW_REG_BITS_ENUM(HALT_COP_EVENTS_MODE, FLOW_MODE_STOP),
                                                                                                                  FLOW_REG_BITS_ENUM(HALT_COP_EVENTS_JTAG,        ENABLED));
        }
    }

    NORETURN void ExceptionHandlerImpl(s32 which, u32 lr, u32 svc_lr) {
        /* TODO */
        ErrorStop();
    }

}

namespace ams::diag {

    NORETURN void AbortImpl(const char *file, int line, const char *func, const char *expr, u64 value, const char *format, ...) {
        AMS_UNUSED(file, line, func, expr, value, format);
        {
            u32 lr;
            __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
            *reinterpret_cast<volatile u32 *>(0x40038004) = lr;
        }
        ams::nxboot::ErrorStop();
    }

    NORETURN void AbortImpl(const char *file, int line, const char *func, const char *expr, u64 value) {
        AMS_UNUSED(file, line, func, expr, value);
        {
            u32 lr;
            __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
            *reinterpret_cast<volatile u32 *>(0x40038004) = lr;
        }
        ams::nxboot::ErrorStop();
    }

    NORETURN void AbortImpl() {
        {
            u32 lr;
            __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
            *reinterpret_cast<volatile u32 *>(0x40038004) = lr;
        }
        ams::nxboot::ErrorStop();
    }

}