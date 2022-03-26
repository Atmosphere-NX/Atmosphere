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
#include "impl/fs_file_system_proxy_service_object.hpp"

namespace ams::fs {

    namespace impl {

        Result ReadSaveDataFileSystemExtraData(SaveDataExtraData *out, SaveDataId id) {
            auto fsp = impl::GetFileSystemProxyServiceObject();
            AMS_FS_R_TRY(fsp->ReadSaveDataFileSystemExtraData(sf::OutBuffer(out, sizeof(*out)), id));
            R_SUCCEED();
        }

        Result ReadSaveDataFileSystemExtraData(SaveDataExtraData *out, SaveDataSpaceId space_id, SaveDataId id) {
            auto fsp = impl::GetFileSystemProxyServiceObject();
            AMS_FS_R_TRY(fsp->ReadSaveDataFileSystemExtraDataBySaveDataSpaceId(sf::OutBuffer(out, sizeof(*out)), static_cast<u8>(space_id), id));
            R_SUCCEED();
        }

        Result WriteSaveDataFileSystemExtraData(SaveDataSpaceId space_id, SaveDataId id, const SaveDataExtraData &extra_data) {
            auto fsp = impl::GetFileSystemProxyServiceObject();
            AMS_FS_R_TRY(fsp->WriteSaveDataFileSystemExtraData(id, static_cast<u8>(space_id), sf::InBuffer(std::addressof(extra_data), sizeof(extra_data))));
            R_SUCCEED();
        }

    }

    void DisableAutoSaveDataCreation() {
        auto fsp = impl::GetFileSystemProxyServiceObject();
        AMS_FS_R_ABORT_UNLESS(fsp->DisableAutoSaveDataCreation());
    }

    Result CreateSystemSaveData(SaveDataSpaceId space_id, SystemSaveDataId save_id, UserId user_id, u64 owner_id, s64 size, s64 journal_size, u32 flags) {
        auto create_impl = [=]() -> Result {
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

            auto fsp = impl::GetFileSystemProxyServiceObject();
            R_RETURN(fsp->CreateSaveDataFileSystemBySystemSaveDataId(attribute, info));
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM(create_impl(), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_CREATE_SYSTEM_SAVE_DATA(space_id, save_id, user_id, owner_id, size, journal_size, flags)));
        R_SUCCEED();
    }

    Result CreateSystemSaveData(SystemSaveDataId save_id, s64 size, s64 journal_size, u32 flags) {
        R_RETURN(CreateSystemSaveData(SaveDataSpaceId::System, save_id, InvalidUserId, 0, size, journal_size, flags));
    }

    Result CreateSystemSaveData(SystemSaveDataId save_id, u64 owner_id, s64 size, s64 journal_size, u32 flags) {
        R_RETURN(CreateSystemSaveData(SaveDataSpaceId::System, save_id, InvalidUserId, owner_id, size, journal_size, flags));
    }

    Result CreateSystemSaveData(SaveDataSpaceId space_id, SystemSaveDataId save_id, u64 owner_id, s64 size, s64 journal_size, u32 flags) {
        R_RETURN(CreateSystemSaveData(space_id, save_id, InvalidUserId, owner_id, size, journal_size, flags));
    }

    Result CreateSystemSaveData(SystemSaveDataId save_id, UserId user_id, s64 size, s64 journal_size, u32 flags) {
        R_RETURN(CreateSystemSaveData(SaveDataSpaceId::System, save_id, user_id, 0, size, journal_size, flags));
    }

    Result CreateSystemSaveData(SystemSaveDataId save_id, UserId user_id, u64 owner_id, s64 size, s64 journal_size, u32 flags) {
        R_RETURN(CreateSystemSaveData(SaveDataSpaceId::System, save_id, user_id, owner_id, size, journal_size, flags));
    }

    Result DeleteSaveData(SaveDataId id) {
        auto delete_impl = [=]() -> Result {
            auto fsp = impl::GetFileSystemProxyServiceObject();
            R_RETURN(fsp->DeleteSaveDataFileSystem(id));
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM(delete_impl(), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_ID, id));
        R_SUCCEED();
    }

    Result DeleteSaveData(SaveDataSpaceId space_id, SaveDataId id) {
        auto delete_impl = [=]() -> Result {
            auto fsp = impl::GetFileSystemProxyServiceObject();
            R_RETURN(fsp->DeleteSaveDataFileSystemBySaveDataSpaceId(static_cast<u8>(space_id), id));
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM(delete_impl(), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_DELETE_SAVE_DATA(space_id, id)));
        R_SUCCEED();
    }

    Result DeleteSystemSaveData(SaveDataSpaceId space_id, SystemSaveDataId id, UserId user_id) {
        auto delete_impl = [=]() -> Result {
            const auto attribute = SaveDataAttribute::Make(ncm::InvalidProgramId, SaveDataType::System, user_id, id);

            auto fsp = impl::GetFileSystemProxyServiceObject();
            R_RETURN(fsp->DeleteSaveDataFileSystemBySaveDataAttribute(static_cast<u8>(space_id), attribute));
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM(delete_impl(), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_DELETE_SYSTEM_SAVE_DATA(space_id, id, user_id)));
        R_SUCCEED();
    }

    Result GetSaveDataFlags(u32 *out, SaveDataId id) {
        SaveDataExtraData extra_data;
        R_TRY(impl::ReadSaveDataFileSystemExtraData(std::addressof(extra_data), id));

        *out = extra_data.flags;
        R_SUCCEED();
    }

    Result GetSaveDataFlags(u32 *out, SaveDataSpaceId space_id, SaveDataId id) {
        SaveDataExtraData extra_data;
        R_TRY(impl::ReadSaveDataFileSystemExtraData(std::addressof(extra_data), space_id, id));

        *out = extra_data.flags;
        R_SUCCEED();
    }

    Result SetSaveDataFlags(SaveDataId id, SaveDataSpaceId space_id, u32 flags) {
        SaveDataExtraData extra_data;
        R_TRY(impl::ReadSaveDataFileSystemExtraData(std::addressof(extra_data), space_id, id));
        extra_data.flags = flags;
        R_RETURN(impl::WriteSaveDataFileSystemExtraData(space_id, id, extra_data));
    }

    Result GetSaveDataAvailableSize(s64 *out, SaveDataId id) {
        SaveDataExtraData extra_data;
        R_TRY(impl::ReadSaveDataFileSystemExtraData(std::addressof(extra_data), id));

        *out = extra_data.available_size;
        R_SUCCEED();
    }

    Result GetSaveDataJournalSize(s64 *out, SaveDataId id) {
        SaveDataExtraData extra_data;
        R_TRY(impl::ReadSaveDataFileSystemExtraData(std::addressof(extra_data), id));

        *out = extra_data.journal_size;
        R_SUCCEED();
    }

    Result ExtendSaveData(SaveDataSpaceId space_id, SaveDataId id, s64 available_size, s64 journal_size) {
        auto extend_impl = [=]() -> Result {
            auto fsp = impl::GetFileSystemProxyServiceObject();
            R_RETURN(fsp->ExtendSaveDataFileSystem(static_cast<u8>(space_id), id, available_size, journal_size));
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM(extend_impl(), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_EXTEND_SAVE_DATA(space_id, id, available_size, journal_size)));
        R_SUCCEED();
    }

}
