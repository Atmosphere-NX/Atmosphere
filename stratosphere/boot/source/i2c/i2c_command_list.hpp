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
#include "i2c_types.hpp"

namespace ams::i2c {

    enum class Command {
        Send = 0,
        Receive = 1,
        SubCommand = 2,
        Count,
    };

    enum class SubCommand {
        Sleep = 0,
        Count,
    };

    class CommandListFormatter {
        public:
            static constexpr size_t MaxCommandListSize = 0x100;
        private:
            u8 *cmd_list = nullptr;
            size_t cmd_list_size = 0;
            size_t cur_index = 0;
        public:
            CommandListFormatter(void *cmd_list, size_t cmd_list_size) : cmd_list(static_cast<u8 *>(cmd_list)), cmd_list_size(cmd_list_size) {
                AMS_ABORT_UNLESS(cmd_list_size <= MaxCommandListSize);
            }
            ~CommandListFormatter() {
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

}
