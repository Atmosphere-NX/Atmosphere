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

namespace ams::pm::dmnt {

    class DebugMonitorServiceBase : public sf::IServiceObject {
        protected:
            /* Actual command implementations. */
            virtual Result GetModuleIdList(sf::Out<u32> out_count, const sf::OutBuffer &out_buf, u64 unused);
            virtual Result GetExceptionProcessIdList(sf::Out<u32> out_count, const sf::OutArray<os::ProcessId> &out_process_ids);
            virtual Result StartProcess(os::ProcessId process_id);
            virtual Result GetProcessId(sf::Out<os::ProcessId> out, ncm::ProgramId program_id);
            virtual Result HookToCreateProcess(sf::OutCopyHandle out_hook, ncm::ProgramId program_id);
            virtual Result GetApplicationProcessId(sf::Out<os::ProcessId> out);
            virtual Result HookToCreateApplicationProcess(sf::OutCopyHandle out_hook);
            virtual Result ClearHook(u32 which);

            /* Atmosphere extension commands. */
            virtual Result AtmosphereGetProcessInfo(sf::OutCopyHandle out_process_handle, sf::Out<ncm::ProgramLocation> out_loc, sf::Out<cfg::OverrideStatus> out_status, os::ProcessId process_id);
            virtual Result AtmosphereGetCurrentLimitInfo(sf::Out<u64> out_cur_val, sf::Out<u64> out_lim_val, u32 group, u32 resource);
    };

    /* This represents modern DebugMonitorService (5.0.0+). */
    class DebugMonitorService final : public DebugMonitorServiceBase {
        private:
            enum class CommandId {
                GetExceptionProcessIdList       = 0,
                StartProcess                    = 1,
                GetProcessId                    = 2,
                HookToCreateProcess             = 3,
                GetApplicationProcessId         = 4,
                HookToCreateApplicationProcess  = 5,

                ClearHook                       = 6,

                AtmosphereGetProcessInfo        = 65000,
                AtmosphereGetCurrentLimitInfo   = 65001,
            };
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                /* 5.0.0-* */
                MAKE_SERVICE_COMMAND_META(GetExceptionProcessIdList),
                MAKE_SERVICE_COMMAND_META(StartProcess),
                MAKE_SERVICE_COMMAND_META(GetProcessId),
                MAKE_SERVICE_COMMAND_META(HookToCreateProcess),
                MAKE_SERVICE_COMMAND_META(GetApplicationProcessId),
                MAKE_SERVICE_COMMAND_META(HookToCreateApplicationProcess),

                /* 6.0.0-* */
                MAKE_SERVICE_COMMAND_META(ClearHook, hos::Version_600),

                /* Atmosphere extensions. */
                MAKE_SERVICE_COMMAND_META(AtmosphereGetProcessInfo),
                MAKE_SERVICE_COMMAND_META(AtmosphereGetCurrentLimitInfo),
            };
    };

    /* This represents deprecated DebugMonitorService (1.0.0-4.1.0). */
    class DebugMonitorServiceDeprecated final : public DebugMonitorServiceBase {
        private:
            enum class CommandId {
                GetModuleIdList                 = 0,
                GetExceptionProcessIdList       = 1,
                StartProcess                    = 2,
                GetProcessId                    = 3,
                HookToCreateProcess             = 4,
                GetApplicationProcessId         = 5,
                HookToCreateApplicationProcess  = 6,

                AtmosphereGetProcessInfo        = 65000,
                AtmosphereGetCurrentLimitInfo   = 65001,
            };
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                /* 1.0.0-4.1.0 */
                MAKE_SERVICE_COMMAND_META(GetModuleIdList),
                MAKE_SERVICE_COMMAND_META(GetExceptionProcessIdList),
                MAKE_SERVICE_COMMAND_META(StartProcess),
                MAKE_SERVICE_COMMAND_META(GetProcessId),
                MAKE_SERVICE_COMMAND_META(HookToCreateProcess),
                MAKE_SERVICE_COMMAND_META(GetApplicationProcessId),
                MAKE_SERVICE_COMMAND_META(HookToCreateApplicationProcess),

                /* Atmosphere extensions. */
                MAKE_SERVICE_COMMAND_META(AtmosphereGetProcessInfo),
                MAKE_SERVICE_COMMAND_META(AtmosphereGetCurrentLimitInfo),
            };
    };

}
