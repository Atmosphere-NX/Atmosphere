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
#pragma once
#include <vapours.hpp>

namespace ams::pmic {

    enum Regulator {
        /* Erista regulators. */
        Regulator_Erista_Max77621   = 0, /* Device code 0x3A000001 */

        /* Mariko regulators. */
        Regulator_Mariko_Max77812_A = 1, /* Device code 0x3A000002 */
        Regulator_Mariko_Max77812_B = 2, /* Device code 0x3A000006 */
    };

    void SetEnBit(Regulator regulator);
    void EnableVddCpu(Regulator regulator);
    void DisableVddCpu(Regulator regulator);
    void EnableSleep();
    void PowerOff();
    void ShutdownSystem(bool reboot);
    bool IsAcOk();
    bool IsPowerButtonPressed();

}