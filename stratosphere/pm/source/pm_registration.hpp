#pragma once
#include <switch.h>
#include <stratosphere.hpp>

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

