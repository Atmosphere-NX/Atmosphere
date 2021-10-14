/*
 * Copyright (c) Atmosphère-NX
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
#include "../sm_wait_list.hpp"

namespace ams::hos {

    void InitializeVersionInternal(bool allow_approximate);

}

namespace ams::sm::impl {

    namespace {

        /* Constexpr definitions. */
        static constexpr size_t ProcessCountMax      = 0x50;
        static constexpr size_t ServiceCountMax      = 0x180;
        static constexpr size_t MitmCountMax         = 0x20;
        static constexpr size_t AccessControlSizeMax = 0x200;

        constexpr const sm::ServiceName InitiallyDeferredServices[] = {
            ServiceName::Encode("fsp-srv")
        };

        /* Types. */
        struct ProcessInfo {
            os::ProcessId process_id;
            ncm::ProgramId program_id;
            cfg::OverrideStatus override_status;
            size_t access_control_size;
            u8 access_control[AccessControlSizeMax];
        };

        constexpr const ProcessInfo InvalidProcessInfo = {
            .process_id          = os::InvalidProcessId,
            .program_id          = ncm::InvalidProgramId,
            .override_status     = {},
            .access_control_size = 0,
            .access_control      = {},
        };

        struct ServiceInfo {
            ServiceName name;
            os::ProcessId owner_process_id;
            os::NativeHandle port_h;
            bool is_light;
            u8 max_sessions;
            u8 mitm_index;
        };

        struct MitmInfo {
            os::ProcessId process_id;
            os::ProcessId waiting_ack_process_id;
            os::NativeHandle port_h;
            os::NativeHandle query_h;
            os::NativeHandle fwd_sess_h;
            bool waiting_ack;
        };

        constexpr const ServiceInfo InvalidServiceInfo = {
            .name                        = sm::InvalidServiceName,
            .owner_process_id            = os::InvalidProcessId,
            .port_h                      = os::InvalidNativeHandle,
            .is_light                    = false,
            .max_sessions                = 0,
            .mitm_index                  = MitmCountMax,
        };

        constexpr const MitmInfo InvalidMitmInfo = {
            .process_id             = os::InvalidProcessId,
            .waiting_ack_process_id = os::InvalidProcessId,
            .port_h                 = os::InvalidNativeHandle,
            .query_h                = os::InvalidNativeHandle,
            .fwd_sess_h             = os::InvalidNativeHandle,
            .waiting_ack            = false,
        };

        class AccessControlEntry {
            private:
                const u8 *m_entry;
                size_t m_capacity;
            public:
                AccessControlEntry(const void *e, size_t c) : m_entry(static_cast<const u8 *>(e)), m_capacity(c) {
                    /* ... */
                }

                AccessControlEntry GetNextEntry() const {
                    return AccessControlEntry(m_entry + this->GetSize(), m_capacity - this->GetSize());
                }

                size_t GetSize() const {
                    return this->GetServiceNameSize() + 1;
                }

                size_t GetServiceNameSize() const {
                    return (m_entry[0] & 7) + 1;
                }

                ServiceName GetServiceName() const {
                    return ServiceName::Encode(reinterpret_cast<const char *>(m_entry + 1), this->GetServiceNameSize());
                }

                bool IsHost() const {
                    return (m_entry[0] & 0x80) != 0;
                }

                bool IsWildcard() const {
                    return m_entry[this->GetServiceNameSize()] == '*';
                }

                bool IsValid() const {
                    /* Validate that we can access data. */
                    if (m_entry == nullptr || m_capacity == 0) {
                        return false;
                    }

                    /* Validate that the size is correct. */
                    return this->GetSize() <= m_capacity;
                }
        };

        class InitialProcessIdLimits {
            private:
                os::ProcessId m_min;
                os::ProcessId m_max;
            public:
                InitialProcessIdLimits() {
                    /* Retrieve process limits. */
                    R_ABORT_UNLESS(svc::GetSystemInfo(std::addressof(m_min.value), svc::SystemInfoType_InitialProcessIdRange, svc::InvalidHandle, svc::InitialProcessIdRangeInfo_Minimum));
                    R_ABORT_UNLESS(svc::GetSystemInfo(std::addressof(m_max.value), svc::SystemInfoType_InitialProcessIdRange, svc::InvalidHandle, svc::InitialProcessIdRangeInfo_Maximum));

                    /* Ensure range is sane. */
                    AMS_ABORT_UNLESS(m_min <= m_max);
                }

                bool IsInitialProcess(os::ProcessId process_id) const {
                    AMS_ABORT_UNLESS(process_id != os::InvalidProcessId);
                    return m_min <= process_id && process_id <= m_max;
                }
        };

        /* Static members. */

        /* NOTE: In 12.0.0, Nintendo added multithreaded processing to sm; however, official sm does not do */
        /* any kind of mutual exclusivity when accessing (and modifying) global state. Previously, this was */
        /* not a problem, because sm was strictly single-threaded, and so two threads could not race eachother. */
        /* We will add a mutex (and perform locking) in order to prevent simultaneous access to global state. */
        constinit os::SdkRecursiveMutex g_mutex;

        constinit std::array<ProcessInfo, ProcessCountMax> g_process_list = [] {
            std::array<ProcessInfo, ProcessCountMax> list = {};

            /* Initialize each info. */
            for (auto &process_info : list) {
                process_info = InvalidProcessInfo;
            }

            return list;
        }();

        constinit std::array<ServiceInfo, ServiceCountMax> g_service_list = [] {
            std::array<ServiceInfo, ServiceCountMax> list = {};

            /* Initialize each info. */
            for (auto &service_info : list) {
                service_info = InvalidServiceInfo;
            }

            return list;
        }();

        constinit std::array<ServiceName, MitmCountMax> g_future_mitm_list = [] {
            std::array<ServiceName, MitmCountMax> list = {};

            /* Initialize each info. */
            for (auto &name : list) {
                name = InvalidServiceName;
            }

            return list;
        }();

        constinit std::array<MitmInfo, MitmCountMax> g_mitm_list = [] {
            std::array<MitmInfo, MitmCountMax> list = {};

            /* Initialize each info. */
            for (auto &mitm_info : list) {
                mitm_info = InvalidMitmInfo;
            }

            return list;
        }();

        constinit bool g_ended_initial_defers = false;

        const InitialProcessIdLimits g_initial_process_id_limits;

        /* Helper functionality. */
        bool IsInitialProcess(os::ProcessId process_id) {
            return g_initial_process_id_limits.IsInitialProcess(process_id);
        }

        constexpr inline bool IsValidProcessId(os::ProcessId process_id) {
            return process_id != os::InvalidProcessId;
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
                        is_valid &= std::memcmp(std::addressof(ac_service), std::addressof(service), access_control.GetServiceNameSize() - 1) == 0;
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
            for (const auto &service_name : InitiallyDeferredServices) {
                if (service == service_name) {
                    return true;
                }
            }

            return false;
        }

        ProcessInfo *GetProcessInfo(os::ProcessId process_id) {
            /* Find a process info with a matching id. */
            for (auto &process_info : g_process_list) {
                if (process_info.process_id == process_id) {
                    return std::addressof(process_info);
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

        ServiceInfo *GetServiceInfo(ServiceName service_name) {
            /* Find a service with a matching name. */
            for (auto &service_info : g_service_list) {
                if (service_info.name == service_name) {
                    return std::addressof(service_info);
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

        MitmInfo *GetMitmInfo(const ServiceInfo *service_info) {
            if (service_info->mitm_index < MitmCountMax) {
                return std::addressof(g_mitm_list[service_info->mitm_index]);
            } else {
                return nullptr;
            }
        }

        MitmInfo *GetFreeMitmInfo() {
            /* Find a mitm info without an owner. */
            for (auto &mitm_info : g_mitm_list) {
                if (!IsValidProcessId(mitm_info.process_id)) {
                    return std::addressof(mitm_info);
                }
            }

            return nullptr;
        }

        bool HasMitm(ServiceName service) {
            const ServiceInfo *service_info = GetServiceInfo(service);
            return service_info != nullptr && GetMitmInfo(service_info) != nullptr;
        }

        Result AddFutureMitmDeclaration(ServiceName service) {
            for (auto &future_mitm : g_future_mitm_list) {
                if (future_mitm == InvalidServiceName) {
                    future_mitm = service;
                    return ResultSuccess();
                }
            }

            return sm::ResultOutOfServices();
        }

        bool HasFutureMitmDeclaration(ServiceName service) {
            for (const auto &future_mitm : g_future_mitm_list){
                if (future_mitm == service) {
                    return true;
                }
            }

            return false;
        }

        void ClearFutureMitmDeclaration(ServiceName service) {
            for (auto &future_mitm : g_future_mitm_list) {
                if (future_mitm == service) {
                    future_mitm = InvalidServiceName;
                }
            }

            /* This might undefer some requests. */
            TriggerResume(service);
        }

        void GetMitmProcessInfo(MitmProcessInfo *out_info, os::ProcessId process_id) {
            /* Anything that can request a mitm session must have a process info. */
            const auto process_info = GetProcessInfo(process_id);
            AMS_ABORT_UNLESS(process_info != nullptr);

            /* Write to output. */
            out_info->process_id      = process_id;
            out_info->program_id      = process_info->program_id;
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

        Result CreatePortImpl(os::NativeHandle *out_server, os::NativeHandle *out_client, size_t max_sessions, bool is_light, sm::ServiceName &name) {
            /* Create the port. */
            svc::Handle server_port, client_port;
            R_TRY(svc::CreatePort(std::addressof(server_port), std::addressof(client_port), max_sessions, is_light, reinterpret_cast<uintptr_t>(name.name)));

            /* Set the output handles. */
            *out_server = server_port;
            *out_client = client_port;
            return ResultSuccess();
        }

        Result ConnectToPortImpl(os::NativeHandle *out, os::NativeHandle port) {
            /* Connect to the port. */
            svc::Handle session;
            R_TRY(svc::ConnectToPort(std::addressof(session), port));

            /* Set the output handle. */
            *out = session;
            return ResultSuccess();
        }

        Result GetMitmServiceHandleImpl(os::NativeHandle *out, ServiceInfo *service_info, const MitmProcessInfo &client_info) {
            /* Get the mitm info. */
            MitmInfo *mitm_info = GetMitmInfo(service_info);
            AMS_ABORT_UNLESS(mitm_info != nullptr);

            /* Send command to query if we should mitm. */
            bool should_mitm;
            {
                /* TODO: Convert mitm internal messaging to use tipc? */
                ::Service srv { .session = mitm_info->query_h };
                R_ABORT_UNLESS(::serviceDispatchInOut(std::addressof(srv), 65000, client_info, should_mitm));
            }

            /* If we shouldn't mitm, give normal session. */
            R_UNLESS(should_mitm, ConnectToPortImpl(out, service_info->port_h));

            /* Create both handles. */
            {
                /* Get the forward handle. */
                R_TRY(ConnectToPortImpl(std::addressof(mitm_info->fwd_sess_h), service_info->port_h));

                /* Get the mitm handle. */
                /* This should be guaranteed to succeed, since we got a forward handle. */
                R_ABORT_UNLESS(ConnectToPortImpl(out, mitm_info->port_h));
            }

            mitm_info->waiting_ack_process_id = client_info.process_id;
            mitm_info->waiting_ack            = true;

            return ResultSuccess();
        }

        Result GetServiceHandleImpl(os::NativeHandle *out, ServiceInfo *service_info, os::ProcessId process_id) {
            /* Get the mitm info. */
            MitmInfo *mitm_info = GetMitmInfo(service_info);

            /* Check if we should return a mitm handle. */
            if (mitm_info != nullptr && mitm_info->process_id != process_id) {
                /* Get mitm process info, ensure that we're allowed to mitm the given program. */
                MitmProcessInfo client_info;
                GetMitmProcessInfo(std::addressof(client_info), process_id);
                if (!IsMitmDisallowed(client_info.program_id)) {
                    /* Get a mitm service handle. */
                    return GetMitmServiceHandleImpl(out, service_info, client_info);
                }
            }

            /* We're not returning a mitm handle, so just return a normal port handle. */
            return ConnectToPortImpl(out, service_info->port_h);
        }

        Result RegisterServiceImpl(os::NativeHandle *out, os::ProcessId process_id, ServiceName service, size_t max_sessions, bool is_light) {
            /* Validate service name. */
            R_TRY(ValidateServiceName(service));

            /* Don't try to register something already registered. */
            R_UNLESS(!HasServiceInfo(service), sm::ResultAlreadyRegistered());

            /* Get free service. */
            ServiceInfo *free_service = GetFreeServiceInfo();
            R_UNLESS(free_service != nullptr, sm::ResultOutOfServices());

            /* Create the new service. */
            R_TRY(CreatePortImpl(out, std::addressof(free_service->port_h), max_sessions, is_light, free_service->name));

            /* Save info. */
            free_service->name             = service;
            free_service->owner_process_id = process_id;
            free_service->max_sessions     = max_sessions;
            free_service->is_light         = is_light;

            /* This might undefer some requests. */
            TriggerResume(service);

            return ResultSuccess();
        }

        void UnregisterServiceImpl(ServiceInfo *service_info) {
            /* Get the mitm info. */
            MitmInfo *mitm_info = GetMitmInfo(service_info);

            /* Close all valid handles. */
            os::CloseNativeHandle(service_info->port_h);

            /* Reset the info's state. */
            *service_info = InvalidServiceInfo;

            /* Reset the mitm info, if necessary. */
            if (mitm_info != nullptr) {
                os::CloseNativeHandle(mitm_info->port_h);
                os::CloseNativeHandle(mitm_info->query_h);
                os::CloseNativeHandle(mitm_info->fwd_sess_h);

                *mitm_info = InvalidMitmInfo;
            }
        }

    }

    /* Client disconnection callback. */
    void OnClientDisconnected(os::ProcessId process_id) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

        /* Ensure that the process id is valid. */
        if (process_id == os::InvalidProcessId) {
            return;
        }

        /* Unregister all services a client hosts, on attached-client-close. */
        for (auto &service_info : g_service_list) {
            if (service_info.name != InvalidServiceName && service_info.owner_process_id == process_id) {
                UnregisterServiceImpl(std::addressof(service_info));
            }
        }
    }

    /* Process management. */
    Result RegisterProcess(os::ProcessId process_id, ncm::ProgramId program_id, cfg::OverrideStatus override_status, const void *acid_sac, size_t acid_sac_size, const void *aci_sac, size_t aci_sac_size) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

        /* Check that access control will fit in the ServiceInfo. */
        R_UNLESS(aci_sac_size <= AccessControlSizeMax, sm::ResultTooLargeAccessControl());

        /* Get free process. */
        ProcessInfo *proc = GetFreeProcessInfo();
        R_UNLESS(proc != nullptr, sm::ResultOutOfProcesses());

        /* Validate restrictions. */
        R_UNLESS(aci_sac_size != 0, sm::ResultNotAllowed());
        R_TRY(ValidateAccessControl(AccessControlEntry(acid_sac, acid_sac_size), AccessControlEntry(aci_sac, aci_sac_size)));

        /* Save info. */
        proc->process_id          = process_id;
        proc->program_id          = program_id;
        proc->override_status     = override_status;
        proc->access_control_size = aci_sac_size;
        std::memcpy(proc->access_control, aci_sac, proc->access_control_size);

        return ResultSuccess();
    }

    Result UnregisterProcess(os::ProcessId process_id) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

        /* Find the process. */
        ProcessInfo *proc = GetProcessInfo(process_id);
        R_UNLESS(proc != nullptr, sm::ResultInvalidClient());

        /* Free the process. */
        *proc = InvalidProcessInfo;

        return ResultSuccess();
    }

    /* Service management. */
    Result HasService(bool *out, ServiceName service) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        *out = HasServiceInfo(service);
        return ResultSuccess();
    }

    Result WaitService(ServiceName service) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

        /* Check that we have the service. */
        bool has_service = false;
        R_TRY(impl::HasService(std::addressof(has_service), service));

        /* If we don't, we want to wait until the service is registered. */
        R_UNLESS(has_service, tipc::ResultRequestDeferred());

        return ResultSuccess();
    }

    Result GetServiceHandle(os::NativeHandle *out, os::ProcessId process_id, ServiceName service) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered and allowed to get the service. */
        if (!IsInitialProcess(process_id)) {
            ProcessInfo *proc = GetProcessInfo(process_id);
            R_UNLESS(proc != nullptr, sm::ResultInvalidClient());
            R_TRY(ValidateAccessControl(AccessControlEntry(proc->access_control, proc->access_control_size), service, false, false));
        }

        /* Get service info/mitm info. */
        ServiceInfo *service_info = GetServiceInfo(service);
        MitmInfo *mitm_info = service_info != nullptr ? GetMitmInfo(service_info) : nullptr;

        /* Check to see if we need to defer until later. */
        R_UNLESS(service_info != nullptr,                           tipc::ResultRequestDeferred());
        R_UNLESS(!ShouldDeferForInit(service),                      tipc::ResultRequestDeferred());
        R_UNLESS(!HasFutureMitmDeclaration(service),                tipc::ResultRequestDeferred());
        R_UNLESS((mitm_info == nullptr || !mitm_info->waiting_ack), tipc::ResultRequestDeferred());

        /* Get a handle from the service info. */
        R_TRY_CATCH(GetServiceHandleImpl(out, service_info, process_id)) {
            R_CONVERT(svc::ResultOutOfSessions, sm::ResultOutOfSessions())
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result RegisterService(os::NativeHandle *out, os::ProcessId process_id, ServiceName service, size_t max_sessions, bool is_light) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Check that the process is registered and allowed to register the service. */
        if (!IsInitialProcess(process_id)) {
            ProcessInfo *proc = GetProcessInfo(process_id);
            R_UNLESS(proc != nullptr, sm::ResultInvalidClient());

            R_TRY(ValidateAccessControl(AccessControlEntry(proc->access_control, proc->access_control_size), service, true, false));
        }

        /* Check that the service isn't already registered. */
        R_UNLESS(!HasServiceInfo(service), sm::ResultAlreadyRegistered());

        return RegisterServiceImpl(out, process_id, service, max_sessions, is_light);
    }

    Result RegisterServiceForSelf(os::NativeHandle *out, ServiceName service, size_t max_sessions) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

        return RegisterServiceImpl(out, os::GetCurrentProcessId(), service, max_sessions, false);
    }

    Result UnregisterService(os::ProcessId process_id, ServiceName service) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

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
        UnregisterServiceImpl(service_info);

        return ResultSuccess();
    }

    /* Mitm extensions. */
    Result HasMitm(bool *out, ServiceName service) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

        /* Validate service name. */
        R_TRY(ValidateServiceName(service));

        /* Get whether we have a mitm. */
        *out = HasMitm(service);

        return ResultSuccess();
    }

    Result WaitMitm(ServiceName service) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

        /* Check that we have the mitm. */
        bool has_mitm = false;
        R_TRY(impl::HasMitm(std::addressof(has_mitm), service));

        /* If we don't, we want to wait until the mitm is installed. */
        R_UNLESS(has_mitm, tipc::ResultRequestDeferred());

        return ResultSuccess();
    }

    Result InstallMitm(os::NativeHandle *out, os::NativeHandle *out_query, os::ProcessId process_id, ServiceName service) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

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
        R_UNLESS(service_info != nullptr, tipc::ResultRequestDeferred());

        /* Validate that the service isn't already being mitm'd. */
        R_UNLESS(GetMitmInfo(service_info) == nullptr, sm::ResultAlreadyRegistered());

        /* Validate that we can create a new mitm. */
        MitmInfo *mitm_info = GetFreeMitmInfo();
        R_UNLESS(mitm_info != nullptr, sm::ResultOutOfServices());

        /* If we don't have a future mitm declaration, add one. */
        /* Client will clear this when ready to process. */
        const bool has_existing_future_declaration = HasFutureMitmDeclaration(service);
        if (!has_existing_future_declaration) {
            R_TRY(AddFutureMitmDeclaration(service));
        }

        auto future_guard = SCOPE_GUARD { if (!has_existing_future_declaration) { ClearFutureMitmDeclaration(service); } };

        /* Create mitm handles. */
        {
            /* Get the port handles. */
            os::NativeHandle hnd, port_hnd;
            R_TRY(CreatePortImpl(std::addressof(hnd), std::addressof(port_hnd), service_info->max_sessions, service_info->is_light, service_info->name));

            /* Ensure that we clean up the port handles, if something goes wrong creating the query sessions. */
            auto port_guard = SCOPE_GUARD { os::CloseNativeHandle(hnd); os::CloseNativeHandle(port_hnd); };

            /* Create the session for our query service. */
            os::NativeHandle qry_hnd, mitm_qry_hnd;
            R_TRY(svc::CreateSession(std::addressof(qry_hnd), std::addressof(mitm_qry_hnd), false, 0));

            /* We created the query service session, so we no longer need to clean up the port handles. */
            port_guard.Cancel();

            /* Setup the mitm info. */
            mitm_info->process_id = process_id;
            mitm_info->port_h     = port_hnd;
            mitm_info->query_h    = mitm_qry_hnd;

            /* Setup the service info. */
            service_info->mitm_index = mitm_info - g_mitm_list.data();

            /* Copy to output. */
            *out       = hnd;
            *out_query = qry_hnd;

            /* This might undefer some requests. */
            TriggerResume(service);
        }

        future_guard.Cancel();
        return ResultSuccess();
    }

    Result UninstallMitm(os::ProcessId process_id, ServiceName service) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

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

        /* Validate that the service is mitm'd. */
        MitmInfo *mitm_info = GetMitmInfo(service_info);
        R_UNLESS(mitm_info != nullptr, sm::ResultNotRegistered());

        /* Validate that the client process_id is the mitm process. */
        R_UNLESS(mitm_info->process_id == process_id, sm::ResultNotAllowed());

        /* Uninstall the mitm. */
        {
            /* Close mitm handles. */
            os::CloseNativeHandle(mitm_info->port_h);
            os::CloseNativeHandle(mitm_info->query_h);
            os::CloseNativeHandle(mitm_info->fwd_sess_h);

            /* Reset mitm members. */
            *mitm_info = InvalidMitmInfo;

            /* Reset service info. */
            service_info->mitm_index = MitmCountMax;
        }

        return ResultSuccess();
    }

    Result DeclareFutureMitm(os::ProcessId process_id, ServiceName service) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

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
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

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
        R_UNLESS(service_info != nullptr, sm::ResultNotRegistered());

        /* Check that we have a mitm or a future declaration. */
        if (MitmInfo *mitm_info = GetMitmInfo(service_info); mitm_info != nullptr) {
            /* Validate that the client process_id is the mitm process. */
            R_UNLESS(mitm_info->process_id == process_id, sm::ResultNotAllowed());
        } else {
            R_UNLESS(HasFutureMitmDeclaration(service), sm::ResultNotRegistered());
        }

        /* Clear the forward declaration. */
        ClearFutureMitmDeclaration(service);

        return ResultSuccess();
    }

    Result AcknowledgeMitmSession(MitmProcessInfo *out_info, os::NativeHandle *out_hnd, os::ProcessId process_id, ServiceName service) {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

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

        /* Get the mitm info. */
        MitmInfo *mitm_info = GetMitmInfo(service_info);
        R_UNLESS(mitm_info != nullptr, sm::ResultNotRegistered());

        /* Validate that the client process_id is the mitm process, and that an acknowledgement is waiting. */
        R_UNLESS(mitm_info->process_id == process_id,  sm::ResultNotAllowed());
        R_UNLESS(mitm_info->waiting_ack,               sm::ResultNotAllowed());

        /* Acknowledge. */
        {
            /* Copy the mitm info to output. */
            GetMitmProcessInfo(out_info, mitm_info->waiting_ack_process_id);

            /* Set the output handle. */
            *out_hnd              = mitm_info->fwd_sess_h;
            mitm_info->fwd_sess_h = os::InvalidNativeHandle;

            /* Clear acknowledgement-related fields. */
            mitm_info->waiting_ack            = false;
            mitm_info->waiting_ack_process_id = os::InvalidProcessId;
        }

        /* Undefer requests to the session. */
        TriggerResume(service);

        return ResultSuccess();
    }

    /* Deferral extension (works around FS bug). */
    Result EndInitialDefers() {
        /* Acquire exclusive access to global state. */
        std::scoped_lock lk(g_mutex);

        /* Note that we have ended the initial deferral period. */
        const bool had_ended_defers = g_ended_initial_defers;
        g_ended_initial_defers = true;

        /* Something about deferral state has changed, so we should refresh our hos version. */
        hos::InitializeVersionInternal(!had_ended_defers);

        /* This might undefer some requests. */
        for (const auto &service_name : InitiallyDeferredServices) {
            TriggerResume(service_name);
        }

        return ResultSuccess();
    }

}
