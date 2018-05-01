#include <switch.h>
#include <algorithm>
#include <stratosphere/servicesession.hpp>
#include "sm_registration.hpp"

static Registration::Process g_process_list[REGISTRATION_LIST_MAX_PROCESS] = {0};
static Registration::Service g_service_list[REGISTRATION_LIST_MAX_SERVICE] = {0};

u64 GetServiceNameLength(u64 service) {
    u64 service_name_len = 0;
    while (service & 0xFF) {
        service_name_len++;
        service >>= 8;
    }
    return service_name_len;
}

/* Utilities. */
Registration::Process *Registration::GetProcessForPid(u64 pid) {
    for (unsigned int i = 0; i < REGISTRATION_LIST_MAX_PROCESS; i++) {
        if (g_process_list[i].pid == pid) {
            return &g_process_list[i];
        }
    }
    return NULL;
}

Registration::Process *Registration::GetFreeProcess() {
    return GetProcessForPid(0);
}

Registration::Service *Registration::GetService(u64 service) {
    for (unsigned int i = 0; i < REGISTRATION_LIST_MAX_SERVICE; i++) {
        if (g_service_list[i].service_name == service) {
            return &g_service_list[i];
        }
    }
    return NULL;
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
        if (cur_service == service_for_compare && (!is_host || cur_is_host)) {
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

/* Process management. */
Result Registration::RegisterProcess(u64 pid, u8 *acid_sac, size_t acid_sac_size, u8 *aci0_sac, size_t aci0_sac_size) {
    Registration::Process *proc = GetFreeProcess();
    if (aci0_sac_size > REGISTRATION_MAX_SAC_SIZE) {
        return 0x1215;
    }
    
    if (proc == NULL) {
        return 0x215;
    }
    
    if (aci0_sac_size && !ValidateSacAgainstRestriction(acid_sac, acid_sac_size, aci0_sac, aci0_sac_size)) {
        return 0x1015;
    }
    
    proc->pid = pid;
    proc->sac_size = aci0_sac_size;
    std::copy(aci0_sac, aci0_sac + aci0_sac_size, proc->sac);
    return 0;
}

Result Registration::UnregisterProcess(u64 pid) {
    Registration::Process *proc = GetProcessForPid(pid);
    if (proc == NULL) {
        return 0x415;
    }
    
    proc->pid = 0;
    return 0;
}

/* Service management. */
bool Registration::HasService(u64 service) {
    for (unsigned int i = 0; i < REGISTRATION_LIST_MAX_SERVICE; i++) {
        if (g_service_list[i].service_name == service) {
            return true;
        }
    }
    return false;
}

Result Registration::GetServiceHandle(u64 service, Handle *out) {
    Registration::Service *target_service = GetService(service);
    if (target_service == NULL) {
        /* Note: This defers the result until later. */
        return RESULT_DEFER_SESSION;
    }
    
    *out = 0;
    Result rc = svcConnectToPort(out, target_service->port_h);
    if (R_FAILED(rc)) {
        if ((rc & 0x3FFFFF) == 0xE01) {
            return 0x615;
        }
    }

    return rc;
}

Result Registration::GetServiceForPid(u64 pid, u64 service, Handle *out) {
    if (!service) {
        return 0xC15;
    }
    
    u64 service_name_len = GetServiceNameLength(service);
    
    /* If the service has bytes after a null terminator, that's no good. */
    if (service_name_len != 8 && (service >> (8 * service_name_len))) {
        return 0xC15;
    }
    
    if (pid >= REGISTRATION_PID_BUILTIN_MAX && service != smEncodeName("dbg:m")) {
        Registration::Process *proc = GetProcessForPid(pid);
        if (proc == NULL) {
            return 0x415;
        }
        
        if (!IsValidForSac(proc->sac, proc->sac_size, service, false)) {
            return 0x1015;
        }
    }
    
    return GetServiceHandle(service, out);
}

Result Registration::RegisterServiceForPid(u64 pid, u64 service, u64 max_sessions, bool is_light, Handle *out) {
    if (!service) {
        return 0xC15;
    }
    
    u64 service_name_len = GetServiceNameLength(service);
    
    /* If the service has bytes after a null terminator, that's no good. */
    if (service_name_len != 8 && (service >> (8 * service_name_len))) {
        return 0xC15;
    }
    
    if (pid >= REGISTRATION_PID_BUILTIN_MAX) {
        Registration::Process *proc = GetProcessForPid(pid);
        if (proc == NULL) {
            return 0x415;
        }
        
        if (!IsValidForSac(proc->sac, proc->sac_size, service, true)) {
            return 0x1015;
        }
    }
    
    if (HasService(service)) {
        return 0x815;
    }
    
    Registration::Service *free_service = GetFreeService();
    if (free_service == NULL) {
        return 0xA15;
    }
    
    *out = 0;
    *free_service = (const Registration::Service){0};
    Result rc = svcCreatePort(out, &free_service->port_h, max_sessions, is_light, (char *)&free_service->service_name);
    
    if (R_SUCCEEDED(rc)) {
        free_service->service_name = service;
        free_service->owner_pid = pid;
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
        return 0xC15;
    }
        
    if (HasService(service)) {
        return 0x815;
    }
    
    Registration::Service *free_service = GetFreeService();
    if (free_service == NULL) {
        return 0xA15;
    }
    
    *out = 0;
    *free_service = (const Registration::Service){0};
    rc = svcCreatePort(out, &free_service->port_h, max_sessions, is_light, (char *)&free_service->service_name);
    
    if (R_SUCCEEDED(rc)) {
        free_service->service_name = service;
        free_service->owner_pid = pid;
    }
    
    return rc;
}

Result Registration::UnregisterServiceForPid(u64 pid, u64 service) {
    if (!service) {
        return 0xC15;
    }
    
    u64 service_name_len = GetServiceNameLength(service);
    
    /* If the service has bytes after a null terminator, that's no good. */
    if (service_name_len != 8 && (service >> (8 * service_name_len))) {
        return 0xC15;
    }
    
    Registration::Service *target_service = GetService(service);
    if (target_service == NULL) {
        return 0xE15;
    }

    if (target_service->owner_pid != pid) {
        return 0x1015;
    }
    
    svcCloseHandle(target_service->port_h);
    *target_service = (const Registration::Service){0};
    return 0;
}
