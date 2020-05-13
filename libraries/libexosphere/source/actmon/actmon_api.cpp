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
#include "actmon_registers.hpp"

namespace ams::actmon {

    namespace {

        constinit uintptr_t g_register_address = secmon::MemoryRegionPhysicalDeviceActivityMonitor.GetAddress();

        constinit InterruptHandler g_interrupt_handler = nullptr;

    }

    void SetRegisterAddress(uintptr_t address) {
        g_register_address = address;
    }

    void HandleInterrupt() {
        /* Get the registers. */
        const uintptr_t ACTMON = g_register_address;

        /* Disable the actmon interrupt. */
        reg::Write(ACTMON + ACTMON_COP_CTRL, ACTMON_REG_BITS_ENUM(COP_CTRL_ENB, DISABLE));

        /* Update the interrupt status. */
        reg::Write(ACTMON + ACTMON_COP_INTR_STATUS, reg::Read(ACTMON + ACTMON_COP_INTR_STATUS));

        /* Invoke the handler. */
        if (g_interrupt_handler != nullptr) {
            g_interrupt_handler();
            g_interrupt_handler = nullptr;
        }
    }

    void StartMonitoringBpmp(InterruptHandler handler) {
        /* Get the registers. */
        const uintptr_t ACTMON = g_register_address;

        /* Configure the activity monitor to poll once per microsecond. */
        reg::Write(ACTMON + ACTMON_GLB_PERIOD_CTRL, ACTMON_REG_BITS_ENUM (GLB_PERIOD_CTRL_SOURCE, USEC),
                                                    ACTMON_REG_BITS_VALUE(GLB_PERIOD_CTRL_SAMPLE_PERIOD, 0));

        /* Configure the activity monitor to generate an interrupt the first time the event occurs. */
        reg::Write(ACTMON + ACTMON_COP_UPPER_WMARK, 0);

        /* Set the interrupt handler. */
        g_interrupt_handler = handler;

        /* Configure the activity monitor to generate events whenever the bpmp is woken up. */
        reg::Write(ACTMON + ACTMON_COP_CTRL, ACTMON_REG_BITS_ENUM (COP_CTRL_ENB,                         ENABLE),
                                             ACTMON_REG_BITS_ENUM (COP_CTRL_CONSECUTIVE_ABOVE_WMARK_EN,  ENABLE),
                                             ACTMON_REG_BITS_ENUM (COP_CTRL_CONSECUTIVE_BELOW_WMARK_EN, DISABLE),
                                             ACTMON_REG_BITS_VALUE(COP_CTRL_ABOVE_WMARK_NUM,                  0),
                                             ACTMON_REG_BITS_VALUE(COP_CTRL_BELOW_WMARK_NUM,                  0),
                                             ACTMON_REG_BITS_ENUM (COP_CTRL_WHEN_OVERFLOW_EN,           DISABLE),
                                             ACTMON_REG_BITS_ENUM (COP_CTRL_AVG_ABOVE_WMARK_EN,         DISABLE),
                                             ACTMON_REG_BITS_ENUM (COP_CTRL_AVG_BELOW_WMARK_EN,         DISABLE),
                                             ACTMON_REG_BITS_ENUM (COP_CTRL_AT_END_EN,                  DISABLE),
                                             ACTMON_REG_BITS_ENUM (COP_CTRL_ENB_PERIODIC,                ENABLE));

        /* Read the activity monitor control register to make sure our configuration takes. */
        reg::Read(ACTMON + ACTMON_COP_CTRL);
    }

    void StopMonitoringBpmp() {
        /* Get the registers. */
        const uintptr_t ACTMON = g_register_address;

        /* Disable the actmon interrupt. */
        reg::Write(ACTMON + ACTMON_COP_CTRL, ACTMON_REG_BITS_ENUM(COP_CTRL_ENB, DISABLE));

        /* Update the interrupt status. */
        reg::Write(ACTMON + ACTMON_COP_INTR_STATUS, reg::Read(ACTMON + ACTMON_COP_INTR_STATUS));

        /* Clear the interrupt handler. */
        g_interrupt_handler = nullptr;
    }

}