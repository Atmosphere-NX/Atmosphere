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

namespace ams::fs {

    Result GetMmcCid(void *dst, size_t size) {
        /* Check pre-conditions. */
        AMS_FS_R_UNLESS(dst != nullptr, fs::ResultNullptrArgument());

        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the cid. */
        AMS_FS_R_TRY(device_operator->GetMmcCid(sf::OutBuffer(dst, size), static_cast<s64>(size)));

        R_SUCCEED();
    }

    Result GetMmcSpeedMode(MmcSpeedMode *out) {
        /* Check pre-conditions. */
        AMS_FS_R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the speed mode. */
        s64 speed_mode = 0;
        AMS_FS_R_TRY(device_operator->GetMmcSpeedMode(std::addressof(speed_mode)));

        *out = static_cast<MmcSpeedMode>(speed_mode);
        R_SUCCEED();
    }

    Result GetMmcPatrolCount(u32 *out) {
        /* Check pre-conditions. */
        AMS_FS_R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the patrol count. */
        AMS_FS_R_TRY(device_operator->GetMmcPatrolCount(out));

        R_SUCCEED();
    }

    Result GetAndClearMmcErrorInfo(StorageErrorInfo *out_sei, size_t *out_log_size, char *out_log_buffer, size_t log_buffer_size) {
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
        AMS_FS_R_TRY(device_operator->GetAndClearMmcErrorInfo(out_sei, std::addressof(log_size), sf::OutBuffer(out_log_buffer, log_buffer_size), static_cast<s64>(log_buffer_size)));

        *out_log_size = static_cast<size_t>(log_size);
        R_SUCCEED();
    }

    Result GetMmcExtendedCsd(void *dst, size_t size) {
        /* Check pre-conditions. */
        AMS_FS_R_UNLESS(dst != nullptr, fs::ResultNullptrArgument());

        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the csd. */
        AMS_FS_R_TRY(device_operator->GetMmcExtendedCsd(sf::OutBuffer(dst, size), static_cast<s64>(size)));

        R_SUCCEED();
    }

}
