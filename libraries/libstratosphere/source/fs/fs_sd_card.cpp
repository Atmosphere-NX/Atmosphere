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
#include "impl/fs_event_notifier_object_adapter.hpp"

namespace ams::fs {

    namespace {

        constexpr inline const char AtmosphereErrorReportDirectory[] = "/atmosphere/erpt_reports";

        /* NOTE: Nintendo does not attach a generator to a mounted SD card filesystem. */
        /* However, it is desirable for homebrew to be able to access SD via common path. */
        class SdCardCommonMountNameGenerator : public fsa::ICommonMountNameGenerator, public impl::Newable {
            public:
                explicit SdCardCommonMountNameGenerator() { /* ... */ }

                virtual Result GenerateCommonMountName(char *dst, size_t dst_size) override {
                    /* Determine how much space we need. */
                    const size_t needed_size = strnlen(impl::SdCardFileSystemMountName, MountNameLengthMax) + 2;
                    AMS_ABORT_UNLESS(dst_size >= needed_size);

                    /* Generate the name. */
                    const auto size = util::SNPrintf(dst, dst_size, "%s:", impl::SdCardFileSystemMountName);
                    AMS_ASSERT(static_cast<size_t>(size) == needed_size - 1);
                    AMS_UNUSED(size);

                    return ResultSuccess();
                }
        };

    }

    Result MountSdCard(const char *name) {
        /* Validate the mount name. */
        R_TRY(impl::CheckMountNameAllowingReserved(name));

        /* Open the SD card. This uses libnx bindings. */
        FsFileSystem fs;
        R_TRY(fsOpenSdCardFileSystem(std::addressof(fs)));

        /* Allocate a new filesystem wrapper. */
        auto fsa = std::make_unique<RemoteFileSystem>(fs);
        R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInSdCardA());

        /* Allocate a new mountname generator. */
        /* NOTE: Nintendo does not attach a generator. */
        auto generator = std::make_unique<SdCardCommonMountNameGenerator>();
        R_UNLESS(generator != nullptr, fs::ResultAllocationFailureInSdCardA());

        /* Register. */
        return fsa::Register(name, std::move(fsa), std::move(generator));
    }

    Result MountSdCardErrorReportDirectoryForAtmosphere(const char *name) {
        /* Validate the mount name. */
        R_TRY(impl::CheckMountName(name));

        /* Open the SD card. This uses libnx bindings. */
        FsFileSystem fs;
        R_TRY(fsOpenSdCardFileSystem(std::addressof(fs)));

        /* Allocate a new filesystem wrapper. */
        std::unique_ptr<fsa::IFileSystem> fsa = std::make_unique<RemoteFileSystem>(fs);
        R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInSdCardA());

        /* Ensure that the error report directory exists. */
        R_TRY(fssystem::EnsureDirectoryRecursively(fsa.get(), AtmosphereErrorReportDirectory));

        /* Create a subdirectory filesystem. */
        auto subdir_fs = std::make_unique<fssystem::SubDirectoryFileSystem>(std::move(fsa), AtmosphereErrorReportDirectory);
        R_UNLESS(subdir_fs != nullptr, fs::ResultAllocationFailureInSdCardA());

        /* Register. */
        return fsa::Register(name, std::move(subdir_fs));
    }

    Result OpenSdCardDetectionEventNotifier(std::unique_ptr<IEventNotifier> *out) {
        /* Try to open an event notifier. */
        FsEventNotifier notifier;
        AMS_FS_R_TRY(fsOpenSdCardDetectionEventNotifier(std::addressof(notifier)));

        /* Create an event notifier adapter. */
        auto adapter = std::make_unique<impl::RemoteEventNotifierObjectAdapter>(notifier);
        R_UNLESS(adapter != nullptr, fs::ResultAllocationFailureInSdCardB());

        *out = std::move(adapter);
        return ResultSuccess();
    }

    bool IsSdCardInserted() {
        /* Open device operator. */
        FsDeviceOperator device_operator;
        AMS_FS_R_ABORT_UNLESS(::fsOpenDeviceOperator(std::addressof(device_operator)));
        ON_SCOPE_EXIT { ::fsDeviceOperatorClose(std::addressof(device_operator)); };

        /* Get SD card inserted. */
        bool inserted;
        AMS_FS_R_ABORT_UNLESS(::fsDeviceOperatorIsSdCardInserted(std::addressof(device_operator), std::addressof(inserted)));

        return inserted;
    }

}
