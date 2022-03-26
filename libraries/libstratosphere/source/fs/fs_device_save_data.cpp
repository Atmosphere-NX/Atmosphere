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

    namespace {

        constexpr inline SaveDataSpaceId DeviceSaveDataSpaceId = SaveDataSpaceId::User;

        Result MountDeviceSaveDataImpl(const char *name, const SaveDataAttribute &attribute) {
            /* Validate the mount name. */
            R_TRY(impl::CheckMountName(name));

            /* Open the filesystem. */
            auto fsp = impl::GetFileSystemProxyServiceObject();
            sf::SharedPointer<fssrv::sf::IFileSystem> fs;
            R_TRY(fsp->OpenSaveDataFileSystem(std::addressof(fs), static_cast<u8>(DeviceSaveDataSpaceId), attribute));

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<impl::FileSystemServiceObjectAdapter>(std::move(fs));
            R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInDeviceSaveDataA());

            /* Register. */
            R_RETURN(fsa::Register(name, std::move(fsa)));
        }

    }

    Result MountDeviceSaveData(const char *name) {
        const auto attr = SaveDataAttribute::Make(ncm::InvalidProgramId, SaveDataType::Device, InvalidUserId, InvalidSystemSaveDataId);

        /* Perform the mount. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT(MountDeviceSaveDataImpl(name, attr), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT, name));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

    Result MountDeviceSaveData(const char *name, const ncm::ApplicationId application_id) {
        const auto attr = SaveDataAttribute::Make(application_id, SaveDataType::Device, InvalidUserId, InvalidSystemSaveDataId);

        /* Perform the mount. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT(MountDeviceSaveDataImpl(name, attr), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_DEVICE_SAVE_DATA_APPLICATION_ID(name, application_id)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

}
