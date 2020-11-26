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
#include <vapours/prfile2/prfile2_handle.hpp>
#include <vapours/prfile2/pdm/prfile2_pdm_common.hpp>

namespace ams::prfile2::pdm {

    struct Partition {
        u32 status;
        HandleType disk_handle;
        u32 signature;
        u16 partition_id;
        u16 open_count;
        Partition *lock_handle;
        u32 start_sector;
        u32 total_sector;
        u32 mbr_sector;
        u8 partition_type;
        pdm::Error last_driver_error;
        void *volume;
        volatile NonBlockingProtocolType nbc_detect;
        PartitionEventCallback event_callback;
        void *event_callback_param;

        template<size_t Ix>
        constexpr ALWAYS_INLINE bool GetStatusBit() const {
            constexpr u32 Mask = (1u << Ix);
            return (this->status & Mask) != 0;
        }

        template<size_t Ix>
        constexpr ALWAYS_INLINE void SetStatusBit(bool en) {
            constexpr u32 Mask = (1u << Ix);
            if (en) {
                this->status |= Mask;
            } else {
                this->status &= ~Mask;
            }
        }

        constexpr bool IsOpen() const { return this->GetStatusBit<0>(); }
        constexpr void SetOpen(bool en) { this->SetStatusBit<0>(en); }

        constexpr bool IsLocked() const { return this->GetStatusBit<1>(); }
        constexpr void SetLocked(bool en) { this->SetStatusBit<1>(en); }
    };

    namespace part {

        pdm::Error OpenPartition(HandleType disk_handle, u16 partition_id, HandleType *out);
        pdm::Error ClosePartition(HandleType part_handle);

        void SetDriverErrorCode(HandleType part_handle, pdm::Error err);

        void CheckPartitionOpen(HandleType disk_handle, bool *out);
        void NotifyMediaEvent(HandleType disk_handle, u32 event);

    }

}
