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

    Result PackageSystemDowngradeTask::PreCommit() {
        constexpr size_t MaxContentMetas = 0x40;

        auto &data = this->GetInstallData();

        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(data.Count(std::addressof(count)));

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(data.Get(std::addressof(content_meta), i));

            /* Create a reader. */
            const auto reader = content_meta.GetReader();
            const auto key = reader.GetKey();

            /* Obtain a list of suitable storage ids. */
            const auto storage_list = GetStorageList(this->GetInstallStorage());

            /* Iterate over storage ids. */
            for (s32 i = 0; i < storage_list.Count(); i++) {
                /* Open the content meta database. */
                ContentMetaDatabase meta_db;
                if (R_FAILED(OpenContentMetaDatabase(std::addressof(meta_db), storage_list[i]))) {
                    continue;
                }

                /* List keys matching the type and id of the install content meta key. */
                ncm::ContentMetaKey keys[MaxContentMetas];
                const auto count = meta_db.ListContentMeta(keys, MaxContentMetas, key.type, { key.id });

                /* Remove matching keys. */
                for (auto i = 0; i < count.total; i++) {
                    meta_db.Remove(keys[i]);
                }
            }
        }

        return ResultSuccess();
    }

    Result PackageSystemDowngradeTask::Commit() {
        R_TRY(this->PreCommit());
        return InstallTaskBase::Commit();
    }

    Result PackageSystemDowngradeTask::PrepareContentMetaIfLatest(const ContentMetaKey &key) {
        /* Get and prepare install content meta info. We aren't concerned if our key is older. */
        InstallContentMetaInfo install_content_meta_info;
        R_TRY(this->GetInstallContentMetaInfo(std::addressof(install_content_meta_info), key));
        return this->PrepareContentMeta(install_content_meta_info, key, std::nullopt);
    }

}
