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

#if defined(AMS_SDMMC_USE_OS_EVENTS)
#include <stratosphere/os.hpp>
#endif

namespace ams::sdmmc::impl {

    enum ResponseType {
        ResponseType_R0 = 0,
        ResponseType_R1 = 1,
        ResponseType_R2 = 2,
        ResponseType_R3 = 3,
        ResponseType_R6 = 4,
        ResponseType_R7 = 5,
    };

    enum TransferDirection {
        TransferDirection_ReadFromDevice = 0,
        TransferDirection_WriteToDevice  = 1,
    };

    struct Command {
        u32 command_index;
        u32 command_argument;
        ResponseType response_type;
        bool is_busy;

        constexpr Command(u32 ci, u32 ca, ResponseType r, bool b) : command_index(ci), command_argument(ca), response_type(r), is_busy(b) { /* ... */ }
    };

    struct TransferData {
        void *buffer;
        size_t block_size;
        u32 num_blocks;
        TransferDirection transfer_direction;
        bool is_multi_block_transfer;
        bool is_stop_transmission_command_enabled;

        constexpr TransferData(void *b, size_t bs, u32 nb, TransferDirection xd, bool mb, bool st)
            : buffer(b), block_size(bs),  num_blocks(nb), transfer_direction(xd), is_multi_block_transfer(mb), is_stop_transmission_command_enabled(st)
        {
            if (this->num_blocks > 1) {
                AMS_ABORT_UNLESS(this->is_multi_block_transfer);
            }
        }

        constexpr TransferData(void *b, size_t bs, u32 nb, TransferDirection xd)
            : buffer(b), block_size(bs), num_blocks(nb), transfer_direction(xd), is_multi_block_transfer(false), is_stop_transmission_command_enabled(false)
        {
            AMS_ABORT_UNLESS(this->num_blocks == 1);
        }
    };

    class IHostController {
        public:
            #if defined(AMS_SDMMC_USE_OS_EVENTS)
            virtual void PreSetRemovedEvent(ams::os::EventType *event) = 0;
            #endif

            virtual void Initialize() = 0;
            virtual void Finalize() = 0;

            /* TODO */

            virtual void ChangeCheckTransferInterval(u32 ms) = 0;
            virtual void SetDefaultCheckTransferInterval() = 0;

            /* TODO */
    };

}
