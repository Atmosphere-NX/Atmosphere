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
#include "htc_htc_service_object.hpp"
#include "../../htcfs/htcfs_working_directory.hpp"

namespace ams::htc::server {

    HtcServiceObject::HtcServiceObject(htclow::HtclowManager *htclow_manager) : m_set(), m_misc_impl(htclow_manager), m_observer(m_misc_impl), m_mutex(){
        /* Initialize our set. */
        m_set.Initialize(MaxSetElements, m_set_memory, sizeof(m_set_memory));
    }


    HtcmiscImpl *HtcServiceObject::GetHtcmiscImpl() {
        return std::addressof(m_misc_impl);
    }

    Result HtcServiceObject::GetEnvironmentVariable(sf::Out<s32> out_size, const sf::OutBuffer &out, const sf::InBuffer &name) {
        /* Get the variable. */
        size_t var_size = std::numeric_limits<size_t>::max();
        R_TRY(m_misc_impl.GetEnvironmentVariable(std::addressof(var_size), reinterpret_cast<char *>(out.GetPointer()), out.GetSize(), reinterpret_cast<const char *>(name.GetPointer()), name.GetSize()));

        /* Check the output size. */
        R_UNLESS(util::IsIntValueRepresentable<s32>(var_size), htc::ResultUnknown());

        /* Set the output size. */
        *out_size = static_cast<s32>(var_size);
        return ResultSuccess();
    }

    Result HtcServiceObject::GetEnvironmentVariableLength(sf::Out<s32> out_size, const sf::InBuffer &name) {
        /* Get the variable. */
        size_t var_size = std::numeric_limits<size_t>::max();
        R_TRY(m_misc_impl.GetEnvironmentVariableLength(std::addressof(var_size), reinterpret_cast<const char *>(name.GetPointer()), name.GetSize()));

        /* Check the output size. */
        R_UNLESS(util::IsIntValueRepresentable<s32>(var_size), htc::ResultUnknown());

        /* Set the output size. */
        *out_size = static_cast<s32>(var_size);
        return ResultSuccess();
    }

    Result HtcServiceObject::GetHostConnectionEvent(sf::OutCopyHandle out) {
        /* Set the output handle. */
        out.SetValue(m_observer.GetConnectEvent()->GetReadableHandle(), false);
        return ResultSuccess();
    }

    Result HtcServiceObject::GetHostDisconnectionEvent(sf::OutCopyHandle out) {
        /* Set the output handle. */
        out.SetValue(m_observer.GetDisconnectEvent()->GetReadableHandle(), false);
        return ResultSuccess();
    }

    Result HtcServiceObject::GetHostConnectionEventForSystem(sf::OutCopyHandle out) {
        /* NOTE: Nintendo presumably reserved this command in case they need it, but they haven't implemented it yet. */
        AMS_UNUSED(out);
        AMS_ABORT("HostEventForSystem not implemented.");
    }

    Result HtcServiceObject::GetHostDisconnectionEventForSystem(sf::OutCopyHandle out) {
        /* NOTE: Nintendo presumably reserved this command in case they need it, but they haven't implemented it yet. */
        AMS_UNUSED(out);
        AMS_ABORT("HostEventForSystem not implemented.");
    }

    Result HtcServiceObject::GetWorkingDirectoryPath(const sf::OutBuffer &out, s32 max_len) {
        return htcfs::GetWorkingDirectory(reinterpret_cast<char *>(out.GetPointer()), max_len);
    }

    Result HtcServiceObject::GetWorkingDirectoryPathSize(sf::Out<s32> out_size) {
        return htcfs::GetWorkingDirectorySize(out_size.GetPointer());
    }

    Result HtcServiceObject::RunOnHostStart(sf::Out<u32> out_id, sf::OutCopyHandle out, const sf::InBuffer &args) {
        /* Begin the run on host task. */
        os::NativeHandle event_handle;
        R_TRY(m_misc_impl.RunOnHostBegin(out_id.GetPointer(), std::addressof(event_handle), reinterpret_cast<const char *>(args.GetPointer()), args.GetSize()));

        /* Add the task id to our set. */
        {
            std::scoped_lock lk(m_mutex);
            m_set.insert(*out_id);
        }

        /* Set the output event. */
        out.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result HtcServiceObject::RunOnHostResults(sf::Out<s32> out_result, u32 id) {
        /* Verify that we have the task. */
        {
            std::scoped_lock lk(m_mutex);
            R_UNLESS(m_set.erase(id), htc::ResultInvalidTaskId());
        }

        /* Finish the run on host task. */
        return m_misc_impl.RunOnHostEnd(out_result.GetPointer(), id);
    }

    Result HtcServiceObject::GetBridgeIpAddress(const sf::OutBuffer &out) {
        /* NOTE: Atmosphere does not support HostBridge, and it's unclear if we ever will. */
        AMS_UNUSED(out);
        AMS_ABORT("HostBridge currently not supported.");
    }

    Result HtcServiceObject::GetBridgePort(const sf::OutBuffer &out) {
        /* NOTE: Atmosphere does not support HostBridge, and it's unclear if we ever will. */
        AMS_UNUSED(out);
        AMS_ABORT("HostBridge currently not supported.");
    }

    Result HtcServiceObject::SetCradleAttached(bool attached) {
        /* NOTE: Atmosphere does not support HostBridge, and it's unclear if we ever will. */
        AMS_UNUSED(attached);
        AMS_ABORT("HostBridge currently not supported.");
    }

    Result HtcServiceObject::GetBridgeSubnetMask(const sf::OutBuffer &out) {
        /* NOTE: Atmosphere does not support HostBridge, and it's unclear if we ever will. */
        AMS_UNUSED(out);
        AMS_ABORT("HostBridge currently not supported.");
    }

    Result HtcServiceObject::GetBridgeMacAddress(const sf::OutBuffer &out) {
        /* NOTE: Atmosphere does not support HostBridge, and it's unclear if we ever will. */
        AMS_UNUSED(out);
        AMS_ABORT("HostBridge currently not supported.");
    }

    Result HtcServiceObject::SetBridgeIpAddress(const sf::InBuffer &arg) {
        /* NOTE: Atmosphere does not support HostBridge, and it's unclear if we ever will. */
        AMS_UNUSED(arg);
        AMS_ABORT("HostBridge currently not supported.");
    }

    Result HtcServiceObject::SetBridgeSubnetMask(const sf::InBuffer &arg) {
        /* NOTE: Atmosphere does not support HostBridge, and it's unclear if we ever will. */
        AMS_UNUSED(arg);
        AMS_ABORT("HostBridge currently not supported.");
    }

    Result HtcServiceObject::SetBridgePort(const sf::InBuffer &arg) {
        /* NOTE: Atmosphere does not support HostBridge, and it's unclear if we ever will. */
        AMS_UNUSED(arg);
        AMS_ABORT("HostBridge currently not supported.");
    }

}
