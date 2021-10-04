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

namespace ams::htclow {

    namespace impl {

        enum class DriverType {
            Unknown      = 0,
            Debug        = 1,
            Socket       = 2,
            Usb          = 3,
            HostBridge   = 4,
            PlainChannel = 5,
        };

    }

    constexpr inline s16 ProtocolVersion = 5;

    enum ReceiveOption {
        ReceiveOption_NonBlocking    = 0,
        ReceiveOption_ReceiveAnyData = 1,
        ReceiveOption_ReceiveAllData = 2,
    };

}
