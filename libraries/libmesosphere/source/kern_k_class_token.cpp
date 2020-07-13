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

    /* Ensure that the absolute token values are correct. */
    static_assert(ClassToken<KAutoObject>              == 0b00000000'00000000);
    static_assert(ClassToken<KSynchronizationObject>   == 0b00000000'00000001);
    static_assert(ClassToken<KReadableEvent>           == 0b00000000'00000011);
    static_assert(ClassToken<KInterruptEvent>          == 0b00000111'00000011);
    static_assert(ClassToken<KDebug>                   == 0b00001011'00000001);
    static_assert(ClassToken<KThread>                  == 0b00010011'00000001);
    static_assert(ClassToken<KServerPort>              == 0b00100011'00000001);
    static_assert(ClassToken<KServerSession>           == 0b01000011'00000001);
    static_assert(ClassToken<KClientPort>              == 0b10000011'00000001);
    static_assert(ClassToken<KClientSession>           == 0b00001101'00000000);
    static_assert(ClassToken<KProcess>                 == 0b00010101'00000001);
    static_assert(ClassToken<KResourceLimit>           == 0b00100101'00000000);
    static_assert(ClassToken<KLightSession>            == 0b01000101'00000000);
    static_assert(ClassToken<KPort>                    == 0b10000101'00000000);
    static_assert(ClassToken<KSession>                 == 0b00011001'00000000);
    static_assert(ClassToken<KSharedMemory>            == 0b00101001'00000000);
    static_assert(ClassToken<KEvent>                   == 0b01001001'00000000);
    static_assert(ClassToken<KWritableEvent>           == 0b10001001'00000000);
    static_assert(ClassToken<KLightClientSession>      == 0b00110001'00000000);
    static_assert(ClassToken<KLightServerSession>      == 0b01010001'00000000);
    static_assert(ClassToken<KTransferMemory>          == 0b10010001'00000000);
    static_assert(ClassToken<KDeviceAddressSpace>      == 0b01100001'00000000);
    static_assert(ClassToken<KSessionRequest>          == 0b10100001'00000000);
    static_assert(ClassToken<KCodeMemory>              == 0b11000001'00000000);

    /* Ensure that the token hierarchy is correct. */

    /* Base classes */
    static_assert(ClassToken<KAutoObject>              == (0b00000000));
    static_assert(ClassToken<KSynchronizationObject>   == (0b00000001 | ClassToken<KAutoObject>));
    static_assert(ClassToken<KReadableEvent>           == (0b00000010 | ClassToken<KSynchronizationObject>));

    /* Final classes */
    static_assert(ClassToken<KInterruptEvent>          == ((0b00000111 << 8) | ClassToken<KReadableEvent>));
    static_assert(ClassToken<KDebug>                   == ((0b00001011 << 8) | ClassToken<KSynchronizationObject>));
    static_assert(ClassToken<KThread>                  == ((0b00010011 << 8) | ClassToken<KSynchronizationObject>));
    static_assert(ClassToken<KServerPort>              == ((0b00100011 << 8) | ClassToken<KSynchronizationObject>));
    static_assert(ClassToken<KServerSession>           == ((0b01000011 << 8) | ClassToken<KSynchronizationObject>));
    static_assert(ClassToken<KClientPort>              == ((0b10000011 << 8) | ClassToken<KSynchronizationObject>));
    static_assert(ClassToken<KClientSession>           == ((0b00001101 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KProcess>                 == ((0b00010101 << 8) | ClassToken<KSynchronizationObject>));
    static_assert(ClassToken<KResourceLimit>           == ((0b00100101 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KLightSession>            == ((0b01000101 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KPort>                    == ((0b10000101 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KSession>                 == ((0b00011001 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KSharedMemory>            == ((0b00101001 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KEvent>                   == ((0b01001001 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KWritableEvent>           == ((0b10001001 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KLightClientSession>      == ((0b00110001 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KLightServerSession>      == ((0b01010001 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KTransferMemory>          == ((0b10010001 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KDeviceAddressSpace>      == ((0b01100001 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KSessionRequest>          == ((0b10100001 << 8) | ClassToken<KAutoObject>));
    static_assert(ClassToken<KCodeMemory>              == ((0b11000001 << 8) | ClassToken<KAutoObject>));

    /* Ensure that the token hierarchy reflects the class hierarchy. */

    /* Base classes. */
    static_assert(!std::is_final<KSynchronizationObject>::value && std::is_base_of<KAutoObject, KSynchronizationObject>::value);
    static_assert(!std::is_final<KReadableEvent>::value         && std::is_base_of<KSynchronizationObject, KReadableEvent>::value);

    /* Final classes */
    static_assert(std::is_final<KInterruptEvent>::value           && std::is_base_of<KReadableEvent, KInterruptEvent>::value);
    static_assert(std::is_final<KDebug>::value                    && std::is_base_of<KSynchronizationObject, KDebug>::value);
    static_assert(std::is_final<KThread>::value                   && std::is_base_of<KSynchronizationObject, KThread>::value);
    static_assert(std::is_final<KServerPort>::value               && std::is_base_of<KSynchronizationObject, KServerPort>::value);
    static_assert(std::is_final<KServerSession>::value            && std::is_base_of<KSynchronizationObject, KServerSession>::value);
    static_assert(std::is_final<KClientPort>::value               && std::is_base_of<KSynchronizationObject, KClientPort>::value);
    static_assert(std::is_final<KClientSession>::value            && std::is_base_of<KAutoObject, KClientSession>::value);
    static_assert(std::is_final<KProcess>::value                  && std::is_base_of<KSynchronizationObject, KProcess>::value);
    static_assert(std::is_final<KResourceLimit>::value            && std::is_base_of<KAutoObject, KResourceLimit>::value);
    static_assert(std::is_final<KLightSession>::value             && std::is_base_of<KAutoObject, KLightSession>::value);
    static_assert(std::is_final<KPort>::value                     && std::is_base_of<KAutoObject, KPort>::value);
    static_assert(std::is_final<KSession>::value                  && std::is_base_of<KAutoObject, KSession>::value);
    static_assert(std::is_final<KSharedMemory>::value             && std::is_base_of<KAutoObject, KSharedMemory>::value);
    static_assert(std::is_final<KEvent>::value                    && std::is_base_of<KAutoObject, KEvent>::value);
    static_assert(std::is_final<KWritableEvent>::value            && std::is_base_of<KAutoObject, KWritableEvent>::value);
    static_assert(std::is_final<KLightClientSession>::value       && std::is_base_of<KAutoObject, KLightClientSession>::value);
    static_assert(std::is_final<KLightServerSession>::value       && std::is_base_of<KAutoObject, KLightServerSession>::value);
    static_assert(std::is_final<KTransferMemory>::value           && std::is_base_of<KAutoObject, KTransferMemory>::value);
    static_assert(std::is_final<KDeviceAddressSpace>::value       && std::is_base_of<KAutoObject, KDeviceAddressSpace>::value);
    static_assert(std::is_final<KSessionRequest>::value           && std::is_base_of<KAutoObject, KSessionRequest>::value);
    static_assert(std::is_final<KCodeMemory>::value               && std::is_base_of<KAutoObject, KCodeMemory>::value);


}
