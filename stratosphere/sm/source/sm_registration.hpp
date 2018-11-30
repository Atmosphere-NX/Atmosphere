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
 
#pragma once
#include <switch.h>

#define REGISTRATION_LIST_MAX_PROCESS (0x40)
#define REGISTRATION_LIST_MAX_SERVICE (0x100)
#define REGISTRATION_MAX_SAC_SIZE (0x200)
#define REGISTRATION_INITIAL_PID_MAX 0x50

class Registration {
    public:
        struct Process {
            u64 pid;
            u64 sac_size;
            u8 sac[REGISTRATION_MAX_SAC_SIZE];
        };
        
        struct Service {
            u64 service_name;
            u64 owner_pid;
            Handle port_h;
            
            /* Debug. */
            u64 max_sessions;
            bool is_light;
            
            /* Extension. */
            u64 mitm_pid;
            Handle mitm_port_h;
            Handle mitm_query_h;
            
            bool mitm_waiting_ack;
            u64 mitm_waiting_ack_pid;
            Handle mitm_fwd_sess_h;
        };
        
        /* Utilities. */
        static void EndInitDefers();
        static bool ShouldInitDefer(u64 service);
        
        static Registration::Process *GetProcessForPid(u64 pid);
        static Registration::Process *GetFreeProcess();
        static Registration::Service *GetService(u64 service);
        static Registration::Service *GetFreeService();
        static bool IsValidForSac(u8 *sac, size_t sac_size, u64 service, bool is_host);
        static bool ValidateSacAgainstRestriction(u8 *r_sac, size_t r_sac_size, u8 *sac, size_t sac_size);
        static void CacheInitialProcessIdLimits();
        static bool IsInitialProcess(u64 pid);
        static u64 GetInitialProcessId();
        
        /* Process management. */
        static Result RegisterProcess(u64 pid, u8 *acid_sac, size_t acid_sac_size, u8 *aci0_sac, size_t aci0_sac_size);
        static Result UnregisterProcess(u64 pid);
        
        /* Service management. */
        static bool HasService(u64 service);
        static bool HasMitm(u64 service);
        static Result GetServiceHandle(u64 pid, u64 service, Handle *out);
        static Result GetServiceForPid(u64 pid, u64 service, Handle *out);
        static Result RegisterServiceForPid(u64 pid, u64 service, u64 max_sessions, bool is_light, Handle *out);
        static Result RegisterServiceForSelf(u64 service, u64 max_sessions, bool is_light, Handle *out);
        static Result UnregisterServiceForPid(u64 pid, u64 service);
        
        /* Extension. */
        static Result InstallMitmForPid(u64 pid, u64 service, Handle *out, Handle *query_out);
        static Result UninstallMitmForPid(u64 pid, u64 service);
        static Result AcknowledgeMitmSessionForPid(u64 pid, u64 service, Handle *out, u64 *out_pid);
        static Result AssociatePidTidForMitm(u64 pid, u64 tid);
};
