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
#include <algorithm>
#include <stratosphere.hpp>
#include "sm_registration.hpp"
#include "meta_tools.hpp"

static std::array<Registration::Process, REGISTRATION_LIST_MAX_PROCESS> g_process_list = {0};
static std::array<Registration::Service, REGISTRATION_LIST_MAX_SERVICE> g_service_list = {0};

static u64 g_initial_process_id_low = 0;
static u64 g_initial_process_id_high = 0;
static bool g_determined_initial_process_ids = false;
static bool g_end_init_defers = false;

u64 GetServiceNameLength(u64 service) {
    u64 service_name_len = 0;
    while (service & 0xFF) {
        service_name_len++;
        service >>= 8;
    }
    return service_name_len;
}

/* Atmosphere extension utilities. */
void Registration::EndInitDefers() {
    g_end_init_defers = true;
}

constexpr u64 EncodeNameConstant(const char *name) {
    u64 service = 0;
    for (unsigned int i = 0; i < sizeof(service); i++) {
        if (name[i] == '\x00') {
            break;
        }
        service |= ((u64)name[i]) << (8 * i);
    }
    return service;
}

bool Registration::ShouldInitDefer(u64 service) {
    /* Only enable if compile-time generated. */
#ifndef SM_ENABLE_INIT_DEFERS
    return false;
#endif

    if (g_end_init_defers) {
        return false;
    }
    
    /* This is a mechanism by which certain services will always be deferred until sm:m receives a special command. */
    /* This can be extended with more services as needed at a later date. */
    constexpr u64 FSP_SRV = EncodeNameConstant("fsp-srv");
    return service == FSP_SRV;
}

/* Utilities. */
Registration::Process *Registration::GetProcessForPid(u64 pid) {
    auto process_it = std::find_if(g_process_list.begin(), g_process_list.end(), member_equals_fn(&Process::pid, pid));
    if (process_it == g_process_list.end()) {
        return nullptr;
    }
    return &*process_it;
}

Registration::Process *Registration::GetFreeProcess() {
    return GetProcessForPid(0);
}

Registration::Service *Registration::GetService(u64 service_name) {
    auto service_it = std::find_if(g_service_list.begin(), g_service_list.end(), member_equals_fn(&Service::service_name, service_name));
    if (service_it == g_service_list.end()) {
        return nullptr;
    }
    return &*service_it;
}

Registration::Service *Registration::GetFreeService() {
    return GetService(0);
}

bool Registration::IsValidForSac(u8 *sac, size_t sac_size, u64 service, bool is_host) {
    u8 cur_ctrl;
    u64 cur_service;
    u64 service_for_compare;
    bool cur_is_host;
    size_t remaining = sac_size;
    while (remaining) {
        cur_ctrl = *sac++;
        remaining--;
        size_t cur_size = (cur_ctrl & 7) + 1;
        if (cur_size > remaining) {
            break;
        }
        cur_is_host = (cur_ctrl & 0x80) != 0;
        cur_service = 0;
        for (unsigned int i = 0; i < cur_size; i++) {
            cur_service |= ((u64)sac[i]) << (8 * i);
        }
        /* Check if the last byte is a wildcard ('*') */
        service_for_compare = service;
        if (sac[cur_size - 1] == '*') {
            u64 mask = U64_MAX;
            for (unsigned int i = 0; i < 8 - (cur_size - 1); i++) {
                mask >>= 8;
            }
            /* Mask cur_service, service with 0xFF.. up until the wildcard. */
            cur_service &= mask;
            service_for_compare &= mask;
        }
        if (cur_service == service_for_compare && (is_host == cur_is_host)) {
            return true;
        }
        sac += cur_size;
        remaining -= cur_size;
    }
    return false;
}


bool Registration::ValidateSacAgainstRestriction(u8 *r_sac, size_t r_sac_size, u8 *sac, size_t sac_size) {
    u8 cur_ctrl;
    u64 cur_service;
    bool cur_is_host;
    size_t remaining = sac_size;
    while (remaining) {
        cur_ctrl = *sac++;
        remaining--;
        size_t cur_size = (cur_ctrl & 7) + 1;
        if (cur_size > remaining) {
            break;
        }
        cur_is_host = (cur_ctrl & 0x80) != 0;
        cur_service = 0;
        for (unsigned int i = 0; i < cur_size; i++) {
            cur_service |= ((u64)sac[i]) << (8 * i);
        }
        if (!IsValidForSac(r_sac, r_sac_size, cur_service, cur_is_host)) {
            return false;
        }
        sac += cur_size;
        remaining -= cur_size;
    }
    return true;
}

void Registration::CacheInitialProcessIdLimits() {
    if (g_determined_initial_process_ids) {
        return;
    }
    if (kernelAbove500()) {
        svcGetSystemInfo(&g_initial_process_id_low, 2, 0, 0);
        svcGetSystemInfo(&g_initial_process_id_high, 2, 0, 1);
    } else {
        g_initial_process_id_low = 0;
        g_initial_process_id_high = REGISTRATION_INITIAL_PID_MAX;
    }
    g_determined_initial_process_ids = true;
}

bool Registration::IsInitialProcess(u64 pid) {
    CacheInitialProcessIdLimits();
    return g_initial_process_id_low <= pid && pid <= g_initial_process_id_high;
}

u64 Registration::GetInitialProcessId() {
    CacheInitialProcessIdLimits();
    if (IsInitialProcess(1)) {
        return 1;
    }
    return g_initial_process_id_low;
}

/* Process management. */
Result Registration::RegisterProcess(u64 pid, u8 *acid_sac, size_t acid_sac_size, u8 *aci0_sac, size_t aci0_sac_size) {
    if (aci0_sac_size > REGISTRATION_MAX_SAC_SIZE) {
        return ResultSmTooLargeAccessControl;
    }

    Registration::Process *proc = GetFreeProcess();
    if (proc == NULL) {
        return ResultSmInsufficientProcesses;
    }
    
    if (aci0_sac_size && !ValidateSacAgainstRestriction(acid_sac, acid_sac_size, aci0_sac, aci0_sac_size)) {
        return ResultSmNotAllowed;
    }
    
    proc->pid = pid;
    proc->sac_size = aci0_sac_size;
    std::copy(aci0_sac, aci0_sac + aci0_sac_size, proc->sac);
    return ResultSuccess;
}

Result Registration::UnregisterProcess(u64 pid) {
    Registration::Process *proc = GetProcessForPid(pid);
    if (proc == NULL) {
        return ResultSmInvalidClient;
    }
    
    proc->pid = 0;
    return ResultSuccess;
}

/* Service management. */
bool Registration::HasService(u64 service) {
    return std::any_of(g_service_list.begin(), g_service_list.end(), member_equals_fn(&Service::service_name, service));
}

bool Registration::HasMitm(u64 service) {
    Registration::Service *target_service = GetService(service);
    return target_service != NULL && target_service->mitm_pid != 0;
}

Result Registration::GetServiceHandle(u64 pid, u64 service, Handle *out) {
    Registration::Service *target_service = GetService(service);
    if (target_service == NULL || ShouldInitDefer(service) || target_service->mitm_waiting_ack) {
        /* Note: This defers the result until later. */
        return ResultServiceFrameworkRequestDeferredByUser;
    }
    
    *out = 0;
    Result rc;
    if (target_service->mitm_pid == 0 || target_service->mitm_pid == pid) {
        rc = svcConnectToPort(out, target_service->port_h);
    } else {
        IpcCommand c;
        ipcInitialize(&c);
        struct {
            u64 magic;
            u64 cmd_id;
            u64 pid;
        } *info = ((decltype(info))ipcPrepareHeader(&c, sizeof(*info)));
        info->magic = SFCI_MAGIC;
        info->cmd_id = 65000;
        info->pid = pid;
        rc = ipcDispatch(target_service->mitm_query_h);
        if (R_SUCCEEDED(rc)) {
            IpcParsedCommand r;
            ipcParse(&r);
            struct {
                u64 magic;
                u64 result;
                bool should_mitm;
            } *resp = ((decltype(resp))r.Raw);
            rc = resp->result;
            if (R_SUCCEEDED(rc)) {
                if (resp->should_mitm) {
                    rc = svcConnectToPort(&target_service->mitm_fwd_sess_h, target_service->port_h);
                    if (R_SUCCEEDED(rc)) {   
                        rc = svcConnectToPort(out, target_service->mitm_port_h);
                        if (R_SUCCEEDED(rc)) {
                            target_service->mitm_waiting_ack_pid = pid;
                            target_service->mitm_waiting_ack = true;
                        } else {
                            svcCloseHandle(target_service->mitm_fwd_sess_h);
                            target_service->mitm_fwd_sess_h = 0;
                        }
                    }
                } else {
                    rc = svcConnectToPort(out, target_service->port_h);
                }
            }
        }
        if (R_FAILED(rc)) {
            rc = svcConnectToPort(out, target_service->port_h);
        }
    }
    if (R_FAILED(rc)) {
        if ((rc & 0x3FFFFF) == ResultKernelOutOfSessions) {
            return ResultSmInsufficientSessions;
        }
    }

    return rc;
}

Result Registration::GetServiceForPid(u64 pid, u64 service, Handle *out) {
    if (!service) {
        return ResultSmInvalidServiceName;
    }
    
    u64 service_name_len = GetServiceNameLength(service);
    
    /* If the service has bytes after a null terminator, that's no good. */
    if (service_name_len != 8 && (service >> (8 * service_name_len))) {
        return ResultSmInvalidServiceName;
    }

    /* In 8.0.0, Nintendo removed the service apm:p -- however, all homebrew attempts to get */
    /* a handle to this when calling appletInitialize(). Because hbl has access to all services, */
    /* This would return true, and homebrew would *wait forever* trying to get a handle to a service */
    /* that will never register. Thus, in the interest of not breaking every single piece of homebrew */
    /* we will provide a little first class help. */
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_800 && service == EncodeNameConstant("apm:p")) {
        return ResultSmNotAllowed;
    }
    
    if (!IsInitialProcess(pid)) {
        Registration::Process *proc = GetProcessForPid(pid);
        if (proc == NULL) {
            return ResultSmInvalidClient;
        }
        
        if (!IsValidForSac(proc->sac, proc->sac_size, service, false)) {
            return ResultSmNotAllowed;
        }
    }
    
    return GetServiceHandle(pid, service, out);
}

Result Registration::RegisterServiceForPid(u64 pid, u64 service, u64 max_sessions, bool is_light, Handle *out) {
    if (!service) {
        return ResultSmInvalidServiceName;
    }
    
    u64 service_name_len = GetServiceNameLength(service);
    
    /* If the service has bytes after a null terminator, that's no good. */
    if (service_name_len != 8 && (service >> (8 * service_name_len))) {
        return ResultSmInvalidServiceName;
    }
    
    if (!IsInitialProcess(pid)) {
        Registration::Process *proc = GetProcessForPid(pid);
        if (proc == NULL) {
            return ResultSmInvalidClient;
        }
        
        if (!IsValidForSac(proc->sac, proc->sac_size, service, true)) {
            return ResultSmNotAllowed;
        }
    }
    
    if (HasService(service)) {
        return ResultSmAlreadyRegistered;
    }
    
#ifdef SM_MINIMUM_SESSION_LIMIT
    if (max_sessions < SM_MINIMUM_SESSION_LIMIT) {
        max_sessions = SM_MINIMUM_SESSION_LIMIT;
    }
#endif

    Registration::Service *free_service = GetFreeService();
    if (free_service == NULL) {
        return ResultSmInsufficientServices;
    }
    
    *out = 0;
    *free_service = (const Registration::Service){0};
    Result rc = svcCreatePort(out, &free_service->port_h, max_sessions, is_light, (char *)&free_service->service_name);
    
    if (R_SUCCEEDED(rc)) {
        free_service->service_name = service;
        free_service->owner_pid = pid;
        free_service->max_sessions = max_sessions;
        free_service->is_light = is_light;
    }
    
    return rc;
}

Result Registration::RegisterServiceForSelf(u64 service, u64 max_sessions, bool is_light, Handle *out) {
    u64 pid;
    Result rc = svcGetProcessId(&pid, CUR_PROCESS_HANDLE);
    if (R_FAILED(rc)) {
        return rc;
    }
    
    u64 service_name_len = GetServiceNameLength(service);
    
    /* If the service has bytes after a null terminator, that's no good. */
    if (service_name_len != 8 && (service >> (8 * service_name_len))) {
        return ResultSmInvalidServiceName;
    }
        
    if (HasService(service)) {
        return ResultSmAlreadyRegistered;
    }

#ifdef SM_MINIMUM_SESSION_LIMIT
    if (max_sessions < SM_MINIMUM_SESSION_LIMIT) {
        max_sessions = SM_MINIMUM_SESSION_LIMIT;
    }
#endif
    
    Registration::Service *free_service = GetFreeService();
    if (free_service == NULL) {
        return ResultSmInsufficientServices;
    }
    
    *out = 0;
    *free_service = (const Registration::Service){0};
    rc = svcCreatePort(out, &free_service->port_h, max_sessions, is_light, (char *)&free_service->service_name);
    
    if (R_SUCCEEDED(rc)) {
        free_service->service_name = service;
        free_service->owner_pid = pid;
        free_service->max_sessions = max_sessions;
        free_service->is_light = is_light;
    }
    
    return rc;
}

Result Registration::UnregisterServiceForPid(u64 pid, u64 service) {
    if (!service) {
        return ResultSmInvalidServiceName;
    }
    
    u64 service_name_len = GetServiceNameLength(service);
    
    /* If the service has bytes after a null terminator, that's no good. */
    if (service_name_len != 8 && (service >> (8 * service_name_len))) {
        return ResultSmInvalidServiceName;
    }
    
    Registration::Service *target_service = GetService(service);
    if (target_service == NULL) {
        return ResultSmNotRegistered;
    }

    if (!IsInitialProcess(pid) && target_service->owner_pid != pid) {
        return ResultSmNotAllowed;
    }
    
    svcCloseHandle(target_service->port_h);
    svcCloseHandle(target_service->mitm_port_h);
    svcCloseHandle(target_service->mitm_query_h);
    *target_service = (const Registration::Service){0};
    return ResultSuccess;
}


Result Registration::InstallMitmForPid(u64 pid, u64 service, Handle *out, Handle *query_out) {
    if (!service) {
        return ResultSmInvalidServiceName;
    }
    
    u64 service_name_len = GetServiceNameLength(service);
    
    /* If the service has bytes after a null terminator, that's no good. */
    if (service_name_len != 8 && (service >> (8 * service_name_len))) {
        return ResultSmInvalidServiceName;
    }
    
    /* Verify we're allowed to mitm the service. */
    if (!IsInitialProcess(pid)) {
        Registration::Process *proc = GetProcessForPid(pid);
        if (proc == NULL) {
            return ResultSmInvalidClient;
        }
        
        if (!IsValidForSac(proc->sac, proc->sac_size, service, true)) {
            return ResultSmNotAllowed;
        }
    }
    
    /* Verify the service exists. */
    Registration::Service *target_service = GetService(service);
    if (target_service == NULL) {
        return ResultServiceFrameworkRequestDeferredByUser;
    }
    
    /* Verify the service isn't already being mitm'd. */
    if (target_service->mitm_pid != 0) {
        return ResultSmAlreadyRegistered;
    }
    
    *out = 0;
    u64 x = 0;
    Result rc = svcCreatePort(out, &target_service->mitm_port_h, target_service->max_sessions, target_service->is_light, (char *)&x);
    
    if (R_SUCCEEDED(rc) && R_SUCCEEDED((rc = svcCreateSession(query_out, &target_service->mitm_query_h, 0, 0)))) {
        target_service->mitm_pid = pid;
    }
    
    return rc;
}

Result Registration::UninstallMitmForPid(u64 pid, u64 service) {
    if (!service) {
        return ResultSmInvalidServiceName;
    }
    
    u64 service_name_len = GetServiceNameLength(service);
    
    /* If the service has bytes after a null terminator, that's no good. */
    if (service_name_len != 8 && (service >> (8 * service_name_len))) {
        return ResultSmInvalidServiceName;
    }
    
    Registration::Service *target_service = GetService(service);
    if (target_service == NULL) {
        return ResultSmNotRegistered;
    }

    if (!IsInitialProcess(pid) && target_service->mitm_pid != pid) {
        return ResultSmNotAllowed;
    }
    
    svcCloseHandle(target_service->mitm_port_h);
    svcCloseHandle(target_service->mitm_query_h);
    target_service->mitm_pid = 0;
    return ResultSuccess;
}

Result Registration::AcknowledgeMitmSessionForPid(u64 pid, u64 service, Handle *out, u64 *out_pid) {
    if (!service) {
        return ResultSmInvalidServiceName;
    }
    
    u64 service_name_len = GetServiceNameLength(service);
    
    /* If the service has bytes after a null terminator, that's no good. */
    if (service_name_len != 8 && (service >> (8 * service_name_len))) {
        return ResultSmInvalidServiceName;
    }
    
    Registration::Service *target_service = GetService(service);
    if (target_service == NULL) {
        return ResultSmNotRegistered;
    }

    if ((!IsInitialProcess(pid) && target_service->mitm_pid != pid) || !target_service->mitm_waiting_ack) {
        return ResultSmNotAllowed;
    }
    
    *out = target_service->mitm_fwd_sess_h;
    *out_pid = target_service->mitm_waiting_ack_pid;
    target_service->mitm_fwd_sess_h = 0;
    target_service->mitm_waiting_ack_pid = 0;
    target_service->mitm_waiting_ack = false;
    return ResultSuccess;
}

Result Registration::AssociatePidTidForMitm(u64 pid, u64 tid) {
    for (auto &service : g_service_list) {
        if (service.mitm_pid) {
            IpcCommand c;
            ipcInitialize(&c);
            struct {
                u64 magic;
                u64 cmd_id;
                u64 pid;
                u64 tid;
            } *info = ((decltype(info))ipcPrepareHeader(&c, sizeof(*info)));
            info->magic = SFCI_MAGIC;
            info->cmd_id = 65001;
            info->pid = pid;
            info->tid = tid;
            ipcDispatch(service.mitm_query_h);
        }
    }
    return ResultSuccess;
}

void Registration::ConvertServiceToRecord(Registration::Service *service, SmServiceRecord *record) {
    record->service_name         = service->service_name;
    record->owner_pid            = service->owner_pid;
    record->max_sessions         = service->max_sessions;
    record->mitm_pid             = service->mitm_pid;
    record->mitm_waiting_ack_pid = service->mitm_waiting_ack_pid;
    record->is_light             = service->is_light;
    record->mitm_waiting_ack     = service->mitm_waiting_ack;
}

Result Registration::GetServiceRecord(u64 service, SmServiceRecord *out) {
    if (!service) {
        return ResultSmInvalidServiceName;
    }
    
    u64 service_name_len = GetServiceNameLength(service);
    
    /* If the service has bytes after a null terminator, that's no good. */
    if (service_name_len != 8 && (service >> (8 * service_name_len))) {
        return ResultSmInvalidServiceName;
    }
    
    Registration::Service *target_service = GetService(service);
    if (target_service == NULL) {
        return ResultSmNotRegistered;
    }
    
    ConvertServiceToRecord(target_service, out);
    return ResultSuccess;
}

void Registration::ListServiceRecords(u64 offset, u64 max_out, SmServiceRecord *out, u64 *out_count) {
    u64 count = 0;
    
    for (auto it = g_service_list.begin(); it != g_service_list.end() && count < max_out; it++) {
        if (it->service_name != 0) {
            if (offset > 0) {
                offset--;
            } else {
                ConvertServiceToRecord(it, out++);
                count++;
            }
        }
    }
    
    *out_count = count;
}
