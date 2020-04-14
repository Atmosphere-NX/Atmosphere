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

namespace ams::ldr {

    class LoaderService : public sf::IServiceObject {
        protected:
            /* Official commands. */
            virtual Result CreateProcess(sf::OutMoveHandle proc_h, PinId id, u32 flags, sf::CopyHandle reslimit_h);
            virtual Result GetProgramInfo(sf::Out<ProgramInfo> out_program_info, const ncm::ProgramLocation &loc);
            virtual Result PinProgram(sf::Out<PinId> out_id, const ncm::ProgramLocation &loc);
            virtual Result UnpinProgram(PinId id);
            virtual Result SetProgramArguments(ncm::ProgramId program_id, const sf::InPointerBuffer &args, u32 args_size);
            virtual Result FlushArguments();
            virtual Result GetProcessModuleInfo(sf::Out<u32> count, const sf::OutPointerArray<ModuleInfo> &out, os::ProcessId process_id);
            virtual Result SetEnabledProgramVerification(bool enabled);

            /* Atmosphere commands. */
            virtual Result AtmosphereRegisterExternalCode(sf::OutMoveHandle out, ncm::ProgramId program_id);
            virtual void   AtmosphereUnregisterExternalCode(ncm::ProgramId program_id);
            virtual void   AtmosphereHasLaunchedProgram(sf::Out<bool> out, ncm::ProgramId program_id);
            virtual Result AtmosphereGetProgramInfo(sf::Out<ProgramInfo> out_program_info, sf::Out<cfg::OverrideStatus> out_status, const ncm::ProgramLocation &loc);
            virtual Result AtmospherePinProgram(sf::Out<PinId> out_id, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &override_status);
    };

    namespace pm {

        class ProcessManagerInterface final : public LoaderService {
            protected:
                enum class CommandId {
                    CreateProcess                 = 0,
                    GetProgramInfo                = 1,
                    PinProgram                    = 2,
                    UnpinProgram                  = 3,
                    SetEnabledProgramVerification = 4,

                    AtmosphereHasLaunchedProgram = 65000,
                    AtmosphereGetProgramInfo     = 65001,
                    AtmospherePinProgram         = 65002,
                };
            public:
                DEFINE_SERVICE_DISPATCH_TABLE {
                    MAKE_SERVICE_COMMAND_META(CreateProcess),
                    MAKE_SERVICE_COMMAND_META(GetProgramInfo),
                    MAKE_SERVICE_COMMAND_META(PinProgram),
                    MAKE_SERVICE_COMMAND_META(UnpinProgram),
                    MAKE_SERVICE_COMMAND_META(SetEnabledProgramVerification, hos::Version_10_0_0),

                    MAKE_SERVICE_COMMAND_META(AtmosphereHasLaunchedProgram),
                    MAKE_SERVICE_COMMAND_META(AtmosphereGetProgramInfo),
                    MAKE_SERVICE_COMMAND_META(AtmospherePinProgram),
                };
        };

    }

    namespace dmnt {

        class DebugMonitorInterface final : public LoaderService {
            protected:
                enum class CommandId {
                    SetProgramArguments  = 0,
                    FlushArguments       = 1,
                    GetProcessModuleInfo = 2,

                    AtmosphereHasLaunchedProgram = 65000,
                };
            public:
                DEFINE_SERVICE_DISPATCH_TABLE {
                    MAKE_SERVICE_COMMAND_META(SetProgramArguments),
                    MAKE_SERVICE_COMMAND_META(FlushArguments),
                    MAKE_SERVICE_COMMAND_META(GetProcessModuleInfo),

                    MAKE_SERVICE_COMMAND_META(AtmosphereHasLaunchedProgram),
                };
        };

    }

    namespace shell {

        class ShellInterface final : public LoaderService {
            protected:
                enum class CommandId {
                    SetProgramArguments  = 0,
                    FlushArguments       = 1,

                    AtmosphereRegisterExternalCode   = 65000,
                    AtmosphereUnregisterExternalCode = 65001,
                };
            public:
                DEFINE_SERVICE_DISPATCH_TABLE {
                    MAKE_SERVICE_COMMAND_META(SetProgramArguments),
                    MAKE_SERVICE_COMMAND_META(FlushArguments),

                    MAKE_SERVICE_COMMAND_META(AtmosphereRegisterExternalCode),
                    MAKE_SERVICE_COMMAND_META(AtmosphereUnregisterExternalCode),
                };
        };

    }

}
