/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include <stratosphere/sm.hpp>
#include <stratosphere/ncm.hpp>

namespace ams::sm::impl {

    /* Process management. */
    Result RegisterProcess(os::ProcessId pid, ncm::TitleId tid, const void *acid_sac, size_t acid_sac_size, const void *aci_sac, size_t aci_sac_size);
    Result UnregisterProcess(os::ProcessId pid);

    /* Service management. */
    Result HasService(bool *out, ServiceName service);
    Result WaitService(ServiceName service);
    Result GetServiceHandle(Handle *out, os::ProcessId pid, ServiceName service);
    Result RegisterService(Handle *out, os::ProcessId pid, ServiceName service, size_t max_sessions, bool is_light);
    Result RegisterServiceForSelf(Handle *out, ServiceName service, size_t max_sessions);
    Result UnregisterService(os::ProcessId pid, ServiceName service);

    /* Mitm extensions. */
    Result HasMitm(bool *out, ServiceName service);
    Result WaitMitm(ServiceName service);
    Result InstallMitm(Handle *out, Handle *out_query, os::ProcessId pid, ServiceName service);
    Result UninstallMitm(os::ProcessId pid, ServiceName service);
    Result DeclareFutureMitm(os::ProcessId pid, ServiceName service);
    Result AcknowledgeMitmSession(os::ProcessId *out_pid, ncm::TitleId *out_tid, Handle *out_hnd, os::ProcessId pid, ServiceName service);

    /* Dmnt record extensions. */
    Result GetServiceRecord(ServiceRecord *out, ServiceName service);
    Result ListServiceRecords(ServiceRecord *out, u64 *out_count, u64 offset, u64 max_count);

    /* Deferral extension (works around FS bug). */
    Result EndInitialDefers();

}
