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
#include <stratosphere/ldr.hpp>

namespace sts::ldr {

    class LoaderService : public IServiceObject {
        protected:
            /* Official commands. */
            virtual Result CreateProcess(Out<MovedHandle> proc_h, PinId id, u32 flags, CopiedHandle reslimit_h);
            virtual Result GetProgramInfo(OutPointerWithServerSize<ProgramInfo, 0x1> out_program_info, ncm::TitleLocation loc);
            virtual Result PinTitle(Out<PinId> out_id, ncm::TitleLocation loc);
            virtual Result UnpinTitle(PinId id);
            virtual Result SetTitleArguments(ncm::TitleId title_id, InPointer<char> args, u32 args_size);
            virtual Result ClearArguments();
            virtual Result GetProcessModuleInfo(Out<u32> count, OutPointerWithClientSize<ModuleInfo> out, u64 process_id);

            /* Atmosphere commands. */
            virtual Result AtmosphereSetExternalContentSource(Out<MovedHandle> out, ncm::TitleId title_id);
            virtual void   AtmosphereClearExternalContentSource(ncm::TitleId title_id);
            virtual void   AtmosphereHasLaunchedTitle(Out<bool> out, ncm::TitleId title_id);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                /* No commands callable, as LoaderService is abstract. */
            };
    };

    namespace pm {

        class ProcessManagerInterface final : public LoaderService {
            protected:
                enum class CommandId {
                    CreateProcess   = 0,
                    GetProgramInfo  = 1,
                    PinTitle        = 2,
                    UnpinTitle      = 3,

                    AtmosphereHasLaunchedTitle = 65000,
                };
            public:
                DEFINE_SERVICE_DISPATCH_TABLE {
                    MAKE_SERVICE_COMMAND_META(ProcessManagerInterface, CreateProcess),
                    MAKE_SERVICE_COMMAND_META(ProcessManagerInterface, GetProgramInfo),
                    MAKE_SERVICE_COMMAND_META(ProcessManagerInterface, PinTitle),
                    MAKE_SERVICE_COMMAND_META(ProcessManagerInterface, UnpinTitle),

                    MAKE_SERVICE_COMMAND_META(ProcessManagerInterface, AtmosphereHasLaunchedTitle),
                };
        };

    }

    namespace dmnt {

        class DebugMonitorInterface final : public LoaderService {
            protected:
                enum class CommandId {
                    SetTitleArguments    = 0,
                    ClearArguments       = 1,
                    GetProcessModuleInfo = 2,

                    AtmosphereHasLaunchedTitle = 65000,
                };
            public:
                DEFINE_SERVICE_DISPATCH_TABLE {
                    MAKE_SERVICE_COMMAND_META(DebugMonitorInterface, SetTitleArguments),
                    MAKE_SERVICE_COMMAND_META(DebugMonitorInterface, ClearArguments),
                    MAKE_SERVICE_COMMAND_META(DebugMonitorInterface, GetProcessModuleInfo),

                    MAKE_SERVICE_COMMAND_META(DebugMonitorInterface, AtmosphereHasLaunchedTitle),
                };
        };

    }

    namespace shell {

        class ShellInterface final : public LoaderService {
            protected:
                enum class CommandId {
                    SetTitleArguments    = 0,
                    ClearArguments       = 1,

                    AtmosphereSetExternalContentSource   = 65000,
                    AtmosphereClearExternalContentSource = 65001,
                };
            public:
                DEFINE_SERVICE_DISPATCH_TABLE {
                    MAKE_SERVICE_COMMAND_META(ShellInterface, SetTitleArguments),
                    MAKE_SERVICE_COMMAND_META(ShellInterface, ClearArguments),

                    MAKE_SERVICE_COMMAND_META(ShellInterface, AtmosphereSetExternalContentSource),
                    MAKE_SERVICE_COMMAND_META(ShellInterface, AtmosphereClearExternalContentSource),
                };
        };

    }

}
