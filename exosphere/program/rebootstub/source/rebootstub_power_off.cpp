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

namespace ams::rebootstub {

    NORETURN void Halt() {
        while (true) {
            reg::Write(secmon::MemoryRegionPhysicalDeviceFlowController.GetAddress() + FLOW_CTLR_HALT_COP_EVENTS, FLOW_REG_BITS_ENUM(HALT_COP_EVENTS_MODE, FLOW_MODE_STOP),
                                                                                                                  FLOW_REG_BITS_ENUM(HALT_COP_EVENTS_JTAG,        ENABLED));
        }

        __builtin_unreachable();
    }

    NORETURN void PowerOff() {
        /* Ensure that i2c5 is usable. */
        clkrst::EnableI2c5Clock();

        /* Initialize i2c5. */
        i2c::Initialize(i2c::Port_5);

        /* Stop rtc alarms. */
        rtc::StopAlarm();

        /* Perform a pmic power off. */
        pmic::PowerOff();

        /* Halt the bpmp. */
        Halt();

        /* This can never be reached. */
        __builtin_unreachable();
    }

}

namespace ams::diag {

    void AbortImpl() {
        /* Halt the bpmp. */
        rebootstub::Halt();

        /* This can never be reached. */
        __builtin_unreachable();
    }

    #include <exosphere/diag/diag_detailed_assertion_impl.inc>

}
