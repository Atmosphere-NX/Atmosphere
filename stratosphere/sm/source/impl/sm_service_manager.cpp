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
#include <stratosphere/sm.hpp>

#include "sm_service_manager.hpp"

namespace sts::sm::impl {

    /* Anonymous namespace for implementation details. */
    namespace {
        /* Constexpr definitions. */
        static constexpr size_t ProcessCountMax = 0x40;
        static constexpr size_t ServiceCountMax = 0x100;
        static constexpr size_t AccessControlSizeMax = 0x200;

        /* Types. */
        struct ProcessInfo {
            u64 pid;
            size_t access_control_size;
            u8 access_control[AccessControlSizeMax];

            ProcessInfo() {
                this->Free();
            }

            void Free() {
                this->pid = InvalidProcessId;
                this->access_control_size = 0;
                std::memset(this->access_control, 0, sizeof(this->access_control));
            }
        };

        struct ServiceInfo {
            ServiceName name;
            u64 owner_pid;
            AutoHandle port_h;

            /* Debug. */
            u64 max_sessions;
            bool is_light;

            /* Mitm Extension. */
            u64 mitm_pid;
            AutoHandle mitm_port_h;
            AutoHandle mitm_query_h;

            /* Acknowledgement members. */
            bool mitm_waiting_ack;
            u64 mitm_waiting_ack_pid;
            AutoHandle mitm_fwd_sess_h;

            ServiceInfo() {
                this->Free();
            }

            void Free() {
                /* Close any open handles. */
                this->port_h.Clear();
                this->mitm_port_h.Clear();
                this->mitm_query_h.Clear();
                this->mitm_fwd_sess_h.Clear();

                /* Reset all other members. */
                this->name = InvalidServiceName;
                this->owner_pid = InvalidProcessId;
                this->max_sessions = 0;
                this->is_light = false;
                this->mitm_pid = InvalidProcessId;
                this->mitm_waiting_ack = false;
                this->mitm_waiting_ack_pid = InvalidProcessId;
            }

            void FreeMitm() {
                /* Close mitm handles. */
                this->mitm_port_h.Clear();
                this->mitm_query_h.Clear();

                /* Reset mitm members. */
                this->mitm_pid = InvalidProcessId;
            }

            void AcknowledgeMitmSession(u64 *out_pid, Handle *out_hnd) {
                /* Copy to output. */
                *out_pid = this->mitm_waiting_ack_pid;
                *out_hnd = this->mitm_fwd_sess_h.Move();
                this->mitm_waiting_ack = false;
                this->mitm_waiting_ack_pid = InvalidProcessId;
            }
        };

        class AccessControlEntry {
            private:
                const u8 *entry;
                size_t capacity;
            public:
                AccessControlEntry(const void *e, size_t c) : entry(reinterpret_cast<const u8 *>(e)), capacity(c) {
                    /* ... */
                }

                AccessControlEntry GetNextEntry() const {
                    return AccessControlEntry(this->entry + this->GetSize(), this->capacity - this->GetSize());
                }

                size_t GetSize() const {
                    return this->GetServiceNameSize() + 1;
                }

                size_t GetServiceNameSize() const {
                    return (this->entry[0] & 7) + 1;
                }

                ServiceName GetServiceName() const {
                    return ServiceName::Encode(reinterpret_cast<const char *>(this->entry + 1), this->GetServiceNameSize());
                }

                bool IsHost() const {
                    return (this->entry[0] & 0x80) != 0;
                }

                bool IsWildcard() const {
                    return this->entry[this->GetServiceNameSize()] == '*';
                }

                bool IsValid() const {
                    /* Validate that we can access data. */
                    if (this->entry == nullptr || this->capacity == 0) {
                        return false;
                    }

                    /* Validate that the size is correct. */
                    return this->GetSize() <= this->capacity;
                }
        };

        class InitialProcessIdLimits {
            public:
                static constexpr u64 InitialProcessIdMin = 0x00;
                static constexpr u64 InitialProcessIdMax = 0x50;
            private:
                u64 min;
                u64 max;
            public:
                InitialProcessIdLimits() {
                    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
                        /* On 5.0.0+, we can get precise limits from svcGetSystemInfo. */
                        R_ASSERT(svcGetSystemInfo(&this->min, SystemInfoType_InitialProcessIdRange, INVALID_HANDLE, InitialProcessIdRangeInfo_Minimum));
                        R_ASSERT(svcGetSystemInfo(&this->max, SystemInfoType_InitialProcessIdRange, INVALID_HANDLE, InitialProcessIdRangeInfo_Maximum));
                    } else if (GetRuntimeFirmwareVersion() >= FirmwareVersion_400) {
                        /* On 4.0.0-4.1.0, we can get the precise limits from normal svcGetInfo. */
                        R_ASSERT(svcGetInfo(&this->min, InfoType_InitialProcessIdRange, INVALID_HANDLE, InitialProcessIdRangeInfo_Minimum));
                        R_ASSERT(svcGetInfo(&this->max, InfoType_InitialProcessIdRange, INVALID_HANDLE, InitialProcessIdRangeInfo_Maximum));
                    } else {
                        /* On < 4.0.0, we just use hardcoded extents. */
                        this->min = InitialProcessIdMin;
                        this->max = InitialProcessIdMax;
                    }

                    /* Ensure range is sane. */
                    if (this->min > this->max) {
                        std::abort();
                    }
                }

                bool IsInitialProcess(u64 pid) const {
                    if (pid == InvalidProcessId) {
                        std::abort();
                    }
                    return this->min <= pid && pid <= this->max;
                }
        };

        /* Static members. */
        ProcessInfo g_process_list[ProcessCountMax];
        ServiceInfo g_service_list[ServiceCountMax];
        InitialProcessIdLimits g_initial_process_id_limits;
        bool g_ended_initial_defers;

        /* Helper functions for interacting with processes/services. */
        ProcessInfo *GetProcessInfo(u64 pid) {
            for (size_t i = 0; i < ProcessCountMax; i++) {
                if (g_process_list[i].pid == pid) {
                    return &g_process_list[i];
                }
            }
            return nullptr;
        }

        ProcessInfo *GetFreeProcessInfo() {
            return GetProcessInfo(InvalidProcessId);
        }

        bool HasProcessInfo(u64 pid) {
            return GetProcessInfo(pid) != nullptr;
        }

        ServiceInfo *GetServiceInfo(ServiceName service_name) {
            for (size_t i = 0; i < ServiceCountMax; i++) {
                if (g_service_list[i].name == service_name) {
                    return &g_service_list[i];
                }
            }
            return nullptr;
        }

        ServiceInfo *GetFreeServiceInfo() {
            return GetServiceInfo(InvalidServiceName);
        }

        bool HasServiceInfo(ServiceName service) {
            return GetServiceInfo(service) != nullptr;
        }

        void GetServiceInfoRecord(ServiceRecord *out_record, const ServiceInfo *service_info) {
            out_record->service              = service_info->name;
            out_record->owner_pid            = service_info->owner_pid;
            out_record->max_sessions         = service_info->max_sessions;
            out_record->mitm_pid             = service_info->mitm_pid;
            out_record->mitm_waiting_ack_pid = service_info->mitm_waiting_ack_pid;
            out_record->is_light             = service_info->is_light;
            out_record->mitm_waiting_ack     = service_info->mitm_waiting_ack;
        }

        Result ValidateAccessControl(AccessControlEntry access_control, ServiceName service, bool is_host, bool is_wildcard) {
            /* Iterate over all entries in the access control, checking to see if we have a match. */
            while (access_control.IsValid()) {
                if (access_control.IsHost() == is_host) {
                    if (access_control.IsWildcard() == is_wildcard) {
                        /* Check for exact match. */
                        if (access_control.GetServiceName() == service) {
                            return ResultSuccess;
                        }
                    } else if (access_control.IsWildcard()) {
                        /* Also allow fuzzy match for wildcard. */
                        ServiceName ac_service = access_control.GetServiceName();
                        if (std::memcmp(&ac_service, &service, access_control.GetServiceNameSize() - 1) == 0) {
                            return ResultSuccess;
                        }
                    }
                }
                access_control = access_control.GetNextEntry();
            }

            return ResultSmNotAllowed;
        }

        Result ValidateAccessControl(AccessControlEntry restriction, AccessControlEntry access) {
            /* Ensure that every entry in the access control is allowed by the restriction control. */
            while (access.IsValid()) {
                R_TRY(ValidateAccessControl(restriction, access.GetServiceName(), access.IsHost(), access.IsWildcard()));
                access = access.GetNextEntry();
            }

            return ResultSuccess;
        }

        Result ValidateServiceName(ServiceName service) {
            /* Service names must be non-empty. */
            if (service.name[0] == 0) {
                return ResultSmInvalidServiceName;
            }

            /* Get name length. */
            size_t name_len;
            for (name_len = 1; name_len < sizeof(service); name_len++) {
                if (service.name[name_len] == 0) {
                    break;
                }
            }

            /* Names must be all-zero after they end. */
            while (name_len < sizeof(service)) {
                if (service.name[name_len++] != 0) {
                    return ResultSmInvalidServiceName;
                }
            }

            return ResultSuccess;
        }

        bool IsInitialProcess(u64 pid) {
            return g_initial_process_id_limits.IsInitialProcess(pid);
        }

        bool IsValidProcessId(u64 pid) {
            return pid != InvalidProcessId;
        }

        bool ShouldDeferForInit(ServiceName service) {
            /* Once end has been called, we're done. */
            if (g_ended_initial_defers) {
                return false;
            }

            /* This is a mechanism by which certain services will always be deferred until sm:m receives a special command. */
            /* This can be extended with more services as needed at a later date. */
            return service == ServiceName::Encode("fsp-srv");
        }

        Result GetMitmServiceHandleImpl(Handle *out, ServiceInfo *service_info, u64 pid) {
            /* Send command to query if we should mitm. */
            {
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
                R_TRY(ipcDispatch(service_info->mitm_query_h.Get()));
            }

            /* Parse response to see if we should mitm. */
            bool should_mitm;
            {
                IpcParsedCommand r;
                ipcParse(&r);
                struct {
                    u64 magic;
                    u64 result;
                    bool should_mitm;
                } *resp = ((decltype(resp))r.Raw);

                R_TRY(resp->result);
                should_mitm = resp->should_mitm;
            }

            /* If we shouldn't mitm, give normal session. */
            if (!should_mitm) {
                return svcConnectToPort(out, service_info->port_h.Get());
            }

            /* Create both handles. */
            {
                AutoHandle fwd_hnd, hnd;
                R_TRY(svcConnectToPort(fwd_hnd.GetPointer(), service_info->port_h.Get()));
                R_TRY(svcConnectToPort(hnd.GetPointer(), service_info->mitm_port_h.Get()));
                service_info->mitm_fwd_sess_h = std::move(fwd_hnd);
                *out = hnd.Move();
            }

            service_info->mitm_waiting_ack_pid = pid;
            service_info->mitm_waiting_ack = true;

            return ResultSuccess;
        }

        Result GetServiceHandleImpl(Handle *out, ServiceInfo *service_info, u64 pid) {
            /* Clear handle output. */
            *out = INVALID_HANDLE;

            /* If not mitm'd or mitm service is requesting, get normal session. */
            if (!IsValidProcessId(service_info->mitm_pid) || service_info->mitm_pid == pid) {
                return svcConnectToPort(out, service_info->port_h.Get());
            }

            /* We're mitm'd. Assert, because mitm service host dead is an error state. */
            R_ASSERT(GetMitmServiceHandleImpl(out, service_info, pid));
            return ResultSuccess;
        }

        Result RegisterServiceImpl(Handle *out, u64 pid, ServiceName service, size_t max_sessions, bool is_light) {
            /* Validate service name. */
            R_TRY(ValidateServiceName(service));

            /* Don't try to register something already registered. */
            if (HasServiceInfo(service)) {
                return ResultSmAlreadyRegistered;
            }

            /* Adjust session limit, if compile flags tell us to. */
#ifdef SM_MINIMUM_SESSION_LIMIT
            if (max_sessions < SM_MINIMUM_SESSION_LIMIT) {
                max_sessions = SM_MINIMUM_SESSION_LIMIT;
            }
#endif

            /* Get free service. */
            ServiceInfo *free_service = GetFreeServiceInfo();
            if (free_service == nullptr) {
                return ResultSmInsufficientServices;
            }

            /* Create the new service. */
            *out = INVALID_HANDLE;
            R_TRY(svcCreatePort(out, free_service->port_h.GetPointerAndClear(), max_sessions, is_light, free_service->name.name));

            /* Save info. */
            free_service->name = service;
            free_service->owner_pid = pid;
            free_service->max_sessions = max_sessions;
            free_service->is_light = is_light;

            return ResultSuccess;
        }
    }

    /* Process management. */
    Result RegisterProcess(u64 pid, const void *acid_sac, size_t acid_sac_size, const void *aci0_sac, size_t aci0_sac_size) {
        /* Check that access control will fit in the ServiceInfo. */
        if (aci0_sac_size > AccessControlSizeMax) {
            return ResultSmTooLargeAccessControl;
        }

        /* Get free process. */
        ProcessInfo *proc = GetFreeProcessInfo();
        if (proc == nullptr) {
            return ResultSmInsufficientProcesses;
        }

        /* Validate restrictions. */
        if (!aci0_sac_size) {
            return ResultSmNotAllowed;
        }
        R_TRY(ValidateAccessControl(AccessControlEntry(acid_sac, acid_sac_size), AccessControlEntry(aci0_sac, aci0_sac_size)));

        /* Save info. */
        proc->pid = pid;
        proc->access_control_size = aci0_sac_size;
        std::memcpy(proc->access_control, aci0_sac, proc->access_control_size);
        return ResultSuccess;
    }

    Result UnregisterProcess(u64 pid) {
        /* Find the process. */
        ProcessInfo *proc = GetProcessInfo(pid);
        if (proc == nullptr) {
            return ResultSmInvalidClient;
        }

        proc->Free();
        return ResultSuccess;
    }

    /* Service management. */
    Result HasService(bool *out, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        *out = HasServiceInfo(service);
        return ResultSuccess;
    }

    Result GetServiceHandle(Handle *out, u64 pid, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* In 8.0.0, Nintendo removed the service apm:p -- however, all homebrew attempts to get */
        /* a handle to this when calling appletInitialize(). Because hbl has access to all services, */
        /* This would return true, and homebrew would *wait forever* trying to get a handle to a service */
        /* that will never register. Thus, in the interest of not breaking every single piece of homebrew */
        /* we will provide a little first class help. */
        if (GetRuntimeFirmwareVersion() >= FirmwareVersion_800 && service == ServiceName::Encode("apm:p")) {
            return ResultSmNotAllowed;
        }

        /* Check that the process is registered and allowed to get the service. */
        if (!IsInitialProcess(pid)) {
            ProcessInfo *proc = GetProcessInfo(pid);
            if (proc == nullptr) {
                return ResultSmInvalidClient;
            }

            R_TRY(ValidateAccessControl(AccessControlEntry(proc->access_control, proc->access_control_size), service, false, false));
        }

        /* Get service info. Check to see if we need to defer this until later. */
        ServiceInfo *service_info = GetServiceInfo(service);
        if (service_info == nullptr || ShouldDeferForInit(service) || service_info->mitm_waiting_ack) {
            return ResultServiceFrameworkRequestDeferredByUser;
        }

        /* Get a handle from the service info. */
        R_TRY_CATCH(GetServiceHandleImpl(out, service_info, pid)) {
            /* Convert Kernel result to SM result. */
            R_CATCH(ResultKernelOutOfSessions) {
                return ResultSmInsufficientSessions;
            }
        } R_END_TRY_CATCH;

        return ResultSuccess;
    }

    Result RegisterService(Handle *out, u64 pid, ServiceName service, size_t max_sessions, bool is_light) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered and allowed to register the service. */
        if (!IsInitialProcess(pid)) {
            ProcessInfo *proc = GetProcessInfo(pid);
            if (proc == nullptr) {
                return ResultSmInvalidClient;
            }

            R_TRY(ValidateAccessControl(AccessControlEntry(proc->access_control, proc->access_control_size), service, true, false));
        }

        if (HasServiceInfo(service)) {
            return ResultSmAlreadyRegistered;
        }

        return RegisterServiceImpl(out, pid, service, max_sessions, is_light);
    }

    Result RegisterServiceForSelf(Handle *out, ServiceName service, size_t max_sessions) {
        u64 self_pid;
        R_TRY(svcGetProcessId(&self_pid, CUR_PROCESS_HANDLE));

        return RegisterServiceImpl(out, self_pid, service, max_sessions, false);
    }

    Result UnregisterService(u64 pid, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered. */
        if (!IsInitialProcess(pid)) {
            if (!HasProcessInfo(pid)) {
                return ResultSmInvalidClient;
            }
        }

        /* Ensure that the service is actually registered. */
        ServiceInfo *service_info = GetServiceInfo(service);
        if (service_info == nullptr) {
            return ResultSmNotRegistered;
        }

        /* Check if we have permission to do this. */
        if (service_info->owner_pid != pid) {
            return ResultSmNotAllowed;
        }

        /* Unregister the service. */
        service_info->Free();
        return ResultSuccess;
    }

    /* Mitm extensions. */
    Result HasMitm(bool *out, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        const ServiceInfo *service_info = GetServiceInfo(service);
        *out = service_info != nullptr && IsValidProcessId(service_info->mitm_pid);
        return ResultSuccess;
    }

    Result InstallMitm(Handle *out, Handle *out_query, u64 pid, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered and allowed to register the service. */
        if (!IsInitialProcess(pid)) {
            ProcessInfo *proc = GetProcessInfo(pid);
            if (proc == nullptr) {
                return ResultSmInvalidClient;
            }

            R_TRY(ValidateAccessControl(AccessControlEntry(proc->access_control, proc->access_control_size), service, true, false));
        }

        /* Validate that the service exists. */
        ServiceInfo *service_info = GetServiceInfo(service);
        if (service_info == nullptr) {
            /* If it doesn't exist, defer until it does. */
            return ResultServiceFrameworkRequestDeferredByUser;
        }

        /* Validate that the service isn't already being mitm'd. */
        if (IsValidProcessId(service_info->mitm_pid)) {
            return ResultSmAlreadyRegistered;
        }

        /* Always clear output. */
        *out = INVALID_HANDLE;
        *out_query = INVALID_HANDLE;

        /* Create mitm handles. */
        {
            AutoHandle hnd, port_hnd, qry_hnd, mitm_qry_hnd;
            u64 x = 0;
            R_TRY(svcCreatePort(hnd.GetPointer(), port_hnd.GetPointer(), service_info->max_sessions, service_info->is_light, reinterpret_cast<char *>(&x)));
            R_TRY(svcCreateSession(qry_hnd.GetPointer(), mitm_qry_hnd.GetPointer(), 0, 0));

            /* Copy to output. */
            service_info->mitm_pid = pid;
            service_info->mitm_port_h = std::move(port_hnd);
            service_info->mitm_query_h = std::move(mitm_qry_hnd);
            *out = hnd.Move();
            *out_query = qry_hnd.Move();
        }

        return ResultSuccess;
    }

    Result UninstallMitm(u64 pid, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered. */
        if (!IsInitialProcess(pid)) {
            ProcessInfo *proc = GetProcessInfo(pid);
            if (proc == nullptr) {
                return ResultSmInvalidClient;
            }
        }

        /* Validate that the service exists. */
        ServiceInfo *service_info = GetServiceInfo(service);
        if (service_info == nullptr) {
            return ResultSmNotRegistered;
        }

        /* Validate that the client pid is the mitm process. */
        if (service_info->mitm_pid != pid) {
            return ResultSmNotAllowed;
        }

        /* Free Mitm session info. */
        service_info->FreeMitm();
        return ResultSuccess;
    }

    Result AcknowledgeMitmSession(u64 *out_pid, Handle *out_hnd, u64 pid, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered. */
        if (!IsInitialProcess(pid)) {
            ProcessInfo *proc = GetProcessInfo(pid);
            if (proc == nullptr) {
                return ResultSmInvalidClient;
            }
        }

        /* Validate that the service exists. */
        ServiceInfo *service_info = GetServiceInfo(service);
        if (service_info == nullptr) {
            return ResultSmNotRegistered;
        }

        /* Validate that the client pid is the mitm process, and that an acknowledgement is waiting. */
        if (service_info->mitm_pid != pid || !service_info->mitm_waiting_ack) {
            return ResultSmNotAllowed;
        }

        /* Acknowledge. */
        service_info->AcknowledgeMitmSession(out_pid, out_hnd);
        return ResultSuccess;
    }

    Result AssociatePidTidForMitm(u64 pid, u64 tid) {
        for (size_t i = 0; i < ServiceCountMax; i++) {
            const ServiceInfo *service_info = &g_service_list[i];
            if (IsValidProcessId(service_info->mitm_pid)) {
                /* Send association command to all mitm processes. */
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
                ipcDispatch(service_info->mitm_query_h.Get());
            }
        }
        return ResultSuccess;
    }

    /* Dmnt record extensions. */
    Result GetServiceRecord(ServiceRecord *out, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Validate that the service exists. */
        const ServiceInfo *service_info = GetServiceInfo(service);
        if (service_info == nullptr) {
            return ResultSmNotRegistered;
        }

        GetServiceInfoRecord(out, service_info);
        return ResultSuccess;
    }

    Result ListServiceRecords(ServiceRecord *out, u64 *out_count, u64 offset, u64 max_count) {
        u64 count = 0;

        for (size_t i = 0; i < ServiceCountMax && count < max_count; i++) {
            const ServiceInfo *service_info = &g_service_list[i];
            if (service_info->name != InvalidServiceName) {
                if (offset == 0) {
                    GetServiceInfoRecord(out++, service_info);
                    count++;
                } else {
                    offset--;
                }
            }
        }

        *out_count = 0;
        return ResultSuccess;
    }

    /* Deferral extension (works around FS bug). */
    Result EndInitialDefers() {
        g_ended_initial_defers = true;
        return ResultSuccess;
    }

}
