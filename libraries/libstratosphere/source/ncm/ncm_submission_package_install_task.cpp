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
#include <stratosphere.hpp>
#include "ncm_fs_utils.hpp"

namespace ams::ncm {

    class SubmissionPackageInstallTask::Impl {
        private:
            fs::FileHandleStorage m_storage;
            util::optional<impl::MountName> m_mount_name;
        public:
            explicit Impl(fs::FileHandle file) : m_storage(file), m_mount_name(util::nullopt) { /* ... */ }

            ~Impl() {
                if (m_mount_name) {
                    fs::fsa::Unregister(m_mount_name->str);
                }
            }

            Result Initialize() {
                AMS_ASSERT(!m_mount_name);

                /* Allocate a partition file system. */
                auto partition_file_system = std::make_unique<fssystem::PartitionFileSystem>();
                R_UNLESS(partition_file_system != nullptr, ncm::ResultAllocationFailed());

                /* Initialize the partition file system. */
                R_TRY(partition_file_system->Initialize(std::addressof(m_storage)));

                /* Create a mount name and register the file system. */
                auto mount_name = impl::CreateUniqueMountName();
                R_TRY(fs::fsa::Register(mount_name.str, std::move(partition_file_system)));

                /* Initialize members. */
                m_mount_name = mount_name;
                return ResultSuccess();
            }

            const impl::MountName &GetMountName() const {
                return *m_mount_name;
            }
    };

    SubmissionPackageInstallTask::SubmissionPackageInstallTask()  { /* ... */ }
    SubmissionPackageInstallTask::~SubmissionPackageInstallTask() { /* ... */ }

    Result SubmissionPackageInstallTask::Initialize(fs::FileHandle file, StorageId storage_id, void *buffer, size_t buffer_size, bool ignore_ticket) {
        AMS_ASSERT(!m_impl);

        /* Allocate impl. */
        m_impl.reset(new (std::nothrow) Impl(file));
        R_UNLESS(m_impl != nullptr, ncm::ResultAllocationFailed());

        /* Initialize impl. */
        R_TRY(m_impl->Initialize());

        /* Initialize parent. N doesn't check the result. */
        PackageInstallTask::Initialize(impl::GetRootDirectoryPath(m_impl->GetMountName()).str, storage_id, buffer, buffer_size, ignore_ticket);
        return ResultSuccess();
    }

}
