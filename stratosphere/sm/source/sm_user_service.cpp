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

Result UserService::GetService(Out<MovedHandle> out_h, u64 service) {
    Handle session_h = 0;
    Result rc = 0x415;
    
#ifdef SM_ENABLE_SMHAX
    if (!this->has_initialized) {
        rc = Registration::GetServiceForPid(Registration::GetInitialProcessId(), service, &session_h);
    }
#endif
    if (this->has_initialized) {
        rc = Registration::GetServiceForPid(this->pid, service, &session_h);
    }
    
    if (R_SUCCEEDED(rc)) {
        out_h.SetValue(session_h);
    }
    return rc;
}

Result UserService::RegisterService(Out<MovedHandle> out_h, u64 service, u8 is_light, u32 max_sessions) {
    Handle service_h = 0;
    Result rc = 0x415;
#ifdef SM_ENABLE_SMHAX
    if (!this->has_initialized) {
        rc = Registration::RegisterServiceForPid(Registration::GetInitialProcessId(), service, max_sessions, (is_light & 1) != 0, &service_h);
    }
#endif
    if (this->has_initialized) {
        rc = Registration::RegisterServiceForPid(this->pid, service, max_sessions, (is_light & 1) != 0, &service_h);
    }
    
    if (R_SUCCEEDED(rc)) {
        out_h.SetValue(service_h);
    }
    return rc;
}

Result UserService::UnregisterService(u64 service) {
    Result rc = 0x415;
#ifdef SM_ENABLE_SMHAX
    if (!this->has_initialized) {
        rc = Registration::UnregisterServiceForPid(Registration::GetInitialProcessId(), service);
    }
#endif
    if (this->has_initialized) {
        rc = Registration::UnregisterServiceForPid(this->pid, service);
    }
    return rc;
}

Result UserService::AtmosphereInstallMitm(Out<MovedHandle> srv_h, Out<MovedHandle> qry_h, u64 service) {
    Handle service_h = 0;
    Handle query_h = 0;
    Result rc = 0x415;
    if (this->has_initialized) {
        rc = Registration::InstallMitmForPid(this->pid, service, &service_h, &query_h);
    }
    
    if (R_SUCCEEDED(rc)) {
        srv_h.SetValue(service_h);
        qry_h.SetValue(query_h);
    }
    return rc;
}

Result UserService::AtmosphereUninstallMitm(u64 service) {
    Result rc = 0x415;
    if (this->has_initialized) {
        rc = Registration::UninstallMitmForPid(this->pid, service);
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
