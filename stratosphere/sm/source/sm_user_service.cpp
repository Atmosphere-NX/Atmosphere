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

#include <switch.h>
#include <stratosphere.hpp>
#include "sm_user_service.hpp"
#include "sm_registration.hpp"

Result UserService::Initialize(PidDescriptor pid) {
    this->pid = pid.pid;
    this->has_initialized = true;
    return ResultSuccess;
}

Result UserService::GetService(Out<MovedHandle> out_h, SmServiceName service) {
    Handle session_h = 0;

    if (!this->has_initialized) {
        return ResultSmInvalidClient;
    }

    R_TRY(Registration::GetServiceForPid(this->pid, smEncodeName(service.name), &session_h));

    out_h.SetValue(session_h);
    return ResultSuccess;
}

Result UserService::RegisterService(Out<MovedHandle> out_h, SmServiceName service, u32 max_sessions, bool is_light) {
    Handle service_h = 0;

    if (!this->has_initialized) {
        return ResultSmInvalidClient;
    }

    R_TRY(Registration::RegisterServiceForPid(this->pid, smEncodeName(service.name), max_sessions, (is_light & 1) != 0, &service_h));

    out_h.SetValue(service_h);
    return ResultSuccess;
}

Result UserService::UnregisterService(SmServiceName service) {
    if (!this->has_initialized) {
        return ResultSmInvalidClient;
    }

    return Registration::UnregisterServiceForPid(this->pid, smEncodeName(service.name));
}

Result UserService::AtmosphereInstallMitm(Out<MovedHandle> srv_h, Out<MovedHandle> qry_h, SmServiceName service) {
    Handle service_h = 0;
    Handle query_h = 0;

    if (!this->has_initialized) {
        return ResultSmInvalidClient;
    }

    R_TRY(Registration::InstallMitmForPid(this->pid, smEncodeName(service.name), &service_h, &query_h));

    srv_h.SetValue(service_h);
    qry_h.SetValue(query_h);
    return ResultSuccess;
}

Result UserService::AtmosphereUninstallMitm(SmServiceName service) {
    if (!this->has_initialized) {
        return ResultSmInvalidClient;
    }

    return Registration::UninstallMitmForPid(this->pid, smEncodeName(service.name));
}

Result UserService::AtmosphereAcknowledgeMitmSession(Out<u64> client_pid, Out<MovedHandle> fwd_h, SmServiceName service) {
    Handle out_fwd_h = 0;

    if (!this->has_initialized) {
        return ResultSmInvalidClient;
    }

    R_TRY(Registration::AcknowledgeMitmSessionForPid(this->pid, smEncodeName(service.name), &out_fwd_h, client_pid.GetPointer()));

    fwd_h.SetValue(out_fwd_h);
    return ResultSuccess;
}

Result UserService::AtmosphereAssociatePidTidForMitm(u64 pid, u64 tid) {
    if (!this->has_initialized) {
        return ResultSmInvalidClient;
    }

    if (Registration::IsInitialProcess(pid)) {
        return ResultSmNotAllowed;
    }

    return Registration::AssociatePidTidForMitm(pid, tid);
}
