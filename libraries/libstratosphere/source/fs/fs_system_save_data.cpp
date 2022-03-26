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
#include "impl/fs_file_system_service_object_adapter.hpp"

namespace ams::fs {

    Result MountSystemSaveData(const char *name, SystemSaveDataId id) {
        R_RETURN(MountSystemSaveData(name, id, InvalidUserId));
    }

    Result MountSystemSaveData(const char *name, SaveDataSpaceId space_id, SystemSaveDataId id) {
        R_RETURN(MountSystemSaveData(name, space_id, id, InvalidUserId));
    }

    Result MountSystemSaveData(const char *name, SystemSaveDataId id, UserId user_id) {
        R_RETURN(MountSystemSaveData(name, SaveDataSpaceId::System, id, user_id));
    }

    Result MountSystemSaveData(const char *name, SaveDataSpaceId space_id, SystemSaveDataId id, UserId user_id) {
        auto mount_impl = [=]() -> Result {
            /* Validate the mount name. */
            R_TRY(impl::CheckMountName(name));

            /* Create the attribute. */
            const auto attribute = SaveDataAttribute::Make(ncm::InvalidProgramId, SaveDataType::System, user_id, id);

            /* Open the filesystem. */
            auto fsp = impl::GetFileSystemProxyServiceObject();
            sf::SharedPointer<fssrv::sf::IFileSystem> fs;
            R_TRY(fsp->OpenSaveDataFileSystemBySystemSaveDataId(std::addressof(fs), static_cast<u8>(space_id), attribute));

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<impl::FileSystemServiceObjectAdapter>(std::move(fs));
            R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInSystemSaveDataA());

            /* Register. */
            R_RETURN(fsa::Register(name, std::move(fsa)));
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(mount_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_SYSTEM_SAVE_DATA(name, space_id, id, user_id)));
        AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);
        R_SUCCEED();
    }

}
