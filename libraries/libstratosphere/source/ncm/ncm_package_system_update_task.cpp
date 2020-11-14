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

    PackageSystemUpdateTask::~PackageSystemUpdateTask() {
        if (this->context_path.GetLength() > 0) {
            fs::DeleteFile(this->context_path);
        }
        this->Inactivate();
    }

    void PackageSystemUpdateTask::Inactivate() {
        if (this->gamecard_content_meta_database_active) {
            InactivateContentMetaDatabase(StorageId::GameCard);
            this->gamecard_content_meta_database_active = false;
        }
    }

    Result PackageSystemUpdateTask::Initialize(const char *package_root, const char *context_path, void *buffer, size_t buffer_size, bool requires_exfat_driver, FirmwareVariationId firmware_variation_id) {
        /* Set the firmware variation id. */
        this->SetFirmwareVariationId(firmware_variation_id);

        /* Activate the game card content meta database. */
        R_TRY(ActivateContentMetaDatabase(StorageId::GameCard));
        this->gamecard_content_meta_database_active = true;
        auto meta_db_guard = SCOPE_GUARD { this->Inactivate(); };

        /* Open the game card content meta database. */
        OpenContentMetaDatabase(std::addressof(this->package_db), StorageId::GameCard);

        ContentMetaDatabaseBuilder builder(std::addressof(this->package_db));

        /* Cleanup and build the content meta database. */
        R_TRY(builder.Cleanup());
        R_TRY(builder.BuildFromPackage(package_root));

        /* Create a new context file. */
        fs::DeleteFile(context_path);
        R_TRY(FileInstallTaskData::Create(context_path, GameCardMaxContentMetaCount));
        auto context_guard = SCOPE_GUARD { fs::DeleteFile(context_path); };

        /* Initialize data. */
        R_TRY(this->data.Initialize(context_path));

        /* Initialize PackageInstallTaskBase. */
        u32 config = !requires_exfat_driver ? InstallConfig_SystemUpdate : InstallConfig_SystemUpdate | InstallConfig_RequiresExFatDriver;
        R_TRY(PackageInstallTaskBase::Initialize(package_root, buffer, buffer_size, StorageId::BuiltInSystem, std::addressof(this->data), config));

        /* Cancel guards. */
        context_guard.Cancel();
        meta_db_guard.Cancel();

        /* Set the context path. */
        this->context_path.Set(context_path);
        return ResultSuccess();
    }

    std::optional<ContentMetaKey> PackageSystemUpdateTask::GetSystemUpdateMetaKey() {
        StorageContentMetaKey storage_keys[0x10];
        s32 ofs = 0;

        s32 count = 0;
        do {
            /* List content meta keys. */
            if (R_FAILED(this->ListContentMetaKey(std::addressof(count), storage_keys, util::size(storage_keys), ofs, ListContentMetaKeyFilter::All))) {
                break;
            }

            /* Add listed keys to the offset. */
            ofs += count;

            /* Check if any of these keys are for a SystemUpdate. */
            for (s32 i = 0; i < count; i++) {
                const ContentMetaKey &key = storage_keys[i].key;
                if (key.type == ContentMetaType::SystemUpdate) {
                    return key;
                }
            }
        } while (count > 0);

        return std::nullopt;
    }

    Result PackageSystemUpdateTask::GetInstallContentMetaInfo(InstallContentMetaInfo *out, const ContentMetaKey &key) {
        /* Get the content info for the key. */
        ContentInfo info;
        R_TRY(this->GetContentInfoOfContentMeta(std::addressof(info), key));

        /* Create a new install content meta info. */
        *out = InstallContentMetaInfo::MakeUnverifiable(info.GetId(), info.GetSize(), key);
        return ResultSuccess();
    }

    Result PackageSystemUpdateTask::PrepareInstallContentMetaData() {
        /* Obtain a SystemUpdate key. */
        ContentMetaKey key;
        auto list_count = this->package_db.ListContentMeta(std::addressof(key), 1, ContentMetaType::SystemUpdate);
        R_UNLESS(list_count.written > 0, ncm::ResultSystemUpdateNotFoundInPackage());

        /* Get the content info for the key. */
        ContentInfo info;
        R_TRY(this->GetContentInfoOfContentMeta(std::addressof(info), key));

        /* Prepare the content meta. */
        return this->PrepareContentMeta(InstallContentMetaInfo::MakeUnverifiable(info.GetId(), info.GetSize(), key), key, std::nullopt);
    }

    Result PackageSystemUpdateTask::PrepareDependency() {
        return this->PrepareSystemUpdateDependency();
    }

    Result PackageSystemUpdateTask::GetContentInfoOfContentMeta(ContentInfo *out, const ContentMetaKey &key) {
        s32 ofs = 0;
        while (true) {
            /* List content infos. */
            s32 count;
            ContentInfo info;
            R_TRY(this->package_db.ListContentInfo(std::addressof(count), std::addressof(info), 1, key, ofs++));

            /* No content infos left to list. */
            if (count == 0) {
                break;
            }

            /* Check if the info is for meta content. */
            if (info.GetType() == ContentType::Meta) {
                *out = info;
                return ResultSuccess();
            }
        }

        /* Not found. */
        return ncm::ResultContentInfoNotFound();
    }

}
