/*
 * Copyright (c) Atmosphère-NX
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
#pragma once
#include <vapours.hpp>

namespace ams::clkrst {

    void SetRegisterAddress(uintptr_t address);

    void SetFuseVisibility(bool visible);

    void EnableUartAClock();
    void EnableUartBClock();
    void EnableUartCClock();
    void EnableActmonClock();
    void EnableI2c1Clock();
    void EnableI2c5Clock();

    void EnableSeClock();
    void EnableCldvfsClock();
    void EnableCsiteClock();
    void EnableTzramClock();

    void EnableCache2Clock();
    void EnableCram2Clock();

    void EnableHost1xClock();
    void EnableTsecClock();
    void EnableSorSafeClock();
    void EnableSor0Clock();
    void EnableSor1Clock();
    void EnableKfuseClock();

    void DisableI2c1Clock();

    void DisableHost1xClock();
    void DisableTsecClock();
    void DisableSorSafeClock();
    void DisableSor0Clock();
    void DisableSor1Clock();
    void DisableKfuseClock();


    enum BpmpClockRate {
        BpmpClockRate_408MHz,
        BpmpClockRate_544MHz,
        BpmpClockRate_576MHz,
        BpmpClockRate_589MHz,

        BpmpClockRate_Count,
    };

    BpmpClockRate GetBpmpClockRate();
    BpmpClockRate SetBpmpClockRate(BpmpClockRate rate);

}