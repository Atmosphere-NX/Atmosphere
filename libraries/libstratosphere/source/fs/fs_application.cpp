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
#include "impl/fs_file_system_service_object_adapter.hpp"
#include "impl/fs_file_system_proxy_service_object.hpp"

namespace ams::fs {

    Result MountApplicationPackage(const char *name, const char *common_path) {
        auto mount_impl = [=]() -> Result {
            /* Validate the mount name. */
            R_TRY(impl::CheckMountName(name));

            /* Validate the path. */
            R_UNLESS(common_path != nullptr, fs::ResultInvalidPath());

            /* Convert the path for ipc. */
            fssrv::sf::FspPath sf_path;
            R_TRY(fs::ConvertToFspPath(std::addressof(sf_path), common_path));

            /* Open the filesystem. */
            auto fsp = impl::GetFileSystemProxyServiceObject();
            sf::SharedPointer<fssrv::sf::IFileSystem> fs;
            R_TRY(fsp->OpenFileSystemWithId(std::addressof(fs), sf_path, fs::ContentAttributes_None, ncm::InvalidProgramId.value, impl::FileSystemProxyType_Package));

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<impl::FileSystemServiceObjectAdapter>(std::move(fs));
            R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInApplicationA());

            /* Register. */
            R_RETURN(fsa::Register(name, std::move(fsa)));
        };

        /* Perform the mount. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(mount_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_APPLICATION_PACKAGE(name, common_path)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

}
