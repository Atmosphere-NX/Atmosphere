/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include "ncm_fs_utils.hpp"

namespace ams::ncm {

    bool PackageInstallTask::IsContentMetaContentName(const char *name) {
        return impl::PathView(name).HasSuffix(".cnmt");
    }

    Result PackageInstallTask::Initialize(const char *package_root, StorageId storage_id, void *buffer, size_t buffer_size, bool ignore_ticket) {
        return PackageInstallTaskBase::Initialize(package_root, buffer, buffer_size, storage_id, std::addressof(this->data), ignore_ticket ? InstallConfig_IgnoreTicket : InstallConfig_None);
    }

    Result PackageInstallTask::GetInstallContentMetaInfo(InstallContentMetaInfo *out_info, const ContentMetaKey &key) {
        return ncm::ResultContentNotFound();
    }

    Result PackageInstallTask::PrepareInstallContentMetaData() {
        /* Open the directory. */
        fs::DirectoryHandle dir;
        R_TRY(fs::OpenDirectory(std::addressof(dir), this->GetPackageRootPath(), fs::OpenDirectoryMode_File));
        ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

        while (true) {
            /* Read the current entry. */
            s64 count;
            fs::DirectoryEntry entry;
            R_TRY(fs::ReadDirectory(std::addressof(count), std::addressof(entry), dir, 1));
            
            /* No more entries remain, we are done. */
            if (count == 0) {
                break;
            }

            /* Check if this entry is content meta. */
            if (this->IsContentMetaContentName(entry.name)) {
                /* Prepare content meta if id is valid. */
                std::optional<ContentId> id = GetContentIdFromString(entry.name, strnlen(entry.name, fs::EntryNameLengthMax + 1));
                R_UNLESS(id, ncm::ResultInvalidPackageFormat());
                R_TRY(this->PrepareContentMeta(InstallContentMetaInfo::MakeUnverifiable(*id, entry.file_size), std::nullopt, std::nullopt));
            }
        }

        return ResultSuccess();
    }

}
