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

namespace ams::ldr {

    class LoaderService : public sf::IServiceObject {
        protected:
            /* Official commands. */
            virtual Result CreateProcess(sf::OutMoveHandle proc_h, PinId id, u32 flags, sf::CopyHandle reslimit_h);
            virtual Result GetProgramInfo(sf::Out<ProgramInfo> out_program_info, const ncm::TitleLocation &loc);
            virtual Result PinTitle(sf::Out<PinId> out_id, const ncm::TitleLocation &loc);
            virtual Result UnpinTitle(PinId id);
            virtual Result SetTitleArguments(ncm::TitleId title_id, const sf::InPointerBuffer &args, u32 args_size);
            virtual Result ClearArguments();
            virtual Result GetProcessModuleInfo(sf::Out<u32> count, const sf::OutPointerArray<ModuleInfo> &out, os::ProcessId process_id);

            /* Atmosphere commands. */
            virtual Result AtmosphereSetExternalContentSource(sf::OutMoveHandle out, ncm::TitleId title_id);
            virtual void   AtmosphereClearExternalContentSource(ncm::TitleId title_id);
            virtual void   AtmosphereHasLaunchedTitle(sf::Out<bool> out, ncm::TitleId title_id);
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
                    MAKE_SERVICE_COMMAND_META(CreateProcess),
                    MAKE_SERVICE_COMMAND_META(GetProgramInfo),
                    MAKE_SERVICE_COMMAND_META(PinTitle),
                    MAKE_SERVICE_COMMAND_META(UnpinTitle),

                    MAKE_SERVICE_COMMAND_META(AtmosphereHasLaunchedTitle),
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
                    MAKE_SERVICE_COMMAND_META(SetTitleArguments),
                    MAKE_SERVICE_COMMAND_META(ClearArguments),
                    MAKE_SERVICE_COMMAND_META(GetProcessModuleInfo),

                    MAKE_SERVICE_COMMAND_META(AtmosphereHasLaunchedTitle),
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
                    MAKE_SERVICE_COMMAND_META(SetTitleArguments),
                    MAKE_SERVICE_COMMAND_META(ClearArguments),

                    MAKE_SERVICE_COMMAND_META(AtmosphereSetExternalContentSource),
                    MAKE_SERVICE_COMMAND_META(AtmosphereClearExternalContentSource),
                };
        };

    }

}
