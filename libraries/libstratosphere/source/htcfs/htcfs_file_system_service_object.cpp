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
#include "htcfs_file_system_service_object.hpp"
#include "htcfs_file_service_object.hpp"
#include "htcfs_directory_service_object.hpp"
#include "htcfs_client.hpp"

namespace ams::htcfs {

    namespace {

        struct DirectoryServiceObjectAllocatorTag;
        struct FileServiceObjectAllocatorTag;

        using DirectoryServiceObjectAllocator = ams::sf::ExpHeapStaticAllocator<4_KB, DirectoryServiceObjectAllocatorTag>;
        using FileServiceObjectAllocator      = ams::sf::ExpHeapStaticAllocator<4_KB, FileServiceObjectAllocatorTag>;
        using DirectoryServiceObjectFactory   = ams::sf::ObjectFactory<typename DirectoryServiceObjectAllocator::Policy>;
        using FileServiceObjectFactory        = ams::sf::ObjectFactory<typename FileServiceObjectAllocator::Policy>;

        class StaticAllocatorInitializer {
            public:
                StaticAllocatorInitializer() {
                    DirectoryServiceObjectAllocator::Initialize(lmem::CreateOption_ThreadSafe);
                    FileServiceObjectAllocator::Initialize(lmem::CreateOption_ThreadSafe);
                }
        } g_static_allocator_initializer;

        constexpr bool IsValidPath(const tma::Path &path) {
            const auto len = util::Strnlen(path.str, fs::EntryNameLengthMax + 1);
            return 0 < len && len < static_cast<int>(fs::EntryNameLengthMax + 1);
        }

        Result ConvertOpenMode(fs::OpenMode *out, u32 open_mode) {
            switch (open_mode) {
                case 1:
                    *out = fs::OpenMode_Read;
                    break;
                case 2:
                    *out = static_cast<fs::OpenMode>(fs::OpenMode_Write | fs::OpenMode_AllowAppend);
                    break;
                case 3:
                    *out = static_cast<fs::OpenMode>(fs::OpenMode_ReadWrite | fs::OpenMode_AllowAppend);
                    break;
                default:
                    return htcfs::ResultInvalidArgument();
            }

            return ResultSuccess();
        }

    }

    Result FileSystemServiceObject::OpenFile(sf::Out<sf::SharedPointer<tma::IFileAccessor>> out, const tma::Path &path, u32 open_mode, bool case_sensitive) {
        /* Check that the path is valid. */
        R_UNLESS(IsValidPath(path), htcfs::ResultInvalidArgument());

        /* Convert the open mode. */
        fs::OpenMode fs_open_mode;
        R_TRY(ConvertOpenMode(std::addressof(fs_open_mode), open_mode));

        /* Open the file. */
        s32 handle;
        R_TRY(htcfs::GetClient().OpenFile(std::addressof(handle), path.str, fs_open_mode, case_sensitive));

        /* Set the output file. */
        *out = FileServiceObjectFactory::CreateSharedEmplaced<tma::IFileAccessor, FileServiceObject>(handle);
        return ResultSuccess();
    }

    Result FileSystemServiceObject::FileExists(sf::Out<bool> out, const tma::Path &path, bool case_sensitive) {
        /* Check that the path is valid. */
        R_UNLESS(IsValidPath(path), htcfs::ResultInvalidArgument());

        /* Get whether the file exists. */
        return htcfs::GetClient().FileExists(out.GetPointer(), path.str, case_sensitive);
    }

    Result FileSystemServiceObject::DeleteFile(const tma::Path &path, bool case_sensitive) {
        /* Check that the path is valid. */
        R_UNLESS(IsValidPath(path), htcfs::ResultInvalidArgument());

        /* Delete the file. */
        return htcfs::GetClient().DeleteFile(path.str, case_sensitive);
    }

    Result FileSystemServiceObject::RenameFile(const tma::Path &old_path, const tma::Path &new_path, bool case_sensitive) {
        /* Check that the paths are valid. */
        R_UNLESS(IsValidPath(old_path), htcfs::ResultInvalidArgument());
        R_UNLESS(IsValidPath(new_path), htcfs::ResultInvalidArgument());

        /* Rename the file. */
        return htcfs::GetClient().RenameFile(old_path.str, new_path.str, case_sensitive);
    }

    Result FileSystemServiceObject::GetIOType(sf::Out<s32> out, const tma::Path &path, bool case_sensitive) {
        /* Check that the path is valid. */
        R_UNLESS(IsValidPath(path), htcfs::ResultInvalidArgument());

        /* Get the entry type. */
        static_assert(sizeof(s32) == sizeof(fs::DirectoryEntryType));
        return htcfs::GetClient().GetEntryType(reinterpret_cast<fs::DirectoryEntryType *>(out.GetPointer()), path.str, case_sensitive);
    }

    Result FileSystemServiceObject::OpenDirectory(sf::Out<sf::SharedPointer<tma::IDirectoryAccessor>> out, const tma::Path &path, s32 open_mode, bool case_sensitive) {
        /* Check that the path is valid. */
        R_UNLESS(IsValidPath(path), htcfs::ResultInvalidArgument());

        /* Open the directory. */
        s32 handle;
        R_TRY(htcfs::GetClient().OpenDirectory(std::addressof(handle), path.str, static_cast<fs::OpenDirectoryMode>(open_mode), case_sensitive));

        /* Set the output directory. */
        *out = DirectoryServiceObjectFactory::CreateSharedEmplaced<tma::IDirectoryAccessor, DirectoryServiceObject>(handle);
        return ResultSuccess();
    }

    Result FileSystemServiceObject::DirectoryExists(sf::Out<bool> out, const tma::Path &path, bool case_sensitive) {
        /* Check that the path is valid. */
        R_UNLESS(IsValidPath(path), htcfs::ResultInvalidArgument());

        /* Get whether the file exists. */
        return htcfs::GetClient().DirectoryExists(out.GetPointer(), path.str, case_sensitive);
    }

    Result FileSystemServiceObject::CreateDirectory(const tma::Path &path, bool case_sensitive) {
        /* Check that the path is valid. */
        R_UNLESS(IsValidPath(path), htcfs::ResultInvalidArgument());

        /* Create the directory. */
        return htcfs::GetClient().CreateDirectory(path.str, case_sensitive);
    }

    Result FileSystemServiceObject::DeleteDirectory(const tma::Path &path, bool recursively, bool case_sensitive) {
        /* Check that the path is valid. */
        R_UNLESS(IsValidPath(path), htcfs::ResultInvalidArgument());

        /* Delete the directory. */
        return htcfs::GetClient().DeleteDirectory(path.str, recursively, case_sensitive);
    }

    Result FileSystemServiceObject::RenameDirectory(const tma::Path &old_path, const tma::Path &new_path, bool case_sensitive) {
        /* Check that the paths are valid. */
        R_UNLESS(IsValidPath(old_path), htcfs::ResultInvalidArgument());
        R_UNLESS(IsValidPath(new_path), htcfs::ResultInvalidArgument());

        /* Rename the file. */
        return htcfs::GetClient().RenameDirectory(old_path.str, new_path.str, case_sensitive);
    }

    Result FileSystemServiceObject::CreateFile(const tma::Path &path, s64 size, bool case_sensitive) {
        /* Check that the path is valid. */
        R_UNLESS(IsValidPath(path), htcfs::ResultInvalidArgument());

        /* Create the file. */
        return htcfs::GetClient().CreateFile(path.str, size, case_sensitive);
    }

    Result FileSystemServiceObject::GetFileTimeStamp(sf::Out<u64> out_create, sf::Out<u64> out_access, sf::Out<u64> out_modify, const tma::Path &path, bool case_sensitive) {
        /* Check that the path is valid. */
        R_UNLESS(IsValidPath(path), htcfs::ResultInvalidArgument());

        /* Get the timestamp. */
        return htcfs::GetClient().GetFileTimeStamp(out_create.GetPointer(), out_access.GetPointer(), out_modify.GetPointer(), path.str, case_sensitive);
    }

    Result FileSystemServiceObject::GetCaseSensitivePath(const tma::Path &path, const sf::OutBuffer &out) {
        /* Check that the path is valid. */
        R_UNLESS(IsValidPath(path), htcfs::ResultInvalidArgument());

        /* Get the case sensitive path. */
        return htcfs::GetClient().GetCaseSensitivePath(reinterpret_cast<char *>(out.GetPointer()), out.GetSize(), path.str);
    }

    Result FileSystemServiceObject::GetDiskFreeSpaceExW(sf::Out<s64> out_free, sf::Out<s64> out_total, sf::Out<s64> out_total_free, const tma::Path &path) {
        /* Check that the path is valid. */
        R_UNLESS(IsValidPath(path), htcfs::ResultInvalidArgument());

        /* Get the timestamp. */
        return htcfs::GetClient().GetDiskFreeSpace(out_free.GetPointer(), out_total.GetPointer(), out_total_free.GetPointer(), path.str);
    }

}
