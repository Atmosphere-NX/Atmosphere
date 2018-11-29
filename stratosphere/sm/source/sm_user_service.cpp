/*
 * Copyright (c) 2018 Atmosphère-NX
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
    return 0;
}

Result UserService::GetService(Out<MovedHandle> out_h, SmServiceName service) {
    Handle session_h = 0;
    Result rc = 0x415;
    
#ifdef SM_ENABLE_SMHAX
    if (!this->has_initialized) {
        rc = Registration::GetServiceForPid(Registration::GetInitialProcessId(), smEncodeName(service.name), &session_h);
    }
#endif
    if (this->has_initialized) {
        rc = Registration::GetServiceForPid(this->pid, smEncodeName(service.name), &session_h);
    }
    
    if (R_SUCCEEDED(rc)) {
        out_h.SetValue(session_h);
    }
    return rc;
}

Result UserService::RegisterService(Out<MovedHandle> out_h, SmServiceName service, u32 max_sessions, bool is_light) {
    Handle service_h = 0;
    Result rc = 0x415;
#ifdef SM_ENABLE_SMHAX
    if (!this->has_initialized) {
        rc = Registration::RegisterServiceForPid(Registration::GetInitialProcessId(), smEncodeName(service.name), max_sessions, (is_light & 1) != 0, &service_h);
    }
#endif
    if (this->has_initialized) {
        rc = Registration::RegisterServiceForPid(this->pid, smEncodeName(service.name), max_sessions, (is_light & 1) != 0, &service_h);
    }
    
    if (R_SUCCEEDED(rc)) {
        out_h.SetValue(service_h);
    }
    return rc;
}

Result UserService::UnregisterService(SmServiceName service) {
    Result rc = 0x415;
#ifdef SM_ENABLE_SMHAX
    if (!this->has_initialized) {
        rc = Registration::UnregisterServiceForPid(Registration::GetInitialProcessId(), smEncodeName(service.name));
    }
#endif
    if (this->has_initialized) {
        rc = Registration::UnregisterServiceForPid(this->pid, smEncodeName(service.name));
    }
    return rc;
}

Result UserService::AtmosphereInstallMitm(Out<MovedHandle> srv_h, Out<MovedHandle> qry_h, SmServiceName service) {
    Handle service_h = 0;
    Handle query_h = 0;
    Result rc = 0x415;
    if (this->has_initialized) {
        rc = Registration::InstallMitmForPid(this->pid, smEncodeName(service.name), &service_h, &query_h);
    }
    
    if (R_SUCCEEDED(rc)) {
        srv_h.SetValue(service_h);
        qry_h.SetValue(query_h);
    }
    return rc;
}

Result UserService::AtmosphereUninstallMitm(SmServiceName service) {
    Result rc = 0x415;
    if (this->has_initialized) {
        rc = Registration::UninstallMitmForPid(this->pid, smEncodeName(service.name));
    }
    return rc;
}

Result UserService::AtmosphereAcknowledgeMitmSession(Out<u64> client_pid, Out<MovedHandle> fwd_h, SmServiceName service) {
    Result rc = 0x415;
    Handle out_fwd_h = 0;
    if (this->has_initialized) {
        rc = Registration::AcknowledgeMitmSessionForPid(this->pid, smEncodeName(service.name), &out_fwd_h, client_pid.GetPointer());
    }
    
    if (R_SUCCEEDED(rc)) {
        fwd_h.SetValue(out_fwd_h);
    }
    
    return rc;
}

Result UserService::AtmosphereAssociatePidTidForMitm(u64 pid, u64 tid) {
    Result rc = 0x415;
    if (this->has_initialized) {
        if (Registration::IsInitialProcess(pid)) {
            rc = 0x1015;
        } else {
            rc = Registration::AssociatePidTidForMitm(pid, tid);
        }
    }
    return rc;
}
