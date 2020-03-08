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
#include <stratosphere.hpp>
#include "fsa/fs_mount_utils.hpp"

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

        Result MountContentImpl(const char *name, const char *path, u64 id, impl::FileSystemProxyType type) {
            /* Open a filesystem using libnx bindings. */
            FsFileSystem fs;
            R_TRY(fsOpenFileSystemWithId(std::addressof(fs), id, static_cast<::FsFileSystemType>(type), path));

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<RemoteFileSystem>(fs);
            R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInContentA());

            /* Register. */
            return fsa::Register(name, std::move(fsa));
        }

        Result MountContent(const char *name, const char *path, u64 id, ContentType content_type) {
            /* Validate the mount name. */
            R_TRY(impl::CheckMountNameAllowingReserved(name));

            /* Validate the path. */
            R_UNLESS(path != nullptr, fs::ResultInvalidPath());

            /* Mount the content. */
            return MountContentImpl(name, path, id, ConvertToFileSystemProxyType(content_type));
        }

    }

    Result MountContent(const char *name, const char *path, ContentType content_type) {
        /* This API only supports mounting Meta content. */
        AMS_ABORT_UNLESS(content_type == ContentType_Meta);

        return MountContent(name, path, ncm::InvalidProgramId, content_type);
    }

    Result MountContent(const char *name, const char *path, ncm::ProgramId id, ContentType content_type) {
        return MountContent(name, path, id.value, content_type);
    }

    Result MountContent(const char *name, const char *path, ncm::DataId id, ContentType content_type) {
        return MountContent(name, path, id.value, content_type);
    }

}
