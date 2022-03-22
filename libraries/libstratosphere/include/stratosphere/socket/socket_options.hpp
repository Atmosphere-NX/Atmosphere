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

namespace ams::socket {

    enum class Level : s32 {
        Sol_Ip      =   0,
        Sol_Icmp    =   1,
        Sol_Tcp     =   6,
        Sol_Udp     =  17,
        Sol_UdpLite = 136,

        Sol_Socket  = 0xFFFF,
    };

    enum class Option : u32 {
        So_Debug     = (1 <<  0),
        /* ... */
        So_ReuseAddr = (1 <<  2),
        /* ... */
    };

}
