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
#include "fs_filesystem_accessor.hpp"
#include "fs_mount_utils.hpp"
#include "fs_user_mount_table.hpp"

namespace ams::fs::impl {

    namespace {

        const char *FindMountNameDriveSeparator(const char *path) {
            for (const char *cur = path; cur < path + MountNameLengthMax + 1; cur++) {
                if (*cur == StringTraits::DriveSeparator) {
                    return cur;
                } else if (PathNormalizer::IsNullTerminator(*cur)) {
                    break;
                }
            }
            return nullptr;
        }

        Result GetMountNameAndSubPath(MountName *out_mount_name, const char **out_sub_path, const char *path) {
            /* Handle the Host-path case. */
            if (fs::IsWindowsDrive(path) || fs::IsUnc(path)) {
                std::strncpy(out_mount_name->str, HostRootFileSystemMountName, MountNameLengthMax);
                out_mount_name->str[MountNameLengthMax] = '\x00';
                return ResultSuccess();
            }

            /* Locate the drive separator. */
            const char *drive_separator = FindMountNameDriveSeparator(path);
            R_UNLESS(drive_separator != nullptr, fs::ResultInvalidMountName());

            /* Ensure the mount name isn't too long. */
            const size_t len = drive_separator - path;
            R_UNLESS(len <= MountNameLengthMax, fs::ResultInvalidMountName());

            /* Ensure the result sub-path is valid. */
            const char *sub_path = drive_separator + 1;
            R_UNLESS(!PathNormalizer::IsNullTerminator(sub_path[0]), fs::ResultInvalidMountName());
            R_UNLESS(PathNormalizer::IsAnySeparator(sub_path[0]),    fs::ResultInvalidPathFormat());

            /* Set output. */
            std::memcpy(out_mount_name->str, path, len);
            out_mount_name->str[len] = StringTraits::NullTerminator;
            *out_sub_path = sub_path;
            return ResultSuccess();
        }

    }

    bool IsValidMountName(const char *name) {
        if (PathNormalizer::IsNullTerminator(name[0])) {
            return false;
        }

        if ((('a' <= name[0] && name[0] <= 'z') || ('A' <= name[0] && name[0] <= 'Z')) && PathNormalizer::IsNullTerminator(name[1])) {
            return false;
        }

        size_t len = 0;
        for (const char *cur = name; !PathNormalizer::IsNullTerminator(*cur); cur++) {
            if (*cur == StringTraits::DriveSeparator || PathNormalizer::IsSeparator(*cur)) {
                return false;
            }

            if ((++len) > MountNameLengthMax) {
                return false;
            }
        }

        /* TODO: N validates that the mount name decodes via utf-8 here. */
        return true;
    }

    bool IsReservedMountName(const char *name) {
        return name[0] == ReservedMountNamePrefixCharacter;
    }

    Result CheckMountName(const char *name) {
        R_TRY(CheckMountNameAllowingReserved(name));
        R_UNLESS(!impl::IsReservedMountName(name), fs::ResultInvalidMountName());
        return ResultSuccess();
    }

    Result CheckMountNameAllowingReserved(const char *name) {
        R_UNLESS(name != nullptr,              fs::ResultInvalidMountName());
        R_UNLESS(impl::IsValidMountName(name), fs::ResultInvalidMountName());
        return ResultSuccess();
    }

    Result FindFileSystem(FileSystemAccessor **out_accessor, const char **out_sub_path, const char *path) {
        R_UNLESS(out_accessor != nullptr, fs::ResultUnexpectedInFindFileSystemA());
        R_UNLESS(out_sub_path != nullptr, fs::ResultUnexpectedInFindFileSystemA());
        R_UNLESS(path != nullptr,         fs::ResultNullptrArgument());

        R_UNLESS(strncmp(path, HostRootFileSystemMountName, strnlen(HostRootFileSystemMountName, sizeof(MountName))) != 0, fs::ResultNotMounted());

        MountName mount_name;
        R_TRY(GetMountNameAndSubPath(std::addressof(mount_name), out_sub_path, path));

        return impl::Find(out_accessor, mount_name.str);
    }

    Result Unmount(const char *name) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::Find(std::addressof(accessor), name));

        if (accessor->IsFileDataCacheAttachable()) {
            /* TODO: Data cache purge */
        }

        impl::Unregister(name);
        return ResultSuccess();
    }

}

namespace ams::fs {

    namespace {

    }

    Result ConvertToFsCommonPath(char *dst, size_t dst_size, const char *src) {
        /* Ensure neither argument is nullptr. */
        AMS_FS_R_UNLESS(dst != nullptr, fs::ResultNullptrArgument());
        AMS_FS_R_UNLESS(src != nullptr, fs::ResultNullptrArgument());

        /* Get the mount name and sub path for the path. */
        MountName mount_name;
        const char *sub_path;
        AMS_FS_R_TRY(impl::GetMountNameAndSubPath(std::addressof(mount_name), std::addressof(sub_path), src));

        impl::FileSystemAccessor *accessor;
        AMS_FS_R_TRY(impl::Find(std::addressof(accessor), mount_name.str));
        AMS_FS_R_TRY(accessor->GetCommonMountName(dst, dst_size));

        const auto mount_name_len = strnlen(dst, dst_size);
        const auto common_path_len = std::snprintf(dst + mount_name_len, dst_size - mount_name_len, "%s", sub_path);

        AMS_FS_R_UNLESS(static_cast<size_t>(common_path_len) < dst_size - mount_name_len, fs::ResultTooLongPath());
        return ResultSuccess();
    }

    void Unmount(const char *mount_name) {
        AMS_FS_R_ABORT_UNLESS(AMS_FS_IMPL_ACCESS_LOG_UNMOUNT(impl::Unmount(mount_name), mount_name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT, mount_name));
    }

}
