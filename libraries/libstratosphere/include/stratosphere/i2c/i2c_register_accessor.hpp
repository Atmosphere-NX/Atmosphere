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
#include <stratosphere/i2c/i2c_types.hpp>
#include <stratosphere/i2c/i2c_command_list_formatter.hpp>
#include <stratosphere/i2c/i2c_bus_api.hpp>

namespace ams::i2c {

    template<typename RegType> requires std::unsigned_integral<RegType>
    Result ReadSingleRegister(const I2cSession &session, u8 address, RegType *out) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(out != nullptr);

        constexpr i2c::TransactionOption StartOption = i2c::TransactionOption_StartCondition;
        constexpr i2c::TransactionOption StopOption  = static_cast<i2c::TransactionOption>(i2c::TransactionOption_StartCondition | i2c::TransactionOption_StopCondition);

        u8 cmd_list[CommandListLengthMax];
        i2c::CommandListFormatter formatter(cmd_list, sizeof(cmd_list));

        R_TRY(formatter.EnqueueSendCommand(StartOption, std::addressof(address), sizeof(address)));
        R_TRY(formatter.EnqueueReceiveCommand(StopOption, sizeof(*out)));

        R_TRY(i2c::ExecuteCommandList(out, sizeof(*out), session, cmd_list, formatter.GetCurrentLength()));

        return ResultSuccess();
    }

    template<typename RegType> requires std::unsigned_integral<RegType>
    Result WriteSingleRegister(const I2cSession &session, u8 address, RegType value) {
        /* Prepare buffer. */
        u8 buf[sizeof(address) + sizeof(value)];
        std::memcpy(buf + 0, std::addressof(address), sizeof(address));
        std::memcpy(buf + sizeof(address), std::addressof(value), sizeof(value));

        constexpr i2c::TransactionOption StopOption  = static_cast<i2c::TransactionOption>(i2c::TransactionOption_StartCondition | i2c::TransactionOption_StopCondition);
        R_TRY(i2c::Send(session, buf, sizeof(buf), StopOption));

        return ResultSuccess();
    }

}
