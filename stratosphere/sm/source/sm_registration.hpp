#pragma once
#include <switch.h>

#define REGISTRATION_LIST_MAX_PROCESS (0x40)
#define REGISTRATION_LIST_MAX_SERVICE (0x100)
#define REGISTRATION_MAX_SAC_SIZE (0x200)
#define REGISTRATION_PID_BUILTIN_MAX 0x7

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
        };
        
        /* Utilities. */
        static Registration::Process *GetProcessForPid(u64 pid);
        static Registration::Process *GetFreeProcess();
        static Registration::Service *GetService(u64 service);
        static Registration::Service *GetFreeService();
        static bool IsValidForSac(u8 *sac, size_t sac_size, u64 service, bool is_host);
        static bool ValidateSacAgainstRestriction(u8 *r_sac, size_t r_sac_size, u8 *sac, size_t sac_size);
        
        /* Process management. */
        static Result RegisterProcess(u64 pid, u8 *acid_sac, size_t acid_sac_size, u8 *aci0_sac, size_t aci0_sac_size);
        static Result UnregisterProcess(u64 pid);
        
        /* Service management. */
        static bool HasService(u64 service);
        static Result GetServiceHandle(u64 service, Handle *out);
        static Result GetServiceForPid(u64 pid, u64 service, Handle *out);
        static Result RegisterServiceForPid(u64 pid, u64 service, u64 max_sessions, bool is_light, Handle *out);
        static Result RegisterServiceForSelf(u64 service, u64 max_sessions, bool is_light, Handle *out);
        static Result UnregisterServiceForPid(u64 pid, u64 service);
};
