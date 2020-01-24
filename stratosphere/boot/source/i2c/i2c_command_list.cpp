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
#include "i2c_types.hpp"
#include "i2c_command_list.hpp"

namespace ams::i2c {

    namespace {

        /* Useful definitions. */
        constexpr size_t SendCommandSize = 2;
        constexpr size_t ReceiveCommandSize = 2;
        constexpr size_t SleepCommandSize = 2;

    }

    Result CommandListFormatter::CanEnqueue(size_t size) const {
        R_UNLESS(this->cmd_list_size - this->cur_index >= size, ResultFullCommandList());
        return ResultSuccess();
    }

    Result CommandListFormatter::EnqueueSendCommand(I2cTransactionOption option, const void *src, size_t size) {
        R_TRY(this->CanEnqueue(SendCommandSize + size));

        this->cmd_list[this->cur_index] = static_cast<u8>(Command::Send);
        this->cmd_list[this->cur_index] |= ((option & I2cTransactionOption_Start) != 0) << 6;
        this->cmd_list[this->cur_index] |= ((option & I2cTransactionOption_Stop) != 0) << 7;
        this->cur_index++;

        this->cmd_list[this->cur_index++] = size;

        const u8 *src_u8 = reinterpret_cast<const u8 *>(src);
        for (size_t i = 0; i < size; i++) {
            this->cmd_list[this->cur_index++] = src_u8[i];
        }
        return ResultSuccess();
    }

    Result CommandListFormatter::EnqueueReceiveCommand(I2cTransactionOption option, size_t size) {
        R_TRY(this->CanEnqueue(ReceiveCommandSize));

        this->cmd_list[this->cur_index] = static_cast<u8>(Command::Receive);
        this->cmd_list[this->cur_index] |= ((option & I2cTransactionOption_Start) != 0) << 6;
        this->cmd_list[this->cur_index] |= ((option & I2cTransactionOption_Stop) != 0) << 7;
        this->cur_index++;

        this->cmd_list[this->cur_index++] = size;
        return ResultSuccess();
    }

    Result CommandListFormatter::EnqueueSleepCommand(size_t us) {
        R_TRY(this->CanEnqueue(SleepCommandSize));

        this->cmd_list[this->cur_index] = static_cast<u8>(Command::SubCommand);
        this->cmd_list[this->cur_index] |= static_cast<u8>(SubCommand::Sleep) << 2;
        this->cur_index++;

        this->cmd_list[this->cur_index++] = us;
        return ResultSuccess();
    }

}


