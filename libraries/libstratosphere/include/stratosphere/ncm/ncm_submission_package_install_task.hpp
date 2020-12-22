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
#include <stratosphere/fs/common/fs_file_storage.hpp>
#include <stratosphere/ncm/ncm_package_install_task.hpp>

namespace ams::ncm {

    class SubmissionPackageInstallTask : public PackageInstallTask {
        private:
            class Impl;
        private:
            std::unique_ptr<Impl> impl;
        public:
            SubmissionPackageInstallTask();
            virtual ~SubmissionPackageInstallTask() override;

            Result Initialize(fs::FileHandle handle, StorageId storage_id, void *buffer, size_t buffer_size, bool ignore_ticket = false);
    };

}
