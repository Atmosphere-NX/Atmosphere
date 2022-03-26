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

        class ContentStorageCommonMountNameGenerator : public fsa::ICommonMountNameGenerator, public impl::Newable {
            private:
                const ContentStorageId id;
            public:
                explicit ContentStorageCommonMountNameGenerator(ContentStorageId i) : id(i) { /* ... */ }

                virtual Result GenerateCommonMountName(char *dst, size_t dst_size) override {
                    /* Determine how much space we need. */
                    const size_t needed_size = util::Strnlen(GetContentStorageMountName(id), MountNameLengthMax) + 2;
                    AMS_ABORT_UNLESS(dst_size >= needed_size);

                    /* Generate the name. */
                    const auto size = util::SNPrintf(dst, dst_size, "%s:", GetContentStorageMountName(id));
                    AMS_ASSERT(static_cast<size_t>(size) == needed_size - 1);
                    AMS_UNUSED(size);

                    R_SUCCEED();
                }
        };

    }

    const char *GetContentStorageMountName(ContentStorageId id) {
        switch (id) {
            case ContentStorageId::System: return impl::ContentStorageSystemMountName;
            case ContentStorageId::User:   return impl::ContentStorageUserMountName;
            case ContentStorageId::SdCard: return impl::ContentStorageSdCardMountName;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    Result MountContentStorage(ContentStorageId id) {
        R_RETURN(MountContentStorage(GetContentStorageMountName(id), id));
    }

    Result MountContentStorage(const char *name, ContentStorageId id) {
        auto mount_impl = [=]() -> Result {
            /* Validate the mount name. */
            R_TRY(impl::CheckMountNameAllowingReserved(name));

            /* It can take some time for the system partition to be ready (if it's on the SD card). */
            /* Thus, we will retry up to 10 times, waiting one second each time. */
            constexpr size_t MaxRetries  = 10;
            constexpr auto RetryInterval = TimeSpan::FromSeconds(1);

            /* Mount the content storage */
            auto fsp = impl::GetFileSystemProxyServiceObject();

            sf::SharedPointer<fssrv::sf::IFileSystem> fs;
            for (size_t i = 0; i < MaxRetries; i++) {
                /* Try to open the filesystem. */
                R_TRY_CATCH(fsp->OpenContentStorageFileSystem(std::addressof(fs), static_cast<u32>(id))) {
                    R_CATCH(fs::ResultSystemPartitionNotReady) {
                        if (i < MaxRetries - 1) {
                            os::SleepThread(RetryInterval);
                            continue;
                        } else {
                            R_THROW(fs::ResultSystemPartitionNotReady());
                        }
                    }
                } R_END_TRY_CATCH;

                /* The filesystem was opened successfully. */
                break;
            }

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<impl::FileSystemServiceObjectAdapter>(std::move(fs));
            R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInContentStorageA());

            /* Allocate a new mountname generator. */
            auto generator = std::make_unique<ContentStorageCommonMountNameGenerator>(id);
            R_UNLESS(generator != nullptr, fs::ResultAllocationMemoryFailedInContentStorageB());

            /* Register. */
            R_RETURN(fsa::Register(name, std::move(fsa), std::move(generator)));
        };

        /* Perform the mount. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(mount_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CONTENT_STORAGE(name, id)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

}
