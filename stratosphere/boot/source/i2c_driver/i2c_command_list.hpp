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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "i2c_types.hpp"

class I2cCommandListFormatter {
    public:
        static constexpr size_t MaxCommandListSize = 0x100;
        static constexpr size_t SendCommandSize = 2;
        static constexpr size_t ReceiveCommandSize = 2;
        static constexpr size_t SleepCommandSize = 2;
    private:
        u8 *cmd_list = nullptr;
        size_t cmd_list_size = 0;
        size_t cur_index = 0;
    public:
        I2cCommandListFormatter(void *cmd_list, size_t cmd_list_size) : cmd_list(static_cast<u8 *>(cmd_list)), cmd_list_size(cmd_list_size) {
            if (cmd_list_size > MaxCommandListSize) {
                std::abort();
            }
        }
        ~I2cCommandListFormatter() {
            this->cmd_list = nullptr;
        }

    private:
        Result CanEnqueue(size_t size) const;
    public:
        size_t GetCurrentSize() const {
            return this->cur_index;
        }

        Result EnqueueSendCommand(I2cTransactionOption option, const void *src, size_t size);
        Result EnqueueReceiveCommand(I2cTransactionOption option, size_t size);
        Result EnqueueSleepCommand(size_t us);
};
