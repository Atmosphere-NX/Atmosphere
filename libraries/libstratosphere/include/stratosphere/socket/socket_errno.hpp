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

    enum class Errno : u32 {
        ESuccess   =   0,
        /* ... */
        EAgain     =  11,
        ENoMem     =  12,
        /* ... */
        EFault     =  14,
        /* ... */
        EInval     =  22,
        /* ... */
        ENoSpc     =  28,
        /* ... */
        EL3Hlt     =  46,
        /* ... */
        EOpNotSupp =  95,
        ENotSup    =  EOpNotSupp,
    };

    enum class HErrno : s32 {
        Netdb_Internal = -1,
        Netdb_Success  = 0,
        Host_Not_Found = 1,
        Try_Again      = 2,
        No_Recovery    = 3,
        No_Data        = 4,

        No_Address     = No_Data,
    };

    enum class AiErrno : u32 {
        EAi_Success = 0,
        /* ... */
    };

    constexpr inline bool operator!(Errno e)   { return e == Errno::ESuccess; }
    constexpr inline bool operator!(HErrno e)  { return e == HErrno::Netdb_Success; }
    constexpr inline bool operator!(AiErrno e) { return e == AiErrno::EAi_Success; }

}
