/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

namespace ams::fs {

    namespace {

        class ContentStorageCommonMountNameGenerator : public fsa::ICommonMountNameGenerator, public impl::Newable {
            private:
                const ContentStorageId id;
            public:
                explicit ContentStorageCommonMountNameGenerator(ContentStorageId i) : id(i) { /* ... */ }

                virtual Result GenerateCommonMountName(char *dst, size_t dst_size) override {
                    /* Determine how much space we need. */
                    const size_t needed_size = strnlen(GetContentStorageMountName(id), MountNameLengthMax) + 2;
                    AMS_ABORT_UNLESS(dst_size >= needed_size);

                    /* Generate the name. */
                    auto size = std::snprintf(dst, dst_size, "%s:", GetContentStorageMountName(id));
                    AMS_ASSERT(static_cast<size_t>(size) == needed_size - 1);

                    return ResultSuccess();
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
        return MountContentStorage(GetContentStorageMountName(id), id);
    }

    Result MountContentStorage(const char *name, ContentStorageId id) {
        /* Validate the mount name. */
        R_TRY(impl::CheckMountNameAllowingReserved(name));

        /* It can take some time for the system partition to be ready (if it's on the SD card). */
        /* Thus, we will retry up to 10 times, waiting one second each time. */
        constexpr size_t MaxRetries  = 10;
        constexpr auto RetryInterval = TimeSpan::FromSeconds(1);

        /* Mount the content storage, use libnx bindings. */
        ::FsFileSystem fs;
        for (size_t i = 0; i < MaxRetries; i++) {
            /* Try to open the filesystem. */
            R_TRY_CATCH(fsOpenContentStorageFileSystem(std::addressof(fs), static_cast<::FsContentStorageId>(id))) {
                R_CATCH(fs::ResultSystemPartitionNotReady) {
                    if (i < MaxRetries - 1) {
                        os::SleepThread(RetryInterval);
                        continue;
                    } else {
                        return fs::ResultSystemPartitionNotReady();
                    }
                }
            } R_END_TRY_CATCH;

            /* The filesystem was opened successfully. */
            break;
        }

        /* Allocate a new filesystem wrapper. */
        auto fsa = std::make_unique<RemoteFileSystem>(fs);
        R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInContentStorageA());

        /* Allocate a new mountname generator. */
        auto generator = std::make_unique<ContentStorageCommonMountNameGenerator>(id);
        R_UNLESS(generator != nullptr, fs::ResultAllocationFailureInContentStorageB());

        /* Register. */
        return fsa::Register(name, std::move(fsa), std::move(generator));
    }

}
