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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

class DebugMonitorService final : public IServiceObject {
    private:
        enum class CommandId {
            BreakDebugProcess               = 0,
            TerminateDebugProcess           = 1,
            CloseHandle                     = 2,
            LoadImage                       = 3,
            GetProcessId                    = 4,
            GetProcessHandle                = 5,
            WaitSynchronization             = 6,
            GetDebugEvent                   = 7,
            GetProcessModuleInfo            = 8,
            GetProcessList                  = 9,
            GetThreadList                   = 10,
            GetDebugThreadContext           = 11,
            ContinueDebugEvent              = 12,
            ReadDebugProcessMemory          = 13,
            WriteDebugProcessMemory         = 14,
            SetDebugThreadContext           = 15,
            GetDebugThreadParam             = 16,
            InitializeThreadInfo            = 17,
            SetHardwareBreakPoint           = 18,
            QueryDebugProcessMemory         = 19,
            GetProcessMemoryDetails         = 20,
            AttachByProgramId               = 21,
            AttachOnLaunch                  = 22,
            GetDebugMonitorProcessId        = 23,
            GetJitDebugProcessList          = 25,
            CreateCoreDump                  = 26,
            GetAllDebugThreadInfo           = 27,
            TargetIO_FileOpen               = 29,
            TargetIO_FileClose              = 30,
            TargetIO_FileRead               = 31,
            TargetIO_FileWrite              = 32,
            TargetIO_FileSetAttributes      = 33,
            TargetIO_FileGetInformation     = 34,
            TargetIO_FileSetTime            = 35,
            TargetIO_FileSetSize            = 36,
            TargetIO_FileDelete             = 37,
            TargetIO_FileMove               = 38,
            TargetIO_DirectoryCreate        = 39,
            TargetIO_DirectoryDelete        = 40,
            TargetIO_DirectoryRename        = 41,
            TargetIO_DirectoryGetCount      = 42,
            TargetIO_DirectoryOpen          = 43,
            TargetIO_DirectoryGetNext       = 44,
            TargetIO_DirectoryClose         = 45,
            TargetIO_GetFreeSpace           = 46,
            TargetIO_GetVolumeInformation   = 47,
            InitiateCoreDump                = 48,
            ContinueCoreDump                = 49,
            AddTTYToCoreDump                = 50,
            AddImageToCoreDump              = 51,
            CloseCoreDump                   = 52,
            CancelAttach                    = 53,
        };
    private:
        Result BreakDebugProcess(Handle debug_hnd);
        Result TerminateDebugProcess(Handle debug_hnd);
        Result CloseHandle(Handle debug_hnd);
        Result GetProcessId(Out<u64> out_pid, Handle hnd);
        Result GetProcessHandle(Out<Handle> out_hnd, u64 pid);
        Result WaitSynchronization(Handle hnd, u64 ns);

        Result TargetIO_FileOpen(OutBuffer<u64> out_hnd, InBuffer<char> path, int open_mode, u32 create_mode);
        Result TargetIO_FileClose(InBuffer<u64> hnd);
        Result TargetIO_FileRead(InBuffer<u64> hnd, OutBuffer<u8, BufferType_Type1> out_data, Out<u32> out_read, u64 offset);
        Result TargetIO_FileWrite(InBuffer<u64> hnd, InBuffer<u8, BufferType_Type1> data, Out<u32> out_written, u64 offset);
        Result TargetIO_FileSetAttributes(InBuffer<char> path, InBuffer<u8> attributes);
        Result TargetIO_FileGetInformation(InBuffer<char> path, OutBuffer<u64> out_info, Out<int> is_directory);
        Result TargetIO_FileSetTime(InBuffer<char> path, u64 create, u64 access, u64 modify);
        Result TargetIO_FileSetSize(InBuffer<char> path, u64 size);
        Result TargetIO_FileDelete(InBuffer<char> path);
        Result TargetIO_FileMove(InBuffer<char> path0, InBuffer<char> path1);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, BreakDebugProcess),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, TerminateDebugProcess),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, CloseHandle),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, LoadImage),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetProcessId),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetProcessHandle),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, WaitSynchronization),
            //MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetDebugEvent),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetProcessModuleInfo),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetProcessList),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetThreadList),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetDebugThreadContext),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, ContinueDebugEvent),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, ReadDebugProcessMemory),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, WriteDebugProcessMemory),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, SetDebugThreadContext),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetDebugThreadParam),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, InitializeThreadInfo),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, SetHardwareBreakPoint),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, QueryDebugProcessMemory),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetProcessMemoryDetails),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, AttachByProgramId),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, AttachOnLaunch),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetDebugMonitorProcessId),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetJitDebugProcessList),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, CreateCoreDump),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetAllDebugThreadInfo),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_FileOpen),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_FileClose),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_FileRead),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_FileWrite),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_FileSetAttributes),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_FileGetInformation),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_FileSetTime),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_FileSetSize),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_FileDelete),
            MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_FileMove),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_DirectoryCreate),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_DirectoryDelete),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_DirectoryRename),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_DirectoryGetCount),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_DirectoryOpen),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_DirectoryGetNext),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_DirectoryClose),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_GetFreeSpace),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, TargetIO_GetVolumeInformation),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, InitiateCoreDump),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, ContinueCoreDump),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, AddTTYToCoreDump),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, AddImageToCoreDump),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, CloseCoreDump),
            // MAKE_SERVICE_COMMAND_META(DebugMonitorService, CancelAttach),
        };
};
