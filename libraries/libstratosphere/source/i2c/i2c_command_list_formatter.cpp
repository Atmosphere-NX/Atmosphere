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
#include <stratosphere.hpp>
#include "impl/i2c_command_list_format.hpp"

namespace ams::i2c {

    Result CommandListFormatter::IsEnqueueAble(size_t sz) const {
        R_UNLESS(this->command_list_length - this->current_index >= sz, ResultCommandListFull());
        return ResultSuccess();
    }

    Result CommandListFormatter::EnqueueReceiveCommand(i2c::TransactionOption option, size_t size) {
        /* Check that we can enqueue the command. */
        constexpr size_t CommandLength = 2;
        R_TRY(this->IsEnqueueAble(CommandLength));

        /* Get the command list. */
        util::BitPack8 *cmd_list = static_cast<util::BitPack8 *>(this->command_list);

        /* Get references to the header. */
        auto &header0 = cmd_list[this->current_index++];
        auto &header1 = cmd_list[this->current_index++];

        /* Set the header. */
        header0.Set<impl::CommonCommandFormat::CommandId>(impl::CommandId_Receive);
        header0.Set<impl::ReceiveCommandFormat::StopCondition>((option & TransactionOption_StopCondition) != 0);
        header0.Set<impl::ReceiveCommandFormat::StartCondition>((option & TransactionOption_StartCondition) != 0);

        header1.Set<impl::ReceiveCommandFormat::Size>(size);

        return ResultSuccess();
    }

    Result CommandListFormatter::EnqueueSendCommand(i2c::TransactionOption option, const void *src, size_t size) {
        /* Check that we can enqueue the command. */
        constexpr size_t CommandLength = 2;
        R_TRY(this->IsEnqueueAble(CommandLength + size));

        /* Get the command list. */
        util::BitPack8 *cmd_list = static_cast<util::BitPack8 *>(this->command_list);

        /* Get references to the header. */
        auto &header0 = cmd_list[this->current_index++];
        auto &header1 = cmd_list[this->current_index++];

        /* Set the header. */
        header0.Set<impl::CommonCommandFormat::CommandId>(impl::CommandId_Send);
        header0.Set<impl::SendCommandFormat::StopCondition>((option & TransactionOption_StopCondition) != 0);
        header0.Set<impl::SendCommandFormat::StartCondition>((option & TransactionOption_StartCondition) != 0);

        header1.Set<impl::SendCommandFormat::Size>(size);

        /* Copy the data we're sending. */
        std::memcpy(cmd_list + this->current_index, src, size);
        this->current_index += size;

        return ResultSuccess();
    }

    Result CommandListFormatter::EnqueueSleepCommand(int us) {
        /* Check that we can enqueue the command. */
        constexpr size_t CommandLength = 2;
        R_TRY(this->IsEnqueueAble(CommandLength));

        /* Get the command list. */
        util::BitPack8 *cmd_list = static_cast<util::BitPack8 *>(this->command_list);

        /* Get references to the header. */
        auto &header0 = cmd_list[this->current_index++];
        auto &header1 = cmd_list[this->current_index++];

        /* Set the header. */
        header0.Set<impl::CommonCommandFormat::CommandId>(impl::CommandId_Extension);
        header0.Set<impl::CommonCommandFormat::SubCommandId>(impl::SubCommandId_Sleep);

        header1.Set<impl::SleepCommandFormat::MicroSeconds>(us);

        return ResultSuccess();
    }

}
