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
#include <stratosphere/servicesession.hpp>
#include "sm_user_service.hpp"
#include "sm_registration.hpp"

Result UserService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601; 
    switch ((UserServiceCmd)cmd_id) {
        case User_Cmd_Initialize:
            rc = WrapIpcCommandImpl<&UserService::initialize>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case User_Cmd_GetService:
            rc = WrapIpcCommandImpl<&UserService::get_service>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case User_Cmd_RegisterService:
            rc = WrapIpcCommandImpl<&UserService::register_service>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case User_Cmd_UnregisterService:
            rc = WrapIpcCommandImpl<&UserService::unregister_service>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
#ifdef SM_ENABLE_MITM
        case User_Cmd_AtmosphereInstallMitm:
            rc = WrapIpcCommandImpl<&UserService::install_mitm>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case User_Cmd_AtmosphereUninstallMitm:
            rc = WrapIpcCommandImpl<&UserService::uninstall_mitm>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case User_Cmd_AtmosphereAssociatePidTidForMitm:
            rc = WrapIpcCommandImpl<&UserService::associate_pid_tid_for_mitm>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
#endif
        default:
            break;
    }
    return rc;
}

Result UserService::handle_deferred() {
    /* If we're deferred, GetService failed. */
    return WrapDeferredIpcCommandImpl<&UserService::deferred_get_service>(this, this->deferred_service);
}


std::tuple<Result> UserService::initialize(PidDescriptor pid) {
    this->pid = pid.pid;
    this->has_initialized = true;
    return {0};
}

std::tuple<Result, MovedHandle> UserService::get_service(u64 service) {
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
    /* It's possible that this will end up deferring us...take that into account. */
    if (rc == RESULT_DEFER_SESSION) {
        this->deferred_service = service;
    }
    return {rc, MovedHandle{session_h}};
}

std::tuple<Result, MovedHandle> UserService::deferred_get_service(u64 service) {
    Handle session_h = 0;
    Result rc = Registration::GetServiceHandle(this->pid, service, &session_h);
    return {rc, MovedHandle{session_h}};
}

std::tuple<Result, MovedHandle> UserService::register_service(u64 service, u8 is_light, u32 max_sessions) {
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
    return {rc, MovedHandle{service_h}};
}

std::tuple<Result> UserService::unregister_service(u64 service) {
    Result rc = 0x415;
#ifdef SM_ENABLE_SMHAX
    if (!this->has_initialized) {
        rc = Registration::UnregisterServiceForPid(Registration::GetInitialProcessId(), service);
    }
#endif
    if (this->has_initialized) {
        rc = Registration::UnregisterServiceForPid(this->pid, service);
    }
    return {rc};
}

std::tuple<Result, MovedHandle, MovedHandle> UserService::install_mitm(u64 service) {
    Handle service_h = 0;
    Handle query_h = 0;
    Result rc = 0x415;
    if (this->has_initialized) {
        rc = Registration::InstallMitmForPid(this->pid, service, &service_h, &query_h);
    }
    return {rc, MovedHandle{service_h}, MovedHandle{query_h}};
}

std::tuple<Result> UserService::associate_pid_tid_for_mitm(u64 pid, u64 tid) {
    Result rc = 0x415;
    if (this->has_initialized) {
        if (Registration::IsInitialProcess(pid)) {
            rc = 0x1015;
        } else {
            rc = Registration::AssociatePidTidForMitm(pid, tid);
        }
    }
    return {rc};
}

std::tuple<Result> UserService::uninstall_mitm(u64 service) {
    Result rc = 0x415;
    if (this->has_initialized) {
        rc = Registration::UninstallMitmForPid(this->pid, service);
    }
    return {rc};
}