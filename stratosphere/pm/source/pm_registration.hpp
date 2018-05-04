#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#define LAUNCHFLAGS_NOTIFYWHENEXITED(flags) (flags & 1)
#define LAUNCHFLAGS_STARTSUSPENDED(flags) (flags & (kernelAbove500() ? 0x10 : 0x2))
#define LAUNCHFLAGS_ARGLOW(flags) (kernelAbove500() ? ((flags & 0x14) != 0x10) : (kernelAbove200() ? ((flags & 0x6) != 0x2) : ((flags >> 2) & 1)))
#define LAUNCHFLAGS_ARGHIGH(flags) (flags & (kernelAbove500() ? 0x20 : 0x8))
#define LAUNCHFLAGS_NOTIFYDEBUGEVENTS(flags) (flags & (kernelAbove500() ? 0x8 : 0x10))
#define LAUNCHFLAGS_NOTIYDEBUGSPECIAL(flags) (flags & (kernelAbove500() ? 0x2 : (kernelAbove200() ? 0x20 : 0x0)))


class Registration {
    public:
        struct TidSid {
            u64 title_id;
            FsStorageId storage_id;
        };
        struct Process {
            Handle handle;
            u64 pid;
            u64 ldr_queue_index;
            Registration::TidSid tid_sid;
            ProcessState state;
            u32 flags;
        };
        
        struct ProcessLaunchState {
            TidSid tid_sid;
            u64 launch_flags;
            u64* out_pid;
            Result result;
        };
        
        static void InitializeSystemResources();
        static IWaitable *GetProcessLaunchStartEvent();
        static Result ProcessLaunchStartCallback(Handle *handles, size_t num_handles, u64 timeout);
        
        static IWaitable *GetProcessList();
        static void HandleSignaledProcess(Process *process);
        static void FinalizeExitedProcess(Process *process);
        
        static void AddProcessToList(Process *process);
        static void RemoveProcessFromList(u64 pid);
        static void SetProcessState(u64 pid, ProcessState new_state);
        
        static void HandleProcessLaunch();
        static void SignalFinishLaunchProcess();
        static Result LaunchProcess(u64 title_id, FsStorageId storage_id, u64 launch_flags, u64 *out_pid);
        static Result LaunchProcessByTidSid(TidSid tid_sid, u64 launch_flags, u64 *out_pid);
        
        static bool HasApplicationProcess();
        static void EnsureApplicationResourcesAvailable();
};

