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

#include "boot_functions.hpp"

static constexpr u32 ExpectedPlluDivP = (1 << 16);
static constexpr u32 ExpectedPlluDivN = (25 << 8);
static constexpr u32 ExpectedPlluDivM = (2  << 0);
static constexpr u32 ExpectedPlluVal  = (ExpectedPlluDivP | ExpectedPlluDivN | ExpectedPlluDivM);
static constexpr u32 ExpectedPlluMask = 0x1FFFFF;

static constexpr u32 ExpectedUtmipDivN = (25 << 16);
static constexpr u32 ExpectedUtmipDivM = (1 << 8);
static constexpr u32 ExpectedUtmipVal  = (ExpectedUtmipDivN | ExpectedUtmipDivM);
static constexpr u32 ExpectedUtmipMask = 0xFFFF00;

static bool IsUsbClockValid() {
    u64 _vaddr;
    if (R_FAILED(svcQueryIoMapping(&_vaddr, 0x60006000ul, 0x1000))) {
        std::abort();
    }
    volatile u32 *car_regs = reinterpret_cast<volatile u32 *>(_vaddr);

    const u32 pllu = car_regs[0xC0 >> 2];
    const u32 utmip = car_regs[0x480 >> 2];
    return ((pllu & ExpectedPlluMask) == ExpectedPlluVal) && ((utmip & ExpectedUtmipMask) == ExpectedUtmipVal);
}

void Boot::CheckClock() {
    if (!IsUsbClockValid()) {
        /* Sleep for 1s, then reboot. */
        svcSleepThread(1'000'000'000ul);
        Boot::RebootSystem();
    }
}
