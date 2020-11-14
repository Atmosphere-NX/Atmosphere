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
#include <stratosphere.hpp>
#include "boot_check_clock.hpp"
#include "boot_power_utils.hpp"

namespace ams::boot {

    namespace {

        /* Convenience definitions. */
        constexpr u32 ExpectedPlluDivP = (1 << 16);
        constexpr u32 ExpectedPlluDivN = (25 << 8);
        constexpr u32 ExpectedPlluDivM = (2  << 0);
        constexpr u32 ExpectedPlluVal  = (ExpectedPlluDivP | ExpectedPlluDivN | ExpectedPlluDivM);
        constexpr u32 ExpectedPlluMask = 0x1FFFFF;

        constexpr u32 ExpectedUtmipDivN = (25 << 16);
        constexpr u32 ExpectedUtmipDivM = (1 << 8);
        constexpr u32 ExpectedUtmipVal  = (ExpectedUtmipDivN | ExpectedUtmipDivM);
        constexpr u32 ExpectedUtmipMask = 0xFFFF00;

        /* Helpers. */
        bool IsUsbClockValid() {
            uintptr_t car_regs = dd::QueryIoMapping(0x60006000ul, os::MemoryPageSize);
            AMS_ASSERT(car_regs != 0);

            const u32 pllu = reg::Read(car_regs + 0xC0);
            const u32 utmip = reg::Read(car_regs + 0x480);
            return ((pllu & ExpectedPlluMask) == ExpectedPlluVal) && ((utmip & ExpectedUtmipMask) == ExpectedUtmipVal);
        }

    }

    void CheckClock() {
        if (!IsUsbClockValid()) {
            /* Sleep for 1s, then reboot. */
            os::SleepThread(TimeSpan::FromSeconds(1));
            RebootSystem();
        }
    }

}
