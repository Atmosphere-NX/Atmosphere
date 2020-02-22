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
#include <mesosphere.hpp>

namespace ams::kern {

    /* Ensure that we generate correct class tokens for all types. */
    static_assert(ClassToken<KAutoObject>              == 0b00000000'00000000);
    static_assert(ClassToken<KSynchronizationObject>   == 0b00000000'00000001);
    static_assert(ClassToken<KReadableEvent>           == 0b00000000'00000011);
    static_assert(ClassToken<KInterruptEvent>          == 0b00000111'00000011);
    static_assert(ClassToken<KDebug>                   == 0b00001011'00000001);
    static_assert(ClassToken<KThread>                  == 0b00010011'00000001);
    /* TODO: static_assert(ClassToken<KServerPort>              == 0b00100011'00000001); */
    /* TODO: static_assert(ClassToken<KServerSession>           == 0b01000011'00000001); */
    /* TODO: static_assert(ClassToken<KClientPort>              == 0b10000011'00000001); */
    /* TODO: static_assert(ClassToken<KClientSession>           == 0b00001101'00000000); */
    static_assert(ClassToken<KProcess>                 == 0b00010101'00000001);
    static_assert(ClassToken<KResourceLimit>           == 0b00100101'00000000);
    static_assert(ClassToken<KLightSession>            == 0b01000101'00000000);
    static_assert(ClassToken<KPort>                    == 0b10000101'00000000);
    static_assert(ClassToken<KSession>                 == 0b00011001'00000000);
    static_assert(ClassToken<KSharedMemory>            == 0b00101001'00000000);
    static_assert(ClassToken<KEvent>                   == 0b01001001'00000000);
    /* TODO: static_assert(ClassToken<KWritableEvent>           == 0b10001001'00000000); */
    /* TODO: static_assert(ClassToken<KLightClientSession>      == 0b00110001'00000000); */
    /* TODO: static_assert(ClassToken<KLightServerSession>      == 0b01010001'00000000); */
    static_assert(ClassToken<KTransferMemory>          == 0b10010001'00000000);
    static_assert(ClassToken<KDeviceAddressSpace>      == 0b01100001'00000000);
    static_assert(ClassToken<KSessionRequest>          == 0b10100001'00000000);
    static_assert(ClassToken<KCodeMemory>              == 0b11000001'00000000);

}
