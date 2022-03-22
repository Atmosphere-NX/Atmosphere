/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fs_save_data_types.hpp>

namespace ams::fs {

    void DisableAutoSaveDataCreation();

    Result CreateSystemSaveData(SystemSaveDataId save_id, s64 size, s64 journal_size, u32 flags);
    Result CreateSystemSaveData(SystemSaveDataId save_id, u64 owner_id, s64 size, s64 journal_size, u32 flags);
    Result CreateSystemSaveData(SaveDataSpaceId space_id, SystemSaveDataId save_id, u64 owner_id, s64 size, s64 journal_size, u32 flags);

    Result CreateSystemSaveData(SystemSaveDataId save_id, UserId user_id, s64 size, s64 journal_size, u32 flags);
    Result CreateSystemSaveData(SystemSaveDataId save_id, UserId user_id, u64 owner_id, s64 size, s64 journal_size, u32 flags);
    Result CreateSystemSaveData(SaveDataSpaceId space_id, SystemSaveDataId save_id, UserId user_id, u64 owner_id, s64 size, s64 journal_size, u32 flags);

    Result MountSystemSaveData(const char *name, SystemSaveDataId id);
    Result MountSystemSaveData(const char *name, SaveDataSpaceId space_id, SystemSaveDataId id);

    Result MountSystemSaveData(const char *name, SystemSaveDataId id, UserId user_id);
    Result MountSystemSaveData(const char *name, SaveDataSpaceId space_id, SystemSaveDataId id, UserId user_id);

    Result DeleteSystemSaveData(SaveDataSpaceId space_id, SystemSaveDataId id, UserId user_id);

}
