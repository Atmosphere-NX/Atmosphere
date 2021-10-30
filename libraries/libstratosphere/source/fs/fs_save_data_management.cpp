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
#include <stratosphere.hpp>
#include "fsa/fs_mount_utils.hpp"

namespace ams::fs {

    namespace impl {

        Result ReadSaveDataFileSystemExtraData(SaveDataExtraData *out, SaveDataId id) {
            return fsReadSaveDataFileSystemExtraData(out, sizeof(*out), id);
        }

        Result ReadSaveDataFileSystemExtraData(SaveDataExtraData *out, SaveDataSpaceId space_id, SaveDataId id) {
            return fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(out, sizeof(*out), static_cast<::FsSaveDataSpaceId>(space_id), id);
        }

        Result WriteSaveDataFileSystemExtraData(SaveDataSpaceId space_id, SaveDataId id, const SaveDataExtraData &extra_data) {
            return fsWriteSaveDataFileSystemExtraData(std::addressof(extra_data), sizeof(extra_data), static_cast<::FsSaveDataSpaceId>(space_id), id);
        }

    }

    void DisableAutoSaveDataCreation() {
        /* Use libnx binding. */
        R_ABORT_UNLESS(fsDisableAutoSaveDataCreation());
    }

    Result CreateSystemSaveData(SaveDataSpaceId space_id, SystemSaveDataId save_id, UserId user_id, u64 owner_id, s64 size, s64 journal_size, u32 flags) {
        const auto attribute = SaveDataAttribute::Make(ncm::InvalidProgramId, SaveDataType::System, user_id, save_id);
        const SaveDataCreationInfo info = {
            .size         = size,
            .journal_size = journal_size,
            .block_size   = DefaultSaveDataBlockSize,
            .owner_id     = owner_id,
            .flags        = flags,
            .space_id     = space_id,
            .pseudo       = false,
        };

        static_assert(sizeof(SaveDataAttribute) == sizeof(::FsSaveDataAttribute));
        static_assert(sizeof(SaveDataCreationInfo) == sizeof(::FsSaveDataCreationInfo));
        return fsCreateSaveDataFileSystemBySystemSaveDataId(reinterpret_cast<const ::FsSaveDataAttribute *>(std::addressof(attribute)), reinterpret_cast<const ::FsSaveDataCreationInfo *>(std::addressof(info)));
    }

    Result CreateSystemSaveData(SystemSaveDataId save_id, s64 size, s64 journal_size, u32 flags) {
        return CreateSystemSaveData(SaveDataSpaceId::System, save_id, InvalidUserId, 0, size, journal_size, flags);
    }

    Result CreateSystemSaveData(SystemSaveDataId save_id, u64 owner_id, s64 size, s64 journal_size, u32 flags) {
        return CreateSystemSaveData(SaveDataSpaceId::System, save_id, InvalidUserId, owner_id, size, journal_size, flags);
    }

    Result CreateSystemSaveData(SaveDataSpaceId space_id, SystemSaveDataId save_id, u64 owner_id, s64 size, s64 journal_size, u32 flags) {
        return CreateSystemSaveData(space_id, save_id, InvalidUserId, owner_id, size, journal_size, flags);
    }

    Result CreateSystemSaveData(SystemSaveDataId save_id, UserId user_id, s64 size, s64 journal_size, u32 flags) {
        return CreateSystemSaveData(SaveDataSpaceId::System, save_id, user_id, 0, size, journal_size, flags);
    }

    Result CreateSystemSaveData(SystemSaveDataId save_id, UserId user_id, u64 owner_id, s64 size, s64 journal_size, u32 flags) {
        return CreateSystemSaveData(SaveDataSpaceId::System, save_id, user_id, owner_id, size, journal_size, flags);
    }

    Result DeleteSaveData(SaveDataId id) {
        /* TODO: Libnx binding for DeleteSaveDataFileSystem */
        AMS_UNUSED(id);
        AMS_ABORT();
    }

    Result DeleteSaveData(SaveDataSpaceId space_id, SaveDataId id) {
        return fsDeleteSaveDataFileSystemBySaveDataSpaceId(static_cast<::FsSaveDataSpaceId>(space_id), id);
    }

    Result DeleteSystemSaveData(SaveDataSpaceId space_id, SystemSaveDataId id, UserId user_id) {
        const auto attribute = SaveDataAttribute::Make(ncm::InvalidProgramId, SaveDataType::System, user_id, id);

        static_assert(sizeof(attribute) == sizeof(::FsSaveDataAttribute));
        return fsDeleteSaveDataFileSystemBySaveDataAttribute(static_cast<::FsSaveDataSpaceId>(space_id), reinterpret_cast<const ::FsSaveDataAttribute *>(std::addressof(attribute)));
    }

    Result GetSaveDataFlags(u32 *out, SaveDataId id) {
        SaveDataExtraData extra_data;
        R_TRY(impl::ReadSaveDataFileSystemExtraData(std::addressof(extra_data), id));

        *out = extra_data.flags;
        return ResultSuccess();
    }

    Result GetSaveDataFlags(u32 *out, SaveDataSpaceId space_id, SaveDataId id) {
        SaveDataExtraData extra_data;
        R_TRY(impl::ReadSaveDataFileSystemExtraData(std::addressof(extra_data), space_id, id));

        *out = extra_data.flags;
        return ResultSuccess();
    }

    Result SetSaveDataFlags(SaveDataId id, SaveDataSpaceId space_id, u32 flags) {
        SaveDataExtraData extra_data;
        R_TRY(impl::ReadSaveDataFileSystemExtraData(std::addressof(extra_data), space_id, id));
        extra_data.flags = flags;
        return impl::WriteSaveDataFileSystemExtraData(space_id, id, extra_data);
    }

    Result GetSaveDataAvailableSize(s64 *out, SaveDataId id) {
        SaveDataExtraData extra_data;
        R_TRY(impl::ReadSaveDataFileSystemExtraData(std::addressof(extra_data), id));

        *out = extra_data.available_size;
        return ResultSuccess();
    }

    Result GetSaveDataJournalSize(s64 *out, SaveDataId id) {
        SaveDataExtraData extra_data;
        R_TRY(impl::ReadSaveDataFileSystemExtraData(std::addressof(extra_data), id));

        *out = extra_data.journal_size;
        return ResultSuccess();
    }

    Result ExtendSaveData(SaveDataSpaceId space_id, SaveDataId id, s64 available_size, s64 journal_size) {
        return ::fsExtendSaveDataFileSystem(static_cast<::FsSaveDataSpaceId>(space_id), static_cast<u64>(id), available_size, journal_size);
    }

}
