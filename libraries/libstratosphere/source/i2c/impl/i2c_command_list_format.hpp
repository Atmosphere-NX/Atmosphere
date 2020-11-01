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
#include <stratosphere.hpp>

namespace ams::i2c::impl {

    enum CommandId {
        CommandId_Send      = 0,
        CommandId_Receive   = 1,
        CommandId_Extension = 2,
        CommandId_Count     = 3,
    };

    enum SubCommandId {
        SubCommandId_Sleep = 0,
    };

    struct CommonCommandFormat {
        using CommandId    = util::BitPack8::Field<0, 2>;
        using SubCommandId = util::BitPack8::Field<2, 6>;
    };

    struct ReceiveCommandFormat {
        using StartCondition = util::BitPack8::Field<6, 1, bool>;
        using StopCondition  = util::BitPack8::Field<7, 1, bool>;
        using Size           = util::BitPack8::Field<0, 8>;
    };

    struct SendCommandFormat {
        using StartCondition = util::BitPack8::Field<6, 1, bool>;
        using StopCondition  = util::BitPack8::Field<7, 1, bool>;
        using Size           = util::BitPack8::Field<0, 8>;
    };

    struct SleepCommandFormat {
        using MicroSeconds = util::BitPack8::Field<0, 8>;
    };

}
