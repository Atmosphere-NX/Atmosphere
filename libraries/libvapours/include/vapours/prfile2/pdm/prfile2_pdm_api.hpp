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
#include <vapours/prfile2/pdm/prfile2_pdm_types.hpp>
#include <vapours/prfile2/pdm/prfile2_pdm_common.hpp>
#include <vapours/prfile2/pdm/prfile2_pdm_disk_management.hpp>

namespace ams::prfile2::pdm {

    pdm::Error Initialize(u32 config, void *param);

    pdm::Error OpenDisk(InitDisk *init_disk_table, HandleType *out);
    pdm::Error CloseDisk(HandleType handle);

    pdm::Error OpenPartition(HandleType disk_handle, u16 part_id, HandleType *out);
    pdm::Error ClosePartition(HandleType handle);

}
