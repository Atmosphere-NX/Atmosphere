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
#pragma once
#include <stratosphere.hpp>

namespace ams::dmnt {

    /* TODO: Move into libstratosphere, eventually. */
    struct TargetIOFileHandle : sf::LargeData, sf::PrefersMapAliasTransferMode {
        u64 value;

        constexpr u64 GetValue() const {
            return this->value;
        }

        constexpr explicit operator u64() const {
            return this->value;
        }

        inline constexpr bool operator==(const TargetIOFileHandle &rhs) const {
            return this->value == rhs.value;
        }

        inline constexpr bool operator!=(const TargetIOFileHandle &rhs) const {
            return this->value != rhs.value;
        }

        inline constexpr bool operator<(const TargetIOFileHandle &rhs) const {
            return this->value < rhs.value;
        }

        inline constexpr bool operator<=(const TargetIOFileHandle &rhs) const {
            return this->value <= rhs.value;
        }

        inline constexpr bool operator>(const TargetIOFileHandle &rhs) const {
            return this->value > rhs.value;
        }

        inline constexpr bool operator>=(const TargetIOFileHandle &rhs) const {
            return this->value >= rhs.value;
        }
    };

    static_assert(std::is_pod<TargetIOFileHandle>::value && sizeof(TargetIOFileHandle) == sizeof(u64), "TargetIOFileHandle");

    class DebugMonitorService final : public sf::IServiceObject {
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
            Result GetProcessId(sf::Out<os::ProcessId> out_pid, Handle hnd);
            Result GetProcessHandle(sf::Out<Handle> out_hnd, os::ProcessId pid);
            Result WaitSynchronization(Handle hnd, u64 ns);

            Result TargetIO_FileOpen(sf::Out<TargetIOFileHandle> out_hnd, const sf::InBuffer &path, int open_mode, u32 create_mode);
            Result TargetIO_FileClose(TargetIOFileHandle hnd);
            Result TargetIO_FileRead(TargetIOFileHandle hnd, const sf::OutNonSecureBuffer &out_data, sf::Out<u32> out_read, s64 offset);
            Result TargetIO_FileWrite(TargetIOFileHandle hnd, const sf::InNonSecureBuffer &data, sf::Out<u32> out_written, s64 offset);
            Result TargetIO_FileSetAttributes(const sf::InBuffer &path, const sf::InBuffer &attributes);
            Result TargetIO_FileGetInformation(const sf::InBuffer &path, const sf::OutArray<u64> &out_info, sf::Out<int> is_directory);
            Result TargetIO_FileSetTime(const sf::InBuffer &path, u64 create, u64 access, u64 modify);
            Result TargetIO_FileSetSize(const sf::InBuffer &input, s64 size);
            Result TargetIO_FileDelete(const sf::InBuffer &path);
            Result TargetIO_FileMove(const sf::InBuffer &src_path, const sf::InBuffer &dst_path);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(BreakDebugProcess),
                MAKE_SERVICE_COMMAND_META(TerminateDebugProcess),
                MAKE_SERVICE_COMMAND_META(CloseHandle),
                // MAKE_SERVICE_COMMAND_META(LoadImage),
                MAKE_SERVICE_COMMAND_META(GetProcessId),
                MAKE_SERVICE_COMMAND_META(GetProcessHandle),
                MAKE_SERVICE_COMMAND_META(WaitSynchronization),
                //MAKE_SERVICE_COMMAND_META(GetDebugEvent),
                // MAKE_SERVICE_COMMAND_META(GetProcessModuleInfo),
                // MAKE_SERVICE_COMMAND_META(GetProcessList),
                // MAKE_SERVICE_COMMAND_META(GetThreadList),
                // MAKE_SERVICE_COMMAND_META(GetDebugThreadContext),
                // MAKE_SERVICE_COMMAND_META(ContinueDebugEvent),
                // MAKE_SERVICE_COMMAND_META(ReadDebugProcessMemory),
                // MAKE_SERVICE_COMMAND_META(WriteDebugProcessMemory),
                // MAKE_SERVICE_COMMAND_META(SetDebugThreadContext),
                // MAKE_SERVICE_COMMAND_META(GetDebugThreadParam),
                // MAKE_SERVICE_COMMAND_META(InitializeThreadInfo),
                // MAKE_SERVICE_COMMAND_META(SetHardwareBreakPoint),
                // MAKE_SERVICE_COMMAND_META(QueryDebugProcessMemory),
                // MAKE_SERVICE_COMMAND_META(GetProcessMemoryDetails),
                // MAKE_SERVICE_COMMAND_META(AttachByProgramId),
                // MAKE_SERVICE_COMMAND_META(AttachOnLaunch),
                // MAKE_SERVICE_COMMAND_META(GetDebugMonitorProcessId),
                // MAKE_SERVICE_COMMAND_META(GetJitDebugProcessList),
                // MAKE_SERVICE_COMMAND_META(CreateCoreDump),
                // MAKE_SERVICE_COMMAND_META(GetAllDebugThreadInfo),
                MAKE_SERVICE_COMMAND_META(TargetIO_FileOpen),
                MAKE_SERVICE_COMMAND_META(TargetIO_FileClose),
                MAKE_SERVICE_COMMAND_META(TargetIO_FileRead),
                MAKE_SERVICE_COMMAND_META(TargetIO_FileWrite),
                MAKE_SERVICE_COMMAND_META(TargetIO_FileSetAttributes),
                MAKE_SERVICE_COMMAND_META(TargetIO_FileGetInformation),
                MAKE_SERVICE_COMMAND_META(TargetIO_FileSetTime),
                MAKE_SERVICE_COMMAND_META(TargetIO_FileSetSize),
                MAKE_SERVICE_COMMAND_META(TargetIO_FileDelete),
                MAKE_SERVICE_COMMAND_META(TargetIO_FileMove),
                // MAKE_SERVICE_COMMAND_META(TargetIO_DirectoryCreate),
                // MAKE_SERVICE_COMMAND_META(TargetIO_DirectoryDelete),
                // MAKE_SERVICE_COMMAND_META(TargetIO_DirectoryRename),
                // MAKE_SERVICE_COMMAND_META(TargetIO_DirectoryGetCount),
                // MAKE_SERVICE_COMMAND_META(TargetIO_DirectoryOpen),
                // MAKE_SERVICE_COMMAND_META(TargetIO_DirectoryGetNext),
                // MAKE_SERVICE_COMMAND_META(TargetIO_DirectoryClose),
                // MAKE_SERVICE_COMMAND_META(TargetIO_GetFreeSpace),
                // MAKE_SERVICE_COMMAND_META(TargetIO_GetVolumeInformation),
                // MAKE_SERVICE_COMMAND_META(InitiateCoreDump),
                // MAKE_SERVICE_COMMAND_META(ContinueCoreDump),
                // MAKE_SERVICE_COMMAND_META(AddTTYToCoreDump),
                // MAKE_SERVICE_COMMAND_META(AddImageToCoreDump),
                // MAKE_SERVICE_COMMAND_META(CloseCoreDump),
                // MAKE_SERVICE_COMMAND_META(CancelAttach),
            };
    };

}
