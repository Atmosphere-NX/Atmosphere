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
#include <stratosphere/kvdb/kvdb_bounded_string.hpp>
#include <stratosphere/ncm/ncm_install_task_base.hpp>

namespace ams::ncm {

    class PackageInstallTaskBase : public InstallTaskBase {
        private:
            using PackagePath = kvdb::BoundedString<256>;
        private:
            PackagePath package_root;
            void *buffer;
            size_t buffer_size;
        public:
            PackageInstallTaskBase() : package_root() { /* ... */ }

            Result Initialize(const char *package_root_path, void *buffer, size_t buffer_size, StorageId storage_id, InstallTaskDataBase *data, u32 config);
        protected:
            const char *GetPackageRootPath() {
                return this->package_root.Get();
            }
        private:
            virtual Result OnWritePlaceHolder(const ContentMetaKey &key, InstallContentInfo *content_info) override;
            virtual Result InstallTicket(const fs::RightsId &rights_id, ContentMetaType meta_type) override;

            void CreateContentMetaPath(PackagePath *out_path, ContentId content_id);
            void CreateContentPath(PackagePath *out_path, ContentId content_id);
            void CreateTicketPath(PackagePath *out_path, fs::RightsId id);
            void CreateCertificatePath(PackagePath *out_path, fs::RightsId id);
    };

}
