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

#pragma once
#include <stratosphere.hpp>

namespace ams::sm::impl {

    /* Client disconnection callback. */
    void OnClientDisconnected(os::ProcessId process_id);

    /* Process management. */
    Result RegisterProcess(os::ProcessId process_id, ncm::ProgramId program_id, cfg::OverrideStatus override_status, const void *acid_sac, size_t acid_sac_size, const void *aci_sac, size_t aci_sac_size);
    Result UnregisterProcess(os::ProcessId process_id);

    /* Service management. */
    Result HasService(bool *out, ServiceName service);
    Result WaitService(ServiceName service);
    Result GetServiceHandle(os::NativeHandle *out, os::ProcessId process_id, ServiceName service);
    Result RegisterService(os::NativeHandle *out, os::ProcessId process_id, ServiceName service, size_t max_sessions, bool is_light);
    Result RegisterServiceForSelf(os::NativeHandle *out, ServiceName service, size_t max_sessions);
    Result UnregisterService(os::ProcessId process_id, ServiceName service);

    /* Mitm extensions. */
    Result HasMitm(bool *out, ServiceName service);
    Result WaitMitm(ServiceName service);
    Result InstallMitm(os::NativeHandle *out, os::NativeHandle *out_query, os::ProcessId process_id, ServiceName service);
    Result UninstallMitm(os::ProcessId process_id, ServiceName service);
    Result DeclareFutureMitm(os::ProcessId process_id, ServiceName service);
    Result ClearFutureMitm(os::ProcessId process_id, ServiceName service);
    Result AcknowledgeMitmSession(MitmProcessInfo *out_info, os::NativeHandle *out_hnd, os::ProcessId process_id, ServiceName service);

    /* Deferral extension (works around FS bug). */
    Result EndInitialDefers();

}
