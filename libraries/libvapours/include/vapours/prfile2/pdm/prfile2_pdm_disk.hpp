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

    struct Disk {
        u32 status;
        DiskTable disk_table;
        u32 signature;
        u16 open_count;
        u16 lock_count;
        Disk *lock_handle;
        DiskInfo disk_info;
        InitDisk *init_disk_table;
        Partition *current_partition;
        DiskCallback erase_callback;
        bool is_inserted;
        volatile NonBlockingProtocolType nbc;
        volatile NonBlockingProtocolType nbc_detect;
        volatile NonBlockingProtocolType nbc_req;

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

    namespace disk {

        pdm::Error OpenDisk(InitDisk *init_disk_table, HandleType *out);
        pdm::Error CloseDisk(HandleType handle);

        /* ... */

    }

}
