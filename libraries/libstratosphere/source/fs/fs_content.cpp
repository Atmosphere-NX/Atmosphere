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

        impl::FileSystemProxyType ConvertToFileSystemProxyType(ContentType content_type) {
            switch (content_type) {
                case ContentType_Control: return impl::FileSystemProxyType_Control;
                case ContentType_Manual:  return impl::FileSystemProxyType_Manual;
                case ContentType_Meta:    return impl::FileSystemProxyType_Meta;
                case ContentType_Data:    return impl::FileSystemProxyType_Data;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        Result MountContentImpl(const char *name, const char *path, fs::ContentAttributes attr, u64 id, ContentType type) {
            /* Validate the mount name. */
            R_TRY(impl::CheckMountNameAllowingReserved(name));

            /* Validate the path. */
            R_UNLESS(path != nullptr, fs::ResultInvalidPath());

            /* Convert the path for fsp. */
            fssrv::sf::FspPath sf_path;
            R_TRY(fs::ConvertToFspPath(std::addressof(sf_path), path));

            /* Open the filesystem. */
            auto fsp = impl::GetFileSystemProxyServiceObject();
            sf::SharedPointer<fssrv::sf::IFileSystem> fs;
            R_TRY(fsp->OpenFileSystemWithId(std::addressof(fs), sf_path, attr, id, ConvertToFileSystemProxyType(type)));

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<impl::FileSystemServiceObjectAdapter>(std::move(fs));
            R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInContentA());

            /* Register. */
            R_RETURN(fsa::Register(name, std::move(fsa)));
        }

    }

    Result MountContent(const char *name, const char *path, fs::ContentAttributes attr, ContentType content_type) {
        auto mount_impl = [=]() -> Result {
            /* This API only supports mounting Meta content. */
            R_UNLESS(content_type == ContentType_Meta, fs::ResultInvalidArgument());

            R_RETURN(MountContentImpl(name, path, attr, ncm::InvalidProgramId.value, content_type));
        };

        /* Perform the mount. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(mount_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CONTENT_PATH(name, path, content_type)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

    Result MountContent(const char *name, const char *path, fs::ContentAttributes attr, ncm::ProgramId id, ContentType content_type) {
        auto mount_impl = [=]() -> Result {
            R_RETURN(MountContentImpl(name, path, attr, id.value, content_type));
        };

        /* Perform the mount. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(mount_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CONTENT_PATH_AND_PROGRAM_ID(name, path, id, content_type)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

    Result MountContent(const char *name, const char *path, fs::ContentAttributes attr, ncm::DataId id, ContentType content_type) {
        auto mount_impl = [=]() -> Result {
            R_RETURN(MountContentImpl(name, path, attr, id.value, content_type));
        };

        /* Perform the mount. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(mount_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CONTENT_PATH_AND_DATA_ID(name, path, id, content_type)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

}
