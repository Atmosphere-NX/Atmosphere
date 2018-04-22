#include <switch.h>
#include <algorithm>
#include "sm_registration.hpp"

static Registration::Process g_process_list[REGISTRATION_LIST_MAX_PROCESS] = {0};
static Registration::Service g_service_list[REGISTRATION_LIST_MAX_SERVICE] = {0};

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
        if (sac[cur_size - 1] == '*') {
            /* Mask cur_service, service with 0xFF.. up until the wildcard. */
            cur_service &= U64_MAX >> (8 * (8 - cur_size - 1));
            service &= U64_MAX >> (8 * (8 - cur_size - 1));
        }
        if (cur_service == service && (!is_host || cur_is_host)) {
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

Result Registration::GetServiceForPid(u64 pid, u64 service, Handle *out) {
    if (!service) {
        return 0xC15;
    }
    
    u64 service_name_len = 0;
    while ((service >> (8 * service_name_len)) & 0xFF) {
        service_name_len++;
    }
    
    /* If the service has bytes after a null terminator, that's no good. */
    if ((service >> (8 * service_name_len))) {
        return 0xC15;
    }
    
    if (pid >= REGISTRATION_PID_BUILTIN_MAX) {
        Registration::Process *proc = GetProcessForPid(pid);
        if (proc == NULL) {
            return 0x415;
        }
        
        if (!IsValidForSac(proc->sac, proc->sac_size, service, false)) {
            return 0x1015;
        }
    }
    
    Registration::Service *target_service = GetService(service);
    if (target_service == NULL) {
        /* TODO: This defers the request, somehow? Needs to be RE'd in more detail. */
        return 0x6580A;
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

Result Registration::RegisterServiceForPid(u64 pid, u64 service, u64 max_sessions, bool is_light, Handle *out) {
    if (!service) {
        return 0xC15;
    }
    
    u64 service_name_len = 0;
    while ((service >> (8 * service_name_len)) & 0xFF) {
        service_name_len++;
    }
    
    /* If the service has bytes after a null terminator, that's no good. */
    if ((service >> (8 * service_name_len))) {
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

Result Registration::UnregisterServiceForPid(u64 pid, u64 service) {
    if (!service) {
        return 0xC15;
    }
    
    u64 service_name_len = 0;
    while ((service >> (8 * service_name_len)) & 0xFF) {
        service_name_len++;
    }
    
    /* If the service has bytes after a null terminator, that's no good. */
    if ((service >> (8 * service_name_len))) {
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
