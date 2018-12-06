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

enum DmntCmd {
    DebugMonitor_Cmd_BreakDebugProcess = 0,
    DebugMonitor_Cmd_TerminateDebugProcess = 1,
    DebugMonitor_Cmd_CloseHandle = 2,
    DebugMonitor_Cmd_LoadImage = 3,
    DebugMonitor_Cmd_GetProcessId = 4,
    DebugMonitor_Cmd_GetProcessHandle = 5,
    DebugMonitor_Cmd_WaitSynchronization = 6,
    DebugMonitor_Cmd_GetDebugEvent = 7,
    DebugMonitor_Cmd_GetProcessModuleInfo = 8,
    DebugMonitor_Cmd_GetProcessList = 9,
    DebugMonitor_Cmd_GetThreadList = 10,
    DebugMonitor_Cmd_GetDebugThreadContext = 11,
    DebugMonitor_Cmd_ContinueDebugEvent = 12,
    DebugMonitor_Cmd_ReadDebugProcessMemory = 13,
    DebugMonitor_Cmd_WriteDebugProcessMemory = 14,
    DebugMonitor_Cmd_SetDebugThreadContext = 15,
    DebugMonitor_Cmd_GetDebugThreadParam = 16,
    DebugMonitor_Cmd_InitializeThreadInfo = 17,
    DebugMonitor_Cmd_SetHardwareBreakPoint = 18,
    DebugMonitor_Cmd_QueryDebugProcessMemory = 19,
    DebugMonitor_Cmd_GetProcessMemoryDetails = 20,
    DebugMonitor_Cmd_AttachByProgramId = 21,
    DebugMonitor_Cmd_AttachOnLaunch = 22,
    DebugMonitor_Cmd_GetDebugMonitorProcessId = 23,
    DebugMonitor_Cmd_GetJitDebugProcessList = 25,
    DebugMonitor_Cmd_CreateCoreDump = 26,
    DebugMonitor_Cmd_GetAllDebugThreadInfo = 27,
    DebugMonitor_Cmd_TargetIO_FileOpen = 29,
    DebugMonitor_Cmd_TargetIO_FileClose = 30,
    DebugMonitor_Cmd_TargetIO_FileRead = 31,
    DebugMonitor_Cmd_TargetIO_FileWrite = 32,
    DebugMonitor_Cmd_TargetIO_FileSetAttributes = 33,
    DebugMonitor_Cmd_TargetIO_FileGetInformation = 34,
    DebugMonitor_Cmd_TargetIO_FileSetTime = 35,
    DebugMonitor_Cmd_TargetIO_FileSetSize = 36,
    DebugMonitor_Cmd_TargetIO_FileDelete = 37,
    DebugMonitor_Cmd_TargetIO_FileMove = 38,
    DebugMonitor_Cmd_TargetIO_DirectoryCreate = 39,
    DebugMonitor_Cmd_TargetIO_DirectoryDelete = 40,
    DebugMonitor_Cmd_TargetIO_DirectoryRename = 41,
    DebugMonitor_Cmd_TargetIO_DirectoryGetCount = 42,
    DebugMonitor_Cmd_TargetIO_DirectoryOpen = 43,
    DebugMonitor_Cmd_TargetIO_DirectoryGetNext = 44,
    DebugMonitor_Cmd_TargetIO_DirectoryClose = 45,
    DebugMonitor_Cmd_TargetIO_GetFreeSpace = 46,
    DebugMonitor_Cmd_TargetIO_GetVolumeInformation = 47,
    DebugMonitor_Cmd_InitiateCoreDump = 48,
    DebugMonitor_Cmd_ContinueCoreDump = 49,
    DebugMonitor_Cmd_AddTTYToCoreDump = 50,
    DebugMonitor_Cmd_AddImageToCoreDump = 51,
    DebugMonitor_Cmd_CloseCoreDump = 52,
    DebugMonitor_Cmd_CancelAttach = 53,
};

class DebugMonitorService final : public IServiceObject {
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
            MakeServiceCommandMeta<DebugMonitor_Cmd_BreakDebugProcess, &DebugMonitorService::BreakDebugProcess>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_TerminateDebugProcess, &DebugMonitorService::TerminateDebugProcess>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_CloseHandle, &DebugMonitorService::CloseHandle>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_LoadImage, &DebugMonitorService::LoadImage>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_GetProcessId, &DebugMonitorService::GetProcessId>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_GetProcessHandle, &DebugMonitorService::GetProcessHandle>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_WaitSynchronization, &DebugMonitorService::WaitSynchronization>(),
            //MakeServiceCommandMeta<DebugMonitor_Cmd_GetDebugEvent, &DebugMonitorService::GetDebugEvent>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_GetProcessModuleInfo, &DebugMonitorService::GetProcessModuleInfo>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_GetProcessList, &DebugMonitorService::GetProcessList>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_GetThreadList, &DebugMonitorService::GetThreadList>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_GetDebugThreadContext, &DebugMonitorService::GetDebugThreadContext>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_ContinueDebugEvent, &DebugMonitorService::ContinueDebugEvent>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_ReadDebugProcessMemory, &DebugMonitorService::ReadDebugProcessMemory>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_WriteDebugProcessMemory, &DebugMonitorService::WriteDebugProcessMemory>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_SetDebugThreadContext, &DebugMonitorService::SetDebugThreadContext>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_GetDebugThreadParam, &DebugMonitorService::GetDebugThreadParam>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_InitializeThreadInfo, &DebugMonitorService::InitializeThreadInfo>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_SetHardwareBreakPoint, &DebugMonitorService::SetHardwareBreakPoint>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_QueryDebugProcessMemory, &DebugMonitorService::QueryDebugProcessMemory>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_GetProcessMemoryDetails, &DebugMonitorService::GetProcessMemoryDetails>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_AttachByProgramId, &DebugMonitorService::AttachByProgramId>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_AttachOnLaunch, &DebugMonitorService::AttachOnLaunch>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_GetDebugMonitorProcessId, &DebugMonitorService::GetDebugMonitorProcessId>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_GetJitDebugProcessList, &DebugMonitorService::GetJitDebugProcessList>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_CreateCoreDump, &DebugMonitorService::CreateCoreDump>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_GetAllDebugThreadInfo, &DebugMonitorService::GetAllDebugThreadInfo>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_FileOpen, &DebugMonitorService::TargetIO_FileOpen>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_FileClose, &DebugMonitorService::TargetIO_FileClose>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_FileRead, &DebugMonitorService::TargetIO_FileRead>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_FileWrite, &DebugMonitorService::TargetIO_FileWrite>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_FileSetAttributes, &DebugMonitorService::TargetIO_FileSetAttributes>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_FileGetInformation, &DebugMonitorService::TargetIO_FileGetInformation>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_FileSetTime, &DebugMonitorService::TargetIO_FileSetTime>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_FileSetSize, &DebugMonitorService::TargetIO_FileSetSize>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_FileDelete, &DebugMonitorService::TargetIO_FileDelete>(),
            MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_FileMove, &DebugMonitorService::TargetIO_FileMove>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_DirectoryCreate, &DebugMonitorService::TargetIO_DirectoryCreate>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_DirectoryDelete, &DebugMonitorService::TargetIO_DirectoryDelete>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_DirectoryRename, &DebugMonitorService::TargetIO_DirectoryRename>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_DirectoryGetCount, &DebugMonitorService::TargetIO_DirectoryGetCount>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_DirectoryOpen, &DebugMonitorService::TargetIO_DirectoryOpen>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_DirectoryGetNext, &DebugMonitorService::TargetIO_DirectoryGetNext>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_DirectoryClose, &DebugMonitorService::TargetIO_DirectoryClose>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_GetFreeSpace, &DebugMonitorService::TargetIO_GetFreeSpace>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_TargetIO_GetVolumeInformation, &DebugMonitorService::TargetIO_GetVolumeInformation>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_InitiateCoreDump, &DebugMonitorService::InitiateCoreDump>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_ContinueCoreDump, &DebugMonitorService::ContinueCoreDump>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_AddTTYToCoreDump, &DebugMonitorService::AddTTYToCoreDump>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_AddImageToCoreDump, &DebugMonitorService::AddImageToCoreDump>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_CloseCoreDump, &DebugMonitorService::CloseCoreDump>(),
            // MakeServiceCommandMeta<DebugMonitor_Cmd_CancelAttach, &DebugMonitorService::CancelAttach>(),
        };
};
