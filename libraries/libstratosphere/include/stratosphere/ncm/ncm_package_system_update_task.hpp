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
#pragma once
#include <stratosphere/ncm/ncm_package_install_task_base.hpp>

namespace ams::ncm {

    class PackageSystemUpdateTask : public PackageInstallTaskBase {
        private:
            using PackagePath = kvdb::BoundedString<0x100>;
        private:
            PackagePath m_context_path;
            FileInstallTaskData m_data;
            ContentMetaDatabase m_package_db;
            bool m_gamecard_content_meta_database_active;
        public:
            ~PackageSystemUpdateTask();

            void Inactivate();
            Result Initialize(const char *package_root, const char *context_path, void *buffer, size_t buffer_size, bool requires_exfat_driver, FirmwareVariationId firmware_variation_id);
            util::optional<ContentMetaKey> GetSystemUpdateMetaKey();
        protected:
            virtual Result PrepareInstallContentMetaData() override;
            virtual Result GetInstallContentMetaInfo(InstallContentMetaInfo *out, const ContentMetaKey &key) override;
            InstallTaskDataBase &GetInstallData() { return m_data; } /* Atmosphere extension. */
        private:
            virtual Result PrepareDependency() override;

            Result GetContentInfoOfContentMeta(ContentInfo *out, const ContentMetaKey &key);
    };

}
