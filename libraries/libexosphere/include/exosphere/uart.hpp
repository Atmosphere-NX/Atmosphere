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

namespace ams::uart {

    enum Port {
        Port_A = 0,
        Port_B = 1,
        Port_C = 2,

        Port_Count = 3,

        Port_ReservedDebug = Port_A,
        Port_RightJoyCon   = Port_B,
        Port_LeftJoyCon    = Port_C,
    };

    enum Flags {
        Flag_None     = (0u << 0),
        Flag_Inverted = (1u << 0),
    };

    void SetRegisterAddress(uintptr_t address);

    void Initialize(Port port, int baud_rate, u32 flags);

    void SendText(Port port, const void *data, size_t size);

    void WaitFlush(Port port);

}
