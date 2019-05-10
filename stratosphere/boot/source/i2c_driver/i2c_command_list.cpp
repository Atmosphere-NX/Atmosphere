/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <switch.h>
#include <stratosphere.hpp>

#include "i2c_command_list.hpp"

Result I2cCommandListFormatter::CanEnqueue(size_t size) const {
    if (this->cmd_list_size - this->cur_index < size) {
        return ResultI2cFullCommandList;
    }
    return ResultSuccess;
}

Result I2cCommandListFormatter::EnqueueSendCommand(I2cTransactionOption option, const void *src, size_t size) {
    Result rc = this->CanEnqueue(SendCommandSize + size);
    if (R_FAILED(rc)) { return rc; }

    this->cmd_list[this->cur_index] = I2cCommand_Send;
    this->cmd_list[this->cur_index] |= ((option & I2cTransactionOption_Start) != 0) << 6;
    this->cmd_list[this->cur_index] |= ((option & I2cTransactionOption_Stop) != 0) << 7;
    this->cur_index++;

    this->cmd_list[this->cur_index++] = size;

    const u8 *src_u8 = reinterpret_cast<const u8 *>(src);
    for (size_t i = 0; i < size; i++) {
        this->cmd_list[this->cur_index++] = src_u8[i];
    }
    return ResultSuccess;
}

Result I2cCommandListFormatter::EnqueueReceiveCommand(I2cTransactionOption option, size_t size) {
    Result rc = this->CanEnqueue(ReceiveCommandSize);
    if (R_FAILED(rc)) { return rc; }

    this->cmd_list[this->cur_index] = I2cCommand_Receive;
    this->cmd_list[this->cur_index] |= ((option & I2cTransactionOption_Start) != 0) << 6;
    this->cmd_list[this->cur_index] |= ((option & I2cTransactionOption_Stop) != 0) << 7;
    this->cur_index++;

    this->cmd_list[this->cur_index++] = size;
    return ResultSuccess;
}

Result I2cCommandListFormatter::EnqueueSleepCommand(size_t us) {
    Result rc = this->CanEnqueue(SleepCommandSize);
    if (R_FAILED(rc)) { return rc; }

    this->cmd_list[this->cur_index] = I2cCommand_SubCommand;
    this->cmd_list[this->cur_index] |= I2cSubCommand_Sleep << 2;
    this->cur_index++;

    this->cmd_list[this->cur_index++] = us;
    return ResultSuccess;
}
