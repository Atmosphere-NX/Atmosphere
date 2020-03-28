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

    class SubmissionPackageInstallTask::Impl {
        private:
            fs::FileHandleStorage storage;
            std::optional<impl::MountName> mount_name;
        public:
            explicit Impl(fs::FileHandle file) : storage(file), mount_name(std::nullopt) { /* ... */ }

            ~Impl() {
                if (this->mount_name) {
                    fs::fsa::Unregister(this->mount_name->str);
                }
            }

            Result Initialize() {
                AMS_ASSERT(!this->mount_name);

                /* Allocate a partition file system. */
                auto partition_file_system = std::make_unique<fssystem::PartitionFileSystem>();
                R_UNLESS(partition_file_system != nullptr, ncm::ResultAllocationFailed());

                /* Create a mount name and register the file system. */
                auto mount_name = impl::CreateUniqueMountName();
                R_TRY(fs::fsa::Register(mount_name.str, std::move(partition_file_system)));

                /* Initialize members. */
                this->mount_name = mount_name;
                return ResultSuccess();
            }

            const impl::MountName &GetMountName() const {
                return *this->mount_name;
            }
    };

    SubmissionPackageInstallTask::SubmissionPackageInstallTask()  { /* ... */ }
    SubmissionPackageInstallTask::~SubmissionPackageInstallTask() { /* ... */ }

    Result SubmissionPackageInstallTask::Initialize(fs::FileHandle file, StorageId storage_id, void *buffer, size_t buffer_size, bool ignore_ticket) {
        AMS_ASSERT(!this->impl);

        /* Allocate impl. */
        this->impl.reset(new (std::nothrow) Impl(file));
        R_UNLESS(this->impl != nullptr, ncm::ResultAllocationFailed());

        /* Initialize impl. */
        R_TRY(this->impl->Initialize());

        /* Initialize parent. N doesn't check the result. */
        PackageInstallTask::Initialize(impl::GetRootDirectoryPath(this->impl->GetMountName()).str, storage_id, buffer, buffer_size, ignore_ticket);
        return ResultSuccess();
    }

}
