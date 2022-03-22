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

    class PackageInstallTask : public PackageInstallTaskBase {
        private:
            MemoryInstallTaskData m_data;
        public:
            Result Initialize(const char *package_root, StorageId storage_id, void *buffer, size_t buffer_size, bool ignore_ticket);
        protected:
            bool IsContentMetaContentName(const char *name);
            virtual Result PrepareInstallContentMetaData() override;
        private:
            virtual Result GetInstallContentMetaInfo(InstallContentMetaInfo *out_info, const ContentMetaKey &key) override;
    };

}
