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

namespace ams::ncm {

    Result PackageInstallTaskBase::Initialize(const char *package_path, void *buffer, size_t buffer_size, StorageId storage_id, InstallTaskDataBase *data, u32 config) {
        R_TRY(InstallTaskBase::Initialize(storage_id, data, config));
        this->package_root.Set(package_path);
        this->buffer = buffer;
        this->buffer_size = buffer_size;
        return ResultSuccess();
    }

    Result PackageInstallTaskBase::OnWritePlaceHolder(const ContentMetaKey &key, InstallContentInfo *content_info) {
        PackagePath path;
        if (content_info->GetType() == ContentType::Meta) {
            this->CreateContentMetaPath(std::addressof(path), content_info->GetId());
        } else {
            this->CreateContentPath(std::addressof(path), content_info->GetId());
        }
        /* Open the file. */
        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Read));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Continuously write the file to the placeholder until there is nothing left to write. */
        while (true) {
            /* Read as much of the remainder of the file as possible. */
            size_t size_read;
            R_TRY(fs::ReadFile(std::addressof(size_read), file, content_info->written, this->buffer, this->buffer_size));

            /* There is nothing left to read. */
            if (size_read == 0) {
                break;
            }

            /* Write the placeholder. */
            R_TRY(this->WritePlaceHolderBuffer(content_info, this->buffer, size_read));
        }

        return ResultSuccess();
    }

    void PackageInstallTaskBase::CreateContentMetaPath(PackagePath *out_path, ContentId content_id) {
        char str[ContentIdStringLength + 1] = {};
        GetStringFromContentId(str, sizeof(str), content_id);
        out_path->SetFormat("%s%s%s", this->package_root.Get(), str, ".cnmt.nca");
    }

    void PackageInstallTaskBase::CreateContentPath(PackagePath *out_path, ContentId content_id) {
        char str[ContentIdStringLength + 1] = {};
        GetStringFromContentId(str, sizeof(str), content_id);
        out_path->SetFormat("%s%s%s", this->package_root.Get(), str, ".nca");
    }

    Result PackageInstallTaskBase::InstallTicket(const fs::RightsId &rights_id, ContentMetaType meta_type) {
        /* TODO: Read ticket from file. */
        /* TODO: Read certificate from file. */
        /* TODO: es::ImportTicket() */
        /* TODO: How should es be handled without undesired effects? */
        return ResultSuccess();
    }

    void PackageInstallTaskBase::CreateTicketPath(PackagePath *out_path, fs::RightsId id) {
        char str[RightsIdStringLength + 1] = {};
        GetStringFromRightsId(str, sizeof(str), id);
        out_path->SetFormat("%s%s%s", this->package_root.Get(), str, ".tik");
    }

    void PackageInstallTaskBase::CreateCertificatePath(PackagePath *out_path, fs::RightsId id) {
        char str[RightsIdStringLength + 1] = {};
        GetStringFromRightsId(str, sizeof(str), id);
        out_path->SetFormat("%s%s%s", this->package_root.Get(), str, ".cert");
    }

}
