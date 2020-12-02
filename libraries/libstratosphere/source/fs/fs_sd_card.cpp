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
                    auto size = std::snprintf(dst, dst_size, "%s:", impl::SdCardFileSystemMountName);
                    AMS_ASSERT(static_cast<size_t>(size) == needed_size - 1);

                    return ResultSuccess();
                }
        };

    }

    SdCardDetectionEventNotifier::~SdCardDetectionEventNotifier() {
        if (this->notifier != nullptr) {
            fsEventNotifierClose(static_cast<FsEventNotifier *>(this->notifier));
        }
    }

    Result SdCardDetectionEventNotifier::GetEventHandle(os::SystemEvent *out_event, os::EventClearMode clear_mode) {
        AMS_ABORT_UNLESS(this->notifier != nullptr);

        ::Event event;
        R_TRY(fsEventNotifierGetEventHandle(static_cast<FsEventNotifier *>(this->notifier), std::addressof(event), clear_mode == os::EventClearMode_AutoClear));

        out_event->Attach(event.revent, true, svc::InvalidHandle, false, clear_mode);
        return ResultSuccess();
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

    bool IsSdCardInserted() {
        /* TODO: fs::DeviceOperator */
        /* Open a DeviceOperator. */
        ::FsDeviceOperator d;
        AMS_FS_R_ABORT_UNLESS(fsOpenDeviceOperator(std::addressof(d)));
        ON_SCOPE_EXIT { fsDeviceOperatorClose(std::addressof(d)); };

        bool sd_card_inserted;
        AMS_FS_R_ABORT_UNLESS(fsDeviceOperatorIsSdCardInserted(std::addressof(d), std::addressof(sd_card_inserted)));
        return sd_card_inserted;
    }

    Result OpenSdCardDetectionEventNotifier(SdCardDetectionEventNotifier *out) {
        auto internal_notifier = new (std::nothrow) FsEventNotifier;
        AMS_ABORT_UNLESS(internal_notifier != nullptr);
        auto session_guard = SCOPE_GUARD { delete internal_notifier; };

        R_TRY(fsOpenSdCardDetectionEventNotifier(internal_notifier));
        out->Open(static_cast<void*>(internal_notifier));

        session_guard.Cancel();
        return ResultSuccess();
    }

}
