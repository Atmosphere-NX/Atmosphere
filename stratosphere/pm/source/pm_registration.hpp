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
#include <stratosphere.hpp>
#include <memory>
#include <mutex>

#define LAUNCHFLAGS_NOTIFYWHENEXITED(flags) (flags & 1)
#define LAUNCHFLAGS_STARTSUSPENDED(flags) (flags & (kernelAbove500() ? 0x10 : 0x2))
#define LAUNCHFLAGS_ARGLOW(flags) (kernelAbove500() ? ((flags & 0x14) != 0x10) : (kernelAbove200() ? ((flags & 0x6) != 0x2) : ((flags >> 2) & 1)))
#define LAUNCHFLAGS_ARGHIGH(flags) ((flags & (kernelAbove500() ? 0x20 : 0x8)) ? 2 : 0)
#define LAUNCHFLAGS_NOTIFYDEBUGEVENTS(flags) (flags & (kernelAbove500() ? 0x8 : 0x10))
#define LAUNCHFLAGS_NOTIYDEBUGSPECIAL(flags) (flags & (kernelAbove500() ? 0x2 : (kernelAbove200() ? 0x20 : 0x0)))

// none of these names are official or authoritative in any way
enum {
    /*
        set in HandleProcessLaunch when
            - launch_flags has LAUNCHFLAGS_NOTIFYWHENEXITED set
        signals g_process_event in HandleSignaledProcess when
            - process enters Exited state
            - we're below 5.0.0
            (finalizes dead process when not set)
        adds to dead process list in FinalizeExitedProcess when
            - we're 5.0.0+
            (also signals g_process_event)
        [5.0.0+] causes ProcessEventType 2
     */
    PROCESSFLAGS_NOTIFYWHENEXITED = 0x001,

    /*
        set in HandleSignaledProcess when
            - process crashes
        causes ProcessEventType 1 ([5.0.0+] 3) (persistent)
     */
    PROCESSFLAGS_CRASHED = 0x002,

    /*
        cleared in HandleSignaledProcess when
            - process goes from CRASHED to any other state
        set in HandleSignaledProcess when
            - process crashes
        adds process to GetDebugProcessIds
     */
    PROCESSFLAGS_CRASH_DEBUG = 0x004,

    /*
        set in HandleProcessLaunch when
            - launch_flags has LAUNCHFLAGS_NOTIFYDEBUGEVENTS set
            - and (we're 1.0.0 or program_info.application_type & 4) (is this because 1.0.0 doesn't check?)
        signals g_process_event in HandleSignaledProcess when
            - process enters DebugDetached state
        signals g_process_event in HandleSignaledProcess when
            - process enters Running state
     */
    PROCESSFLAGS_NOTIFYDEBUGEVENTS = 0x008,

    /*
        set in HandleSignaledProcess when
            - process enters DebugDetached state
            - and PROCESSFLAGS_NOTIFYDEBUGEVENTS is set
        set in HandleSignaledProcess when
            - process enters Running state
            - and PROCESSFLAGS_NOTIFYDEBUGEVENTS is set
        set in HandleSignaledProcess when
            - process enters DebugSuspended state
            - and PROCESSFLAGS_NOTIFYDEBUGEVENTS is set
        causes some complicated ProcessEvent (one-shot)
     */
    PROCESSFLAGS_DEBUGEVENTPENDING = 0x010,

    /*
        cleared in HandleSignaledProcess when
            - process enters DebugDetached state
            - and PROCESSFLAGS_NOTIFYDEBUGEVENTS is set
        cleared in HandleSignaledProcess when
            - process enters Running state
            - and PROCESSFLAGS_NOTIFYDEBUGEVENTS is set
        set in HandleSignaledProcess when
            - process enters DebugSuspended state
            - and PROCESSFLAGS_NOTIFYDEBUGEVENTS is set
     */
    PROCESSFLAGS_DEBUGSUSPENDED = 0x020,
    
    /*
        set in HandleProcessLaunch when
            - program_info.application_type & 1
        signals g_debug_application_event in HandleProcessLaunch when
            - g_debug_next_application is set (unsets g_debug_next_application)
        causes HasApplicationProcess to return true
        meaning?
            application
    */
    PROCESSFLAGS_APPLICATION = 0x040,

    /*
        set in HandleProcessLaunch when
            - we're above 2.0.0 (creport related?)
            - and launch_flags has LAUNCHFLAGS_NOTIFYDEBUGSPECIAL set
            - and program_info.application_type & 4
        tested in HandleSignaledProcess when
            - process enters DebugDetached state
            - and we're above 2.0.0
            causes
            - clearing of PROCESSFLAGS_NOTIFYDEBUGSPECIAL (one-shot?)
            - setting of PROCESSFLAGS_DEBUGDETACHED
     */
    PROCESSFLAGS_NOTIFYDEBUGSPECIAL = 0x080,

    /*
        set in HandleSignaledProcess when
            - process enters DebugDetached state
            - and we're above 2.0.0
            - and PROCESSFLAGS_NOTIFYDEBUGSPECIAL was set
        causes ProcessEventType 5 ([5.0.0+] 2) (one-shot)
     */
    PROCESSFLAGS_DEBUGDETACHED = 0x100,
};

enum {
    PROCESSEVENTTYPE_CRASH = 1,
    PROCESSEVENTTYPE_EXIT = 2, // only fired once, when process enters DebugDetached state (likely creport related)
    PROCESSEVENTTYPE_RUNNING = 3, // debug detached or running
    PROCESSEVENTTYPE_SUSPENDED = 4, // debug suspended
    PROCESSEVENTTYPE_DEBUGDETACHED = 5,


    PROCESSEVENTTYPE_500_EXIT = 1,
    PROCESSEVENTTYPE_500_DEBUGDETACHED = 2, // only fired once, when process enters DebugDetached state (likely creport related)
    PROCESSEVENTTYPE_500_CRASH = 3,
    PROCESSEVENTTYPE_500_RUNNING = 4, // debug detached or running
    PROCESSEVENTTYPE_500_SUSPENDED = 5, // debug suspended
};
    
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
        static std::unique_lock<HosRecursiveMutex> GetProcessListUniqueLock();
        static Result ProcessLaunchStartCallback(u64 timeout);
        
        static Result HandleSignaledProcess(std::shared_ptr<Process> process);
        static void FinalizeExitedProcess(std::shared_ptr<Process> process);
        
        static void AddProcessToList(std::shared_ptr<Process> process);
        static void RemoveProcessFromList(u64 pid);
        static void SetProcessState(u64 pid, ProcessState new_state);
        
        static std::shared_ptr<Registration::Process> GetProcess(u64 pid);
        static std::shared_ptr<Registration::Process> GetProcessByTitleId(u64 tid);
        static Result GetDebugProcessIds(u64 *out_pids, u32 max_out, u32 *num_out);
        static Handle GetProcessEventHandle();
        static void GetProcessEventType(u64 *out_pid, u64 *out_type);
        static Result EnableDebugForTitleId(u64 tid, Handle *out);
        static Result EnableDebugForApplication(Handle *out);
        static Result DisableDebug(u32 which);
        static Handle GetDebugTitleEventHandle();
        static Handle GetDebugApplicationEventHandle();
        
        static void HandleProcessLaunch();
        static Result LaunchDebugProcess(u64 pid);
        static void SignalFinishLaunchProcess();
        static Result LaunchProcess(u64 title_id, FsStorageId storage_id, u64 launch_flags, u64 *out_pid);
        static Result LaunchProcessByTidSid(TidSid tid_sid, u64 launch_flags, u64 *out_pid);
        
        static bool HasApplicationProcess(std::shared_ptr<Process> *out);
};

