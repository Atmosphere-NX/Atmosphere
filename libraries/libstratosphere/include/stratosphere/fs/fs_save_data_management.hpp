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
#include "fs_common.hpp"
#include "fs_save_data_types.hpp"

namespace ams::fs {

    Result DeleteSaveData(SaveDataId id);
    Result DeleteSaveData(SaveDataSpaceId space_id, SaveDataId id);

    Result GetSaveDataFlags(u32 *out, SaveDataId id);
    Result GetSaveDataFlags(u32 *out, SaveDataSpaceId space_id, SaveDataId id);
    Result SetSaveDataFlags(SaveDataId id, SaveDataSpaceId space_id, u32 flags);

    Result GetSaveDataAvailableSize(s64 *out, SaveDataId id);
    Result GetSaveDataJournalSize(s64 *out, SaveDataId id);

    Result ExtendSaveData(SaveDataSpaceId space_id, SaveDataId id, s64 available_size, s64 journal_size);

}
