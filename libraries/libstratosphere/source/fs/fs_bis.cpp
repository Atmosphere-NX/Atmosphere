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
#include "fsa/fs_mount_utils.hpp"
#include "impl/fs_file_system_service_object_adapter.hpp"
#include "impl/fs_file_system_proxy_service_object.hpp"

namespace ams::fs {

    namespace {

        class BisCommonMountNameGenerator : public fsa::ICommonMountNameGenerator, public impl::Newable {
            private:
                const BisPartitionId m_id;
            public:
                explicit BisCommonMountNameGenerator(BisPartitionId i) : m_id(i) { /* ... */ }

                virtual Result GenerateCommonMountName(char *dst, size_t dst_size) override {
                    /* Determine how much space we need. */
                    const char *bis_mount_name = GetBisMountName(m_id);
                    const size_t needed_size = util::Strnlen(bis_mount_name, MountNameLengthMax) + 2;
                    AMS_ABORT_UNLESS(dst_size >= needed_size);

                    /* Generate the name. */
                    const auto size = util::SNPrintf(dst, dst_size, "%s:", bis_mount_name);
                    AMS_ASSERT(static_cast<size_t>(size) == needed_size - 1);
                    AMS_UNUSED(size);

                    R_SUCCEED();
                }
        };

    }

    namespace impl {

        Result MountBisImpl(const char *name, BisPartitionId id, const char *root_path) {
            auto mount_impl = [=]() -> Result {
                /* Validate the mount name. */
                R_TRY(impl::CheckMountNameAllowingReserved(name));

                /* Convert the path for ipc. */
                /* NOTE: Nintendo ignores the root_path here. */
                fssrv::sf::FspPath sf_path;
                sf_path.str[0] = '\x00';
                AMS_UNUSED(root_path);

                /* Open the filesystem. */
                auto fsp = impl::GetFileSystemProxyServiceObject();
                sf::SharedPointer<fssrv::sf::IFileSystem> fs;
                R_TRY(fsp->OpenBisFileSystem(std::addressof(fs), sf_path, static_cast<u32>(id)));

                /* Allocate a new mountname generator. */
                auto generator = std::make_unique<BisCommonMountNameGenerator>(id);
                R_UNLESS(generator != nullptr, fs::ResultAllocationMemoryFailedInBisA());

                /* Allocate a new filesystem wrapper. */
                auto fsa = std::make_unique<impl::FileSystemServiceObjectAdapter>(std::move(fs));
                R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInBisB());

                /* Register. */
                R_RETURN(fsa::Register(name, std::move(fsa), std::move(generator)));
            };

            /* Perform the mount. */
            AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(mount_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_BIS(name, id, root_path)));

            /* Enable access logging. */
            AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);

            R_SUCCEED();
        }

        Result SetBisRootForHostImpl(BisPartitionId id, const char *root_path) {
            AMS_UNUSED(id, root_path);
            AMS_ABORT("TODO");
        }

    }

   const char *GetBisMountName(BisPartitionId id) {
       switch (id) {
           case BisPartitionId::CalibrationFile: return impl::BisCalibrationFilePartitionMountName;
           case BisPartitionId::SafeMode:        return impl::BisSafeModePartitionMountName;
           case BisPartitionId::User:            return impl::BisUserPartitionMountName;
           case BisPartitionId::System:          return impl::BisSystemPartitionMountName;
           AMS_UNREACHABLE_DEFAULT_CASE();
       }
   }

   Result MountBis(BisPartitionId id, const char *root_path) {
       R_RETURN(impl::MountBisImpl(GetBisMountName(id), id, root_path));
   }

   Result MountBis(const char *name, BisPartitionId id) {
       R_RETURN(impl::MountBisImpl(name, id, nullptr));
   }

   void SetBisRootForHost(BisPartitionId id, const char *root_path) {
       R_ABORT_UNLESS(impl::SetBisRootForHostImpl(id, root_path));
   }

   Result OpenBisPartition(std::unique_ptr<fs::IStorage> *out, BisPartitionId id) {
        /* Open the partition. */
        auto fsp = impl::GetFileSystemProxyServiceObject();
        sf::SharedPointer<fssrv::sf::IStorage> s;
        AMS_FS_R_TRY(fsp->OpenBisStorage(std::addressof(s), static_cast<u32>(id)));

        /* Allocate a new storage wrapper. */
        auto storage = std::make_unique<impl::StorageServiceObjectAdapter<fssrv::sf::IStorage>>(std::move(s));
        AMS_FS_R_UNLESS(storage != nullptr, fs::ResultAllocationMemoryFailedInBisC());

        *out = std::move(storage);
        R_SUCCEED();
   }

   Result InvalidateBisCache() {
       auto fsp = impl::GetFileSystemProxyServiceObject();
       AMS_FS_R_ABORT_UNLESS(fsp->InvalidateBisCache());
       R_SUCCEED();
   }

}
