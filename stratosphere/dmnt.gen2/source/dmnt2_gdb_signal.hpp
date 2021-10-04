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
#include <stratosphere.hpp>

namespace ams::dmnt {

    enum GdbSignal {
        GdbSignal_Signal0             =  0,
        GdbSignal_Interrupt           =  2,
        GdbSignal_IllegalInstruction  =  4,
        GdbSignal_BreakpointTrap      =  5,
        GdbSignal_EmulationTrap       =  7,
        GdbSignal_ArithmeticException =  8,
        GdbSignal_Killed              =  9,
        GdbSignal_BusError            = 10,
        GdbSignal_SegmentationFault   = 11,
        GdbSignal_BadSystemCall       = 12,
    };

}