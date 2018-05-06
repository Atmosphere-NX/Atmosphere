#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#define LAUNCHFLAGS_NOTIFYWHENEXITED(flags) (flags & 1)
#define LAUNCHFLAGS_STARTSUSPENDED(flags) (flags & (kernelAbove500() ? 0x10 : 0x2))
#define LAUNCHFLAGS_ARGLOW(flags) (kernelAbove500() ? ((flags & 0x14) != 0x10) : (kernelAbove200() ? ((flags & 0x6) != 0x2) : ((flags >> 2) & 1)))
#define LAUNCHFLAGS_ARGHIGH(flags) ((flags & (kernelAbove500() ? 0x20 : 0x8)) ? 2 : 0)
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
        class AutoProcessListLock {
            private:
                bool has_lock;
            public:
                AutoProcessListLock();
                ~AutoProcessListLock();
                void Unlock();
        };
                
        static void InitializeSystemResources();
        static bool TryWaitProcessLaunchStartEvent();
        
        static IWaitable *GetProcessList();
        static void HandleSignaledProcess(Process *process);
        static void FinalizeExitedProcess(Process *process);
        
        static void AddProcessToList(Process *process);
        static void RemoveProcessFromList(u64 pid);
        static void SetProcessState(u64 pid, ProcessState new_state);
        
        static Process *GetProcess(u64 pid);
        static Process *GetProcessByTitleId(u64 tid);
        static Handle GetProcessEventHandle();
        static void GetProcessEventType(u64 *out_pid, u64 *out_type);
        static Handle GetDebugTitleEventHandle();
        static Handle GetDebugApplicationEventHandle();
        
        static void HandleProcessLaunch();
        static void SignalFinishLaunchProcess();
        static Result LaunchProcess(u64 title_id, FsStorageId storage_id, u64 launch_flags, u64 *out_pid);
        static Result LaunchProcessByTidSid(TidSid tid_sid, u64 launch_flags, u64 *out_pid);
        
        static bool HasApplicationProcess(Process **out);
        static void EnsureApplicationResourcesAvailable();
};

