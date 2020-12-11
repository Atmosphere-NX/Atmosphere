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

        class BisCommonMountNameGenerator : public fsa::ICommonMountNameGenerator, public impl::Newable {
            private:
                const BisPartitionId id;
            public:
                explicit BisCommonMountNameGenerator(BisPartitionId i) : id(i) { /* ... */ }

                virtual Result GenerateCommonMountName(char *dst, size_t dst_size) override {
                    /* Determine how much space we need. */
                    const char *bis_mount_name = GetBisMountName(this->id);
                    const size_t needed_size = strnlen(bis_mount_name, MountNameLengthMax) + 2;
                    AMS_ABORT_UNLESS(dst_size >= needed_size);

                    /* Generate the name. */
                    auto size = std::snprintf(dst, dst_size, "%s:", bis_mount_name);
                    AMS_ASSERT(static_cast<size_t>(size) == needed_size - 1);

                    return ResultSuccess();
                }
        };

    }

    namespace impl {

        Result MountBisImpl(const char *name, BisPartitionId id, const char *root_path) {
            /* Validate the mount name. */
            R_TRY(impl::CheckMountNameAllowingReserved(name));

            /* Open the partition. This uses libnx bindings. */
            /* Note: Nintendo ignores the root_path here. */
            FsFileSystem fs;
            R_TRY(fsOpenBisFileSystem(std::addressof(fs), static_cast<::FsBisPartitionId>(id), ""));

            /* Allocate a new mountname generator. */
            auto generator = std::make_unique<BisCommonMountNameGenerator>(id);
            R_UNLESS(generator != nullptr, fs::ResultAllocationFailureInBisA());

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<RemoteFileSystem>(fs);
            R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInBisB());

            /* Register. */
            return fsa::Register(name, std::move(fsa), std::move(generator));
        }

        Result SetBisRootForHostImpl(BisPartitionId id, const char *root_path) {
            /* Ensure the path isn't too long. */
            size_t len = strnlen(root_path, fs::EntryNameLengthMax + 1);
            R_UNLESS(len <= fs::EntryNameLengthMax, fs::ResultTooLongPath());

            fssrv::sf::Path sf_path;
            if (len > 0) {
                const bool ending_sep = PathNormalizer::IsSeparator(root_path[len - 1]);
                FspPathPrintf(std::addressof(sf_path), "%s%s", root_path, ending_sep ? "" : "/");
            } else {
                sf_path.str[0] = '\x00';
            }

            /* TODO: Libnx binding for fsSetBisRootForHost */
            AMS_ABORT();
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
       return impl::MountBisImpl(GetBisMountName(id), id, root_path);
   }

   Result MountBis(const char *name, BisPartitionId id) {
       return impl::MountBisImpl(name, id, nullptr);
   }

   void SetBisRootForHost(BisPartitionId id, const char *root_path) {
       R_ABORT_UNLESS(impl::SetBisRootForHostImpl(id, root_path));
   }

   Result OpenBisPartition(std::unique_ptr<fs::IStorage> *out, BisPartitionId id) {
        /* Open the partition. This uses libnx bindings. */
        FsStorage s;
        R_TRY(fsOpenBisStorage(std::addressof(s), static_cast<::FsBisPartitionId>(id)));

        /* Allocate a new storage wrapper. */
        auto storage = std::make_unique<RemoteStorage>(s);
        R_UNLESS(storage != nullptr, fs::ResultAllocationFailureInBisC());

        *out = std::move(storage);
        return ResultSuccess();
   }

   Result InvalidateBisCache() {
       /* TODO: Libnx binding for this command. */
       AMS_ABORT();
   }

}
