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
#include "impl/fs_file_system_proxy_service_object.hpp"
#include "impl/fs_file_system_service_object_adapter.hpp"
#include "impl/fs_event_notifier_service_object_adapter.hpp"

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
                    const size_t needed_size = util::Strnlen(impl::SdCardFileSystemMountName, MountNameLengthMax) + 2;
                    AMS_ABORT_UNLESS(dst_size >= needed_size);

                    /* Generate the name. */
                    const auto size = util::SNPrintf(dst, dst_size, "%s:", impl::SdCardFileSystemMountName);
                    AMS_ASSERT(static_cast<size_t>(size) == needed_size - 1);
                    AMS_UNUSED(size);

                    R_SUCCEED();
                }
        };

    }

    Result MountSdCard(const char *name) {
        /* Validate the mount name. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT_UNLESS_R_SUCCEEDED(impl::CheckMountNameAllowingReserved(name), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT, name));

        /* Open the SD card filesystem. */
        auto fsp = impl::GetFileSystemProxyServiceObject();
        sf::SharedPointer<fssrv::sf::IFileSystem> fs;
        R_TRY(fsp->OpenSdCardFileSystem(std::addressof(fs)));

        /* Allocate a new filesystem wrapper. */
        auto fsa = std::make_unique<impl::FileSystemServiceObjectAdapter>(std::move(fs));
        R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInSdCardA());

        /* Allocate a new mountname generator. */
        /* NOTE: Nintendo does not attach a generator. */
        auto generator = std::make_unique<SdCardCommonMountNameGenerator>();
        R_UNLESS(generator != nullptr, fs::ResultAllocationMemoryFailedInSdCardA());

        /* Register. */
        R_RETURN(fsa::Register(name, std::move(fsa), std::move(generator)));
    }

    Result MountSdCardErrorReportDirectoryForAtmosphere(const char *name) {
        /* Validate the mount name. */
        R_TRY(impl::CheckMountName(name));

        /* Open the SD card filesystem. */
        auto fsp = impl::GetFileSystemProxyServiceObject();
        sf::SharedPointer<fssrv::sf::IFileSystem> fs;
        R_TRY(fsp->OpenSdCardFileSystem(std::addressof(fs)));

        /* Allocate a new filesystem wrapper. */
        auto fsa = fs::AllocateShared<impl::FileSystemServiceObjectAdapter>(std::move(fs));
        R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInSdCardA());

        /* Ensure that the error report directory exists. */
        constexpr fs::Path fs_path = fs::MakeConstantPath(AtmosphereErrorReportDirectory);
        R_TRY(fssystem::EnsureDirectory(fsa.get(), fs_path));

        /* Create a subdirectory filesystem. */
        auto subdir_fs = std::make_unique<fssystem::SubDirectoryFileSystem>(std::move(fsa));
        R_UNLESS(subdir_fs != nullptr, fs::ResultAllocationMemoryFailedInSdCardA());
        R_TRY(subdir_fs->Initialize(fs_path));

        /* Register. */
        R_RETURN(fsa::Register(name, std::move(subdir_fs)));
    }

    Result OpenSdCardDetectionEventNotifier(std::unique_ptr<IEventNotifier> *out) {
        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Try to open an event notifier. */
        sf::SharedPointer<fssrv::sf::IEventNotifier> notifier;
        AMS_FS_R_TRY(fsp->OpenSdCardDetectionEventNotifier(std::addressof(notifier)));

        /* Create an event notifier adapter. */
        auto adapter = std::make_unique<impl::EventNotifierObjectAdapter>(std::move(notifier));
        AMS_FS_R_UNLESS(adapter != nullptr, fs::ResultAllocationMemoryFailedInSdCardB());

        *out = std::move(adapter);
        R_SUCCEED();
    }

    bool IsSdCardInserted() {
        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_ABORT_UNLESS(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get insertion status. */
        bool inserted;
        AMS_FS_R_ABORT_UNLESS(device_operator->IsSdCardInserted(std::addressof(inserted)));

        return inserted;
    }

    Result GetSdCardSpeedMode(SdCardSpeedMode *out) {
        /* Check pre-conditions. */
        AMS_FS_R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the speed mode. */
        s64 speed_mode = 0;
        AMS_FS_R_TRY(device_operator->GetSdCardSpeedMode(std::addressof(speed_mode)));

        *out = static_cast<SdCardSpeedMode>(speed_mode);
        R_SUCCEED();
    }

    Result GetSdCardCid(void *dst, size_t size) {
        /* Check pre-conditions. */
        AMS_FS_R_UNLESS(dst != nullptr, fs::ResultNullptrArgument());

        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the cid. */
        AMS_FS_R_TRY(device_operator->GetSdCardCid(sf::OutBuffer(dst, size), static_cast<s64>(size)));

        R_SUCCEED();
    }

    Result GetSdCardUserAreaSize(s64 *out) {
        /* Check pre-conditions. */
        AMS_FS_R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the size. */
        AMS_FS_R_TRY(device_operator->GetSdCardUserAreaSize(out));

        R_SUCCEED();
    }

    Result GetSdCardProtectedAreaSize(s64 *out) {
        /* Check pre-conditions. */
        AMS_FS_R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the size. */
        AMS_FS_R_TRY(device_operator->GetSdCardProtectedAreaSize(out));

        R_SUCCEED();
    }

    Result GetAndClearSdCardErrorInfo(StorageErrorInfo *out_sei, size_t *out_log_size, char *out_log_buffer, size_t log_buffer_size) {
        /* Check pre-conditions. */
        AMS_FS_R_UNLESS(out_sei != nullptr,        fs::ResultNullptrArgument());
        AMS_FS_R_UNLESS(out_log_size != nullptr,   fs::ResultNullptrArgument());
        AMS_FS_R_UNLESS(out_log_buffer != nullptr, fs::ResultNullptrArgument());

        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the error info. */
        s64 log_size = 0;
        AMS_FS_R_TRY(device_operator->GetAndClearSdCardErrorInfo(out_sei, std::addressof(log_size), sf::OutBuffer(out_log_buffer, log_buffer_size), static_cast<s64>(log_buffer_size)));

        *out_log_size = static_cast<size_t>(log_size);
        R_SUCCEED();
    }

}
