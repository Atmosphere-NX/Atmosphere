/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere.hpp>
#include "sm_service_manager.hpp"
#include "sm_wait_list.hpp"

namespace ams::sm::impl {

    namespace {

        /* Constexpr definitions. */
        static constexpr size_t ProcessCountMax      = 0x40;
        static constexpr size_t ServiceCountMax      = 0x100;
        static constexpr size_t FutureMitmCountMax   = 0x20;
        static constexpr size_t AccessControlSizeMax = 0x200;

        constexpr auto InitiallyDeferredServiceName = ServiceName::Encode("fsp-srv");

        /* Types. */
        struct ProcessInfo {
            os::ProcessId process_id;
            ncm::ProgramId program_id;
            cfg::OverrideStatus override_status;
            size_t access_control_size;
            u8 access_control[AccessControlSizeMax];

            ProcessInfo() {
                this->Free();
            }

            void Free() {
                this->process_id = os::InvalidProcessId;
                this->program_id = ncm::InvalidProgramId;
                this->override_status = {};
                this->access_control_size = 0;
                std::memset(this->access_control, 0, sizeof(this->access_control));
            }
        };

        /* Forward declaration, for use in ServiceInfo. */
        void GetMitmProcessInfo(MitmProcessInfo *out, os::ProcessId process_id);

        struct ServiceInfo {
            ServiceName name;
            os::ProcessId owner_process_id;
            os::ManagedHandle port_h;

            /* Debug. */
            u64 max_sessions;
            bool is_light;

            /* Mitm Extension. */
            os::ProcessId mitm_process_id;
            os::ManagedHandle mitm_port_h;
            os::ManagedHandle mitm_query_h;

            /* Acknowledgement members. */
            bool mitm_waiting_ack;
            os::ProcessId mitm_waiting_ack_process_id;
            os::ManagedHandle mitm_fwd_sess_h;

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
                this->owner_process_id = os::InvalidProcessId;
                this->max_sessions = 0;
                this->is_light = false;
                this->mitm_process_id = os::InvalidProcessId;
                this->mitm_waiting_ack = false;
                this->mitm_waiting_ack_process_id = os::InvalidProcessId;
            }

            void FreeMitm() {
                /* Close mitm handles. */
                this->mitm_port_h.Clear();
                this->mitm_query_h.Clear();

                /* Reset mitm members. */
                this->mitm_process_id = os::InvalidProcessId;
            }

            void AcknowledgeMitmSession(MitmProcessInfo *out_info, Handle *out_hnd) {
                /* Copy to output. */
                GetMitmProcessInfo(out_info, this->mitm_waiting_ack_process_id);
                *out_hnd = this->mitm_fwd_sess_h.Move();
                this->mitm_waiting_ack = false;
                this->mitm_waiting_ack_process_id = os::InvalidProcessId;
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
            private:
                os::ProcessId min;
                os::ProcessId max;
            public:
                InitialProcessIdLimits() {
                    /* Retrieve process limits. */
                    cfg::GetInitialProcessRange(&this->min, &this->max);

                    /* Ensure range is sane. */
                    AMS_ABORT_UNLESS(this->min <= this->max);
                }

                bool IsInitialProcess(os::ProcessId process_id) const {
                    AMS_ABORT_UNLESS(process_id != os::InvalidProcessId);
                    return this->min <= process_id && process_id <= this->max;
                }
        };

        /* Static members. */
        ProcessInfo g_process_list[ProcessCountMax];
        ServiceInfo g_service_list[ServiceCountMax];
        ServiceName g_future_mitm_list[FutureMitmCountMax];
        InitialProcessIdLimits g_initial_process_id_limits;
        bool g_ended_initial_defers;

        /* Helper functions for interacting with processes/services. */
        ProcessInfo *GetProcessInfo(os::ProcessId process_id) {
            for (size_t i = 0; i < ProcessCountMax; i++) {
                if (g_process_list[i].process_id == process_id) {
                    return &g_process_list[i];
                }
            }
            return nullptr;
        }

        ProcessInfo *GetFreeProcessInfo() {
            return GetProcessInfo(os::InvalidProcessId);
        }

        bool HasProcessInfo(os::ProcessId process_id) {
            return GetProcessInfo(process_id) != nullptr;
        }

        bool IsInitialProcess(os::ProcessId process_id) {
            return g_initial_process_id_limits.IsInitialProcess(process_id);
        }

        constexpr inline bool IsValidProcessId(os::ProcessId process_id) {
            return process_id != os::InvalidProcessId;
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

        bool HasMitm(ServiceName service) {
            const ServiceInfo *service_info = GetServiceInfo(service);
            return service_info != nullptr && IsValidProcessId(service_info->mitm_process_id);
        }

        void GetMitmProcessInfo(MitmProcessInfo *out_info, os::ProcessId process_id) {
            /* Anything that can request a mitm session must have a process info. */
            const auto process_info = GetProcessInfo(process_id);
            AMS_ABORT_UNLESS(process_info != nullptr);

            /* Write to output. */
            out_info->process_id = process_id;
            out_info->program_id = process_info->program_id;
            out_info->override_status = process_info->override_status;
        }

        bool IsMitmDisallowed(ncm::ProgramId program_id) {
            /* Mitm used on certain programs can prevent the boot process from completing. */
            /* TODO: Is there a way to do this that's less hardcoded? Needs design thought. */
            return program_id == ncm::SystemProgramId::Loader   ||
                   program_id == ncm::SystemProgramId::Pm       ||
                   program_id == ncm::SystemProgramId::Spl      ||
                   program_id == ncm::SystemProgramId::Boot     ||
                   program_id == ncm::SystemProgramId::Ncm      ||
                   program_id == ncm::AtmosphereProgramId::Mitm ||
                   program_id == ncm::SystemProgramId::Creport;
        }

        Result AddFutureMitmDeclaration(ServiceName service) {
            for (size_t i = 0; i < FutureMitmCountMax; i++) {
                if (g_future_mitm_list[i] == InvalidServiceName) {
                    g_future_mitm_list[i] = service;
                    return ResultSuccess();
                }
            }
            return sm::ResultOutOfServices();
        }

        bool HasFutureMitmDeclaration(ServiceName service) {
            for (size_t i = 0; i < FutureMitmCountMax; i++) {
                if (g_future_mitm_list[i] == service) {
                    return true;
                }
            }
            return false;
        }

        void ClearFutureMitmDeclaration(ServiceName service) {
            for (size_t i = 0; i < FutureMitmCountMax; i++) {
                if (g_future_mitm_list[i] == service) {
                    g_future_mitm_list[i] = InvalidServiceName;
                }
            }

            /* This might undefer some requests. */
            TriggerResume(service);
        }

        void GetServiceInfoRecord(ServiceRecord *out_record, const ServiceInfo *service_info) {
            out_record->service                     = service_info->name;
            out_record->owner_process_id            = service_info->owner_process_id;
            out_record->max_sessions                = service_info->max_sessions;
            out_record->mitm_process_id             = service_info->mitm_process_id;
            out_record->mitm_waiting_ack_process_id = service_info->mitm_waiting_ack_process_id;
            out_record->is_light                    = service_info->is_light;
            out_record->mitm_waiting_ack            = service_info->mitm_waiting_ack;
        }

        Result ValidateAccessControl(AccessControlEntry access_control, ServiceName service, bool is_host, bool is_wildcard) {
            /* Iterate over all entries in the access control, checking to see if we have a match. */
            while (access_control.IsValid()) {
                if (access_control.IsHost() == is_host) {
                    bool is_valid = true;

                    if (access_control.IsWildcard() == is_wildcard) {
                        /* Check for exact match. */
                        is_valid &= access_control.GetServiceName() == service;
                    } else if (access_control.IsWildcard()) {
                        /* Also allow fuzzy match for wildcard. */
                        ServiceName ac_service = access_control.GetServiceName();
                        is_valid &= std::memcmp(&ac_service, &service, access_control.GetServiceNameSize() - 1) == 0;
                    }

                    R_SUCCEED_IF(is_valid);
                }
                access_control = access_control.GetNextEntry();
            }

            return sm::ResultNotAllowed();
        }

        Result ValidateAccessControl(AccessControlEntry restriction, AccessControlEntry access) {
            /* Ensure that every entry in the access control is allowed by the restriction control. */
            while (access.IsValid()) {
                R_TRY(ValidateAccessControl(restriction, access.GetServiceName(), access.IsHost(), access.IsWildcard()));
                access = access.GetNextEntry();
            }

            return ResultSuccess();
        }

        Result ValidateServiceName(ServiceName service) {
            /* Service names must be non-empty. */
            R_UNLESS(service.name[0] != 0, sm::ResultInvalidServiceName());

            /* Get name length. */
            size_t name_len;
            for (name_len = 1; name_len < sizeof(service); name_len++) {
                if (service.name[name_len] == 0) {
                    break;
                }
            }

            /* Names must be all-zero after they end. */
            while (name_len < sizeof(service)) {
                R_UNLESS(service.name[name_len++] == 0, sm::ResultInvalidServiceName());
            }

            return ResultSuccess();
        }

        bool ShouldDeferForInit(ServiceName service) {
            /* Once end has been called, we're done. */
            if (g_ended_initial_defers) {
                return false;
            }

            /* This is a mechanism by which certain services will always be deferred until sm:m receives a special command. */
            /* This can be extended with more services as needed at a later date. */
            return service == InitiallyDeferredServiceName;
        }

        bool ShouldCloseOnClientDisconnect(ServiceName service) {
            /* jit sysmodule is closed and relaunched by am for each application which uses it. */
            constexpr auto JitU = ServiceName::Encode("jit:u");
            return service == JitU;
        }

        Result GetMitmServiceHandleImpl(Handle *out, ServiceInfo *service_info, const MitmProcessInfo &client_info) {
            /* Send command to query if we should mitm. */
            bool should_mitm;
            {
                Service srv { .session = service_info->mitm_query_h.Get() };
                R_TRY(serviceDispatchInOut(&srv, 65000, client_info, should_mitm));
            }

            /* If we shouldn't mitm, give normal session. */
            R_UNLESS(should_mitm, svcConnectToPort(out, service_info->port_h.Get()));

            /* Create both handles. */
            {
                os::ManagedHandle fwd_hnd, hnd;
                R_TRY(svcConnectToPort(fwd_hnd.GetPointer(), service_info->port_h.Get()));
                R_TRY(svcConnectToPort(hnd.GetPointer(), service_info->mitm_port_h.Get()));
                service_info->mitm_fwd_sess_h = std::move(fwd_hnd);
                *out = hnd.Move();
            }

            service_info->mitm_waiting_ack_process_id = client_info.process_id;
            service_info->mitm_waiting_ack = true;

            return ResultSuccess();
        }

        Result GetServiceHandleImpl(Handle *out, ServiceInfo *service_info, os::ProcessId process_id) {
            /* Clear handle output. */
            *out = INVALID_HANDLE;

            /* Check if we should return a mitm handle. */
            if (IsValidProcessId(service_info->mitm_process_id) && service_info->mitm_process_id != process_id) {
                /* Get mitm process info, ensure that we're allowed to mitm the given program. */
                MitmProcessInfo client_info;
                GetMitmProcessInfo(&client_info, process_id);
                if (!IsMitmDisallowed(client_info.program_id)) {
                    /* We're mitm'd. Assert, because mitm service host dead is an error state. */
                    R_ABORT_UNLESS(GetMitmServiceHandleImpl(out, service_info, client_info));
                    return ResultSuccess();
                }
            }

            /* We're not returning a mitm handle, so just return a normal port handle. */
            return svcConnectToPort(out, service_info->port_h.Get());
        }

        Result RegisterServiceImpl(Handle *out, os::ProcessId process_id, ServiceName service, size_t max_sessions, bool is_light) {
            /* Validate service name. */
            R_TRY(ValidateServiceName(service));

            /* Don't try to register something already registered. */
            R_UNLESS(!HasServiceInfo(service), sm::ResultAlreadyRegistered());

            /* Get free service. */
            ServiceInfo *free_service = GetFreeServiceInfo();
            R_UNLESS(free_service != nullptr, sm::ResultOutOfServices());

            /* Create the new service. */
            *out = INVALID_HANDLE;
            R_TRY(svcCreatePort(out, free_service->port_h.GetPointerAndClear(), max_sessions, is_light, free_service->name.name));

            /* Save info. */
            free_service->name = service;
            free_service->owner_process_id = process_id;
            free_service->max_sessions = max_sessions;
            free_service->is_light = is_light;

            /* This might undefer some requests. */
            TriggerResume(service);

            return ResultSuccess();
        }

    }

    /* Client disconnection callback. */
    void OnClientDisconnected(os::ProcessId process_id) {
        /* Ensure that the process id is valid. */
        if (process_id == os::InvalidProcessId) {
            return;
        }

        /* NOTE: Nintendo unregisters all services a process hosted on client close. */
        /* We do not do this as an atmosphere extension, in order to reduce the number */
        /* of sessions open at any given time. */
        /* However, certain system behavior (jit) relies on this occurring. */
        /* As such, we will special case the system components which rely on the behavior. */
        for (size_t i = 0; i < ServiceCountMax; i++) {
            if (g_service_list[i].name != InvalidServiceName && g_service_list[i].owner_process_id == process_id) {
                if (ShouldCloseOnClientDisconnect(g_service_list[i].name)) {
                    g_service_list[i].Free();
                }
            }
        }
    }

    /* Process management. */
    Result RegisterProcess(os::ProcessId process_id, ncm::ProgramId program_id, cfg::OverrideStatus override_status, const void *acid_sac, size_t acid_sac_size, const void *aci_sac, size_t aci_sac_size) {
        /* Check that access control will fit in the ServiceInfo. */
        R_UNLESS(aci_sac_size <= AccessControlSizeMax, sm::ResultTooLargeAccessControl());

        /* Get free process. */
        ProcessInfo *proc = GetFreeProcessInfo();
        R_UNLESS(proc != nullptr, sm::ResultOutOfProcesses());

        /* Validate restrictions. */
        R_UNLESS(aci_sac_size != 0, sm::ResultNotAllowed());
        R_TRY(ValidateAccessControl(AccessControlEntry(acid_sac, acid_sac_size), AccessControlEntry(aci_sac, aci_sac_size)));

        /* Save info. */
        proc->process_id = process_id;
        proc->program_id = program_id;
        proc->override_status = override_status;
        proc->access_control_size = aci_sac_size;
        std::memcpy(proc->access_control, aci_sac, proc->access_control_size);
        return ResultSuccess();
    }

    Result UnregisterProcess(os::ProcessId process_id) {
        /* Find the process. */
        ProcessInfo *proc = GetProcessInfo(process_id);
        R_UNLESS(proc != nullptr, sm::ResultInvalidClient());

        proc->Free();
        return ResultSuccess();
    }

    /* Service management. */
    Result HasService(bool *out, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        *out = HasServiceInfo(service);
        return ResultSuccess();
    }

    Result WaitService(ServiceName service) {
        bool has_service = false;
        R_TRY(impl::HasService(&has_service, service));

        /* Wait until we have the service. */
        R_SUCCEED_IF(has_service);

        return StartRegisterRetry(service);
    }

    Result GetServiceHandle(Handle *out, os::ProcessId process_id, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* In 8.0.0, Nintendo removed the service apm:p -- however, all homebrew attempts to get */
        /* a handle to this when calling appletInitialize(). Because hbl has access to all services, */
        /* This would return true, and homebrew would *wait forever* trying to get a handle to a service */
        /* that will never register. Thus, in the interest of not breaking every single piece of homebrew */
        /* we will provide a little first class help. */
        constexpr ServiceName ApmP = ServiceName::Encode("apm:p");
        R_UNLESS((hos::GetVersion() < hos::Version_8_0_0) || (service != ApmP), sm::ResultNotAllowed());

        /* Check that the process is registered and allowed to get the service. */
        if (!IsInitialProcess(process_id)) {
            ProcessInfo *proc = GetProcessInfo(process_id);
            R_UNLESS(proc != nullptr, sm::ResultInvalidClient());
            R_TRY(ValidateAccessControl(AccessControlEntry(proc->access_control, proc->access_control_size), service, false, false));
        }

        /* Get service info. Check to see if we need to defer this until later. */
        ServiceInfo *service_info = GetServiceInfo(service);
        if (service_info == nullptr || ShouldDeferForInit(service) || HasFutureMitmDeclaration(service) || service_info->mitm_waiting_ack) {
            return StartRegisterRetry(service);
        }

        /* Get a handle from the service info. */
        R_TRY_CATCH(GetServiceHandleImpl(out, service_info, process_id)) {
            R_CONVERT(svc::ResultOutOfSessions, sm::ResultOutOfSessions())
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result RegisterService(Handle *out, os::ProcessId process_id, ServiceName service, size_t max_sessions, bool is_light) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered and allowed to register the service. */
        if (!IsInitialProcess(process_id)) {
            ProcessInfo *proc = GetProcessInfo(process_id);
            R_UNLESS(proc != nullptr, sm::ResultInvalidClient());

            R_TRY(ValidateAccessControl(AccessControlEntry(proc->access_control, proc->access_control_size), service, true, false));
        }

        R_UNLESS(!HasServiceInfo(service), sm::ResultAlreadyRegistered());
        return RegisterServiceImpl(out, process_id, service, max_sessions, is_light);
    }

    Result RegisterServiceForSelf(Handle *out, ServiceName service, size_t max_sessions) {
        return RegisterServiceImpl(out, os::GetCurrentProcessId(), service, max_sessions, false);
    }

    Result UnregisterService(os::ProcessId process_id, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered. */
        if (!IsInitialProcess(process_id)) {
            R_UNLESS(HasProcessInfo(process_id), sm::ResultInvalidClient());
        }

        /* Ensure that the service is actually registered. */
        ServiceInfo *service_info = GetServiceInfo(service);
        R_UNLESS(service_info != nullptr, sm::ResultNotRegistered());

        /* Check if we have permission to do this. */
        R_UNLESS(service_info->owner_process_id == process_id, sm::ResultNotAllowed());

        /* Unregister the service. */
        service_info->Free();
        return ResultSuccess();
    }

    /* Mitm extensions. */
    Result HasMitm(bool *out, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        const ServiceInfo *service_info = GetServiceInfo(service);
        *out = service_info != nullptr && IsValidProcessId(service_info->mitm_process_id);
        return ResultSuccess();
    }

    Result WaitMitm(ServiceName service) {
        bool has_mitm = false;
        R_TRY(impl::HasMitm(&has_mitm, service));

        /* Wait until we have the mitm. */
        R_SUCCEED_IF(has_mitm);

        return StartRegisterRetry(service);
    }

    Result InstallMitm(Handle *out, Handle *out_query, os::ProcessId process_id, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered and allowed to register the service. */
        if (!IsInitialProcess(process_id)) {
            ProcessInfo *proc = GetProcessInfo(process_id);
            R_UNLESS(proc != nullptr, sm::ResultInvalidClient());
            R_TRY(ValidateAccessControl(AccessControlEntry(proc->access_control, proc->access_control_size), service, true, false));
        }

        /* Validate that the service exists. */
        ServiceInfo *service_info = GetServiceInfo(service);

        /* If it doesn't exist, defer until it does. */
        if (service_info == nullptr) {
            return StartRegisterRetry(service);
        }

        /* Validate that the service isn't already being mitm'd. */
        R_UNLESS(!IsValidProcessId(service_info->mitm_process_id), sm::ResultAlreadyRegistered());

        /* Always clear output. */
        *out = INVALID_HANDLE;
        *out_query = INVALID_HANDLE;

        /* If we don't have a future mitm declaration, add one. */
        /* Client will clear this when ready to process. */
        bool has_existing_future_declaration = HasFutureMitmDeclaration(service);
        if (!has_existing_future_declaration) {
            R_TRY(AddFutureMitmDeclaration(service));
        }

        auto future_guard = SCOPE_GUARD { if (!has_existing_future_declaration) { ClearFutureMitmDeclaration(service); } };

        /* Create mitm handles. */
        {
            os::ManagedHandle hnd, port_hnd, qry_hnd, mitm_qry_hnd;
            R_TRY(svcCreatePort(hnd.GetPointer(), port_hnd.GetPointer(), service_info->max_sessions, service_info->is_light, service_info->name.name));
            R_TRY(svcCreateSession(qry_hnd.GetPointer(), mitm_qry_hnd.GetPointer(), 0, 0));

            /* Copy to output. */
            service_info->mitm_process_id = process_id;
            service_info->mitm_port_h = std::move(port_hnd);
            service_info->mitm_query_h = std::move(mitm_qry_hnd);
            *out = hnd.Move();
            *out_query = qry_hnd.Move();

            /* This might undefer some requests. */
            TriggerResume(service);
        }

        future_guard.Cancel();
        return ResultSuccess();
    }

    Result UninstallMitm(os::ProcessId process_id, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered. */
        if (!IsInitialProcess(process_id)) {
            ProcessInfo *proc = GetProcessInfo(process_id);
            R_UNLESS(proc != nullptr, sm::ResultInvalidClient());
        }

        /* Validate that the service exists. */
        ServiceInfo *service_info = GetServiceInfo(service);
        R_UNLESS(service_info != nullptr, sm::ResultNotRegistered());

        /* Validate that the client process_id is the mitm process. */
        R_UNLESS(service_info->mitm_process_id == process_id, sm::ResultNotAllowed());

        /* Free Mitm session info. */
        service_info->FreeMitm();
        return ResultSuccess();
    }

    Result DeclareFutureMitm(os::ProcessId process_id, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered and allowed to register the service. */
        if (!IsInitialProcess(process_id)) {
            ProcessInfo *proc = GetProcessInfo(process_id);
            R_UNLESS(proc != nullptr, sm::ResultInvalidClient());
            R_TRY(ValidateAccessControl(AccessControlEntry(proc->access_control, proc->access_control_size), service, true, false));
        }

        /* Check that mitm hasn't already been registered or declared. */
        R_UNLESS(!HasMitm(service),                  sm::ResultAlreadyRegistered());
        R_UNLESS(!HasFutureMitmDeclaration(service), sm::ResultAlreadyRegistered());

        /* Try to forward declare it. */
        R_TRY(AddFutureMitmDeclaration(service));
        return ResultSuccess();
    }

    Result ClearFutureMitm(os::ProcessId process_id, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered and allowed to register the service. */
        if (!IsInitialProcess(process_id)) {
            ProcessInfo *proc = GetProcessInfo(process_id);
            R_UNLESS(proc != nullptr, sm::ResultInvalidClient());
            R_TRY(ValidateAccessControl(AccessControlEntry(proc->access_control, proc->access_control_size), service, true, false));
        }

        /* Check that a future mitm declaration is present or we have a mitm. */
        if (HasMitm(service)) {
            /* Validate that the service exists. */
            ServiceInfo *service_info = GetServiceInfo(service);
            R_UNLESS(service_info != nullptr, sm::ResultNotRegistered());

            /* Validate that the client process_id is the mitm process. */
            R_UNLESS(service_info->mitm_process_id == process_id, sm::ResultNotAllowed());
        } else {
            R_UNLESS(HasFutureMitmDeclaration(service), sm::ResultNotRegistered());
        }

        /* Clear the forward declaration. */
        ClearFutureMitmDeclaration(service);
        return ResultSuccess();
    }

    Result AcknowledgeMitmSession(MitmProcessInfo *out_info, Handle *out_hnd, os::ProcessId process_id, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered. */
        if (!IsInitialProcess(process_id)) {
            ProcessInfo *proc = GetProcessInfo(process_id);
            R_UNLESS(proc != nullptr, sm::ResultInvalidClient());
        }

        /* Validate that the service exists. */
        ServiceInfo *service_info = GetServiceInfo(service);
        R_UNLESS(service_info != nullptr, sm::ResultNotRegistered());

        /* Validate that the client process_id is the mitm process, and that an acknowledgement is waiting. */
        R_UNLESS(service_info->mitm_process_id == process_id,  sm::ResultNotAllowed());
        R_UNLESS(service_info->mitm_waiting_ack,               sm::ResultNotAllowed());

        /* Acknowledge. */
        service_info->AcknowledgeMitmSession(out_info, out_hnd);

        /* Undefer requests to the session. */
        TriggerResume(service);

        return ResultSuccess();
    }

    /* Dmnt record extensions. */
    Result GetServiceRecord(ServiceRecord *out, ServiceName service) {
        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Validate that the service exists. */
        const ServiceInfo *service_info = GetServiceInfo(service);
        R_UNLESS(service_info != nullptr, sm::ResultNotRegistered());

        GetServiceInfoRecord(out, service_info);
        return ResultSuccess();
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
        return ResultSuccess();
    }

    /* Deferral extension (works around FS bug). */
    Result EndInitialDefers() {
        g_ended_initial_defers = true;

        /* This might undefer some requests. */
        TriggerResume(InitiallyDeferredServiceName);

        return ResultSuccess();
    }

}
