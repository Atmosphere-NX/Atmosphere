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

namespace ams::i2c {

    constexpr inline size_t CommandListLengthMax          = 0x100;
    constexpr inline size_t CommandListReceiveCommandSize = 2;
    constexpr inline size_t CommandListSendCommandSize    = 2;
    constexpr inline size_t CommandListSleepCommandSize   = 2;

    class CommandListFormatter {
        NON_COPYABLE(CommandListFormatter);
        NON_MOVEABLE(CommandListFormatter);
        private:
            size_t current_index;
            size_t command_list_length;
            void *command_list;
        private:
            Result IsEnqueueAble(size_t sz) const;
        public:
            CommandListFormatter(void *p, size_t sz) : current_index(0), command_list_length(sz), command_list(p) {
                AMS_ABORT_UNLESS(this->command_list_length <= CommandListLengthMax);
            }

            ~CommandListFormatter() { this->command_list = nullptr; }

            size_t GetCurrentLength() const { return this->current_index; }
            const void *GetListHead() const { return this->command_list; }

            Result EnqueueReceiveCommand(i2c::TransactionOption option, size_t size);
            Result EnqueueSendCommand(i2c::TransactionOption option, const void *src, size_t size);
            Result EnqueueSleepCommand(int us);
    };

}
