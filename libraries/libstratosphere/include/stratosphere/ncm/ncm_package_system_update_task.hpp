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
#pragma once
#include <stratosphere/ncm/ncm_package_install_task_base.hpp>

namespace ams::ncm {

    class PackageSystemUpdateTask : public PackageInstallTaskBase {
        private:
            using PackagePath = kvdb::BoundedString<0x100>;
        private:
            PackagePath context_path;
            FileInstallTaskData data;
            ContentMetaDatabase package_db;
            bool gamecard_content_meta_database_active;
        public:
            ~PackageSystemUpdateTask();

            void Inactivate();
            Result Initialize(const char *package_root, const char *context_path, void *buffer, size_t buffer_size, bool requires_exfat_driver, FirmwareVariationId firmware_variation_id);
            std::optional<ContentMetaKey> GetSystemUpdateMetaKey();
        protected:
            virtual Result PrepareInstallContentMetaData() override;
            virtual Result GetInstallContentMetaInfo(InstallContentMetaInfo *out, const ContentMetaKey &key) override;
            InstallTaskDataBase &GetInstallData() { return this->data; } /* Atmosphere extension. */
        private:
            virtual Result PrepareDependency() override;

            Result GetContentInfoOfContentMeta(ContentInfo *out, const ContentMetaKey &key);
    };

}
