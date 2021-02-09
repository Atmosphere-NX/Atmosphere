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
#include "htc_htc_service_object.hpp"

namespace ams::htc::server {

    HtcServiceObject::HtcServiceObject(htclow::HtclowManager *htclow_manager) {
        AMS_ABORT("HtcServiceObject::HtcServiceObject");
    }


    HtcmiscImpl *HtcServiceObject::GetHtcmiscImpl() {
        AMS_ABORT("HtcServiceObject::GetHtcmiscImpl");
    }

    Result HtcServiceObject::GetEnvironmentVariable(sf::Out<s32> out_size, const sf::OutBuffer &out, const sf::InBuffer &name) {
        AMS_ABORT("HtcServiceObject::GetEnvironmentVariable");
    }

    Result HtcServiceObject::GetEnvironmentVariableLength(sf::Out<s32> out_size, const sf::InBuffer &name) {
        AMS_ABORT("HtcServiceObject::GetEnvironmentVariableLength");
    }

    Result HtcServiceObject::GetHostConnectionEvent(sf::OutCopyHandle out) {
        AMS_ABORT("HtcServiceObject::GetHostConnectionEvent");
    }

    Result HtcServiceObject::GetHostDisconnectionEvent(sf::OutCopyHandle out) {
        AMS_ABORT("HtcServiceObject::GetHostDisconnectionEvent");
    }

    Result HtcServiceObject::GetHostConnectionEventForSystem(sf::OutCopyHandle out) {
        AMS_ABORT("HtcServiceObject::GetHostConnectionEventForSystem");
    }

    Result HtcServiceObject::GetHostDisconnectionEventForSystem(sf::OutCopyHandle out) {
        AMS_ABORT("HtcServiceObject::GetHostDisconnectionEventForSystem");
    }

    Result HtcServiceObject::GetBridgeIpAddress(const sf::OutBuffer &out) {
        AMS_ABORT("HtcServiceObject::GetBridgeIpAddress");
    }

    Result HtcServiceObject::GetBridgePort(const sf::OutBuffer &out) {
        AMS_ABORT("HtcServiceObject::GetBridgePort");
    }

    Result HtcServiceObject::SetCradleAttached(bool attached) {
        AMS_ABORT("HtcServiceObject::SetCradleAttached");
    }

    Result HtcServiceObject::GetBridgeSubnetMask(const sf::OutBuffer &out) {
        AMS_ABORT("HtcServiceObject::GetBridgeSubnetMask");
    }

    Result HtcServiceObject::GetBridgeMacAddress(const sf::OutBuffer &out) {
        AMS_ABORT("HtcServiceObject::GetBridgeMacAddress");
    }

    Result HtcServiceObject::GetWorkingDirectoryPath(const sf::OutBuffer &out, s32 max_len) {
        AMS_ABORT("HtcServiceObject::GetWorkingDirectoryPath");
    }

    Result HtcServiceObject::GetWorkingDirectoryPathSize(sf::Out<s32> out_size) {
        AMS_ABORT("HtcServiceObject::GetWorkingDirectoryPathSize");
    }

    Result HtcServiceObject::RunOnHostStart(sf::Out<u32> out_id, sf::OutCopyHandle out, const sf::InBuffer &args) {
        AMS_ABORT("HtcServiceObject::RunOnHostStart");
    }

    Result HtcServiceObject::RunOnHostResults(sf::Out<s32> out_result, u32 id) {
        AMS_ABORT("HtcServiceObject::RunOnHostResults");
    }

    Result HtcServiceObject::SetBridgeIpAddress(const sf::InBuffer &arg) {
        AMS_ABORT("HtcServiceObject::SetBridgeIpAddress");
    }

    Result HtcServiceObject::SetBridgeSubnetMask(const sf::InBuffer &arg) {
        AMS_ABORT("HtcServiceObject::SetBridgeSubnetMask");
    }

    Result HtcServiceObject::SetBridgePort(const sf::InBuffer &arg) {
        AMS_ABORT("HtcServiceObject::SetBridgePort");
    }

}
