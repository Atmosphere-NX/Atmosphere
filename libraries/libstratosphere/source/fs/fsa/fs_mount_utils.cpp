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
                if (PathTool::IsDriveSeparator(*cur)) {
                    return cur;
                } else if (PathTool::IsNullTerminator(*cur)) {
                    break;
                }
            }
            return nullptr;
        }

        Result GetMountNameImpl(MountName &out, const char *path) {
            const char *drive_separator = FindMountNameDriveSeparator(path);
            R_UNLESS(drive_separator != nullptr, fs::ResultInvalidMountName());

            const size_t len = drive_separator - path;
            R_UNLESS(len + 1 <= sizeof(MountName), fs::ResultInvalidMountName());

            std::memcpy(out.str, path, len);
            out.str[len] = StringTraits::NullTerminator;
            return ResultSuccess();
        }

    }

    MountName GetMountName(const char *path) {
        MountName mount_name;

        if (IsWindowsDrive(path)) {
            std::strncpy(mount_name.str, HostRootFileSystemMountName, MountNameLengthMax);
            mount_name.str[MountNameLengthMax] = StringTraits::NullTerminator;
        } else {
            R_ABORT_UNLESS(GetMountNameImpl(mount_name, path));
        }

        return mount_name;
    }

    const char *GetSubPath(const char *path) {
        if (IsWindowsDrive(path)) {
            return path;
        }

        const char *sub_path = path;
        while (!PathTool::IsDriveSeparator(*sub_path)) {
            sub_path++;
        }

        AMS_ABORT_UNLESS(PathTool::IsSeparator(sub_path[1]) || PathTool::IsUnsupportedSeparator(sub_path[1]));
        return sub_path + 1;
    }

    bool IsValidMountName(const char *name) {
        if (PathTool::IsNullTerminator(*name)) {
            return false;
        }

        if (PathTool::IsWindowsDriveCharacter(name[0]) && PathTool::IsNullTerminator(name[1])) {
            return false;
        }

        size_t len = 0;
        for (const char *cur = name; !PathTool::IsNullTerminator(*cur); cur++) {
            if (PathTool::IsDriveSeparator(*cur) || PathTool::IsSeparator(*cur)) {
                return false;
            }

            if ((++len) > MountNameLengthMax) {
                return false;
            }
        }

        /* TODO: N validates that the mount name decodes via utf-8 here. */
        return true;
    }

    bool IsWindowsDrive(const char *name) {
        return PathTool::IsWindowsAbsolutePath(name);
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

    Result FindFileSystem(FileSystemAccessor **out, const char *path) {
        R_UNLESS(path != nullptr, fs::ResultInvalidPath());

        R_UNLESS(strncmp(path, HostRootFileSystemMountName, strnlen(HostRootFileSystemMountName, sizeof(MountName))) != 0, fs::ResultInvalidPathFormat());

        return impl::Find(out, GetMountName(path).str);
    }

}

namespace ams::fs {

    namespace {

        Result UnmountImpl(const char *name) {
            impl::FileSystemAccessor *accessor;
            R_TRY(impl::Find(std::addressof(accessor), name));

            if (accessor->IsFileDataCacheAttachable()) {
                /* TODO: Data cache purge */
            }

            impl::Unregister(name);
            return ResultSuccess();
        }

    }

    Result ConvertToFsCommonPath(char *dst, size_t dst_size, const char *src) {
        /* Get the mount name for the path. */
        MountName mount_name = impl::GetMountName(src);

        impl::FileSystemAccessor *accessor;
        R_TRY(impl::Find(std::addressof(accessor), mount_name.str));
        R_TRY(accessor->GetCommonMountName(dst, dst_size));

        const auto mount_name_len = strnlen(dst, dst_size);
        const auto common_path_len = std::snprintf(dst + mount_name_len, dst_size - mount_name_len, "%s", impl::GetSubPath(src));

        R_UNLESS(static_cast<size_t>(common_path_len) < dst_size - mount_name_len, fs::ResultTooLongPath());
        return ResultSuccess();
    }

    void Unmount(const char *mount_name) {
        R_ABORT_UNLESS(UnmountImpl(mount_name));
    }

}
