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
#include <stratosphere/pm.hpp>

namespace sts::pm::dmnt {

    class DebugMonitorServiceBase : public IServiceObject {
        protected:
            /* Actual command implementations. */
            virtual Result GetModuleIdList(Out<u32> out_count, OutBuffer<u8> out_buf, u64 unused);
            virtual Result GetExceptionProcessIdList(Out<u32> out_count, OutBuffer<u64> out_process_ids);
            virtual Result StartProcess(u64 process_id);
            virtual Result GetProcessId(Out<u64> out, ncm::TitleId title_id);
            virtual Result HookToCreateProcess(Out<CopiedHandle> out_hook, ncm::TitleId title_id);
            virtual Result GetApplicationProcessId(Out<u64> out);
            virtual Result HookToCreateApplicationProcess(Out<CopiedHandle> out_hook);
            virtual Result ClearHook(u32 which);

            /* Atmosphere extension commands. */
            virtual Result AtmosphereGetProcessInfo(Out<CopiedHandle> out_process_handle, Out<ncm::TitleLocation> out_loc, u64 process_id);
            virtual Result AtmosphereGetCurrentLimitInfo(Out<u64> out_cur_val, Out<u64> out_lim_val, u32 group, u32 resource);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                /* No entries, because DebugMonitorServiceBase is abstract. */
            };
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
                MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetExceptionProcessIdList),
                MAKE_SERVICE_COMMAND_META(DebugMonitorService, StartProcess),
                MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetProcessId),
                MAKE_SERVICE_COMMAND_META(DebugMonitorService, HookToCreateProcess),
                MAKE_SERVICE_COMMAND_META(DebugMonitorService, GetApplicationProcessId),
                MAKE_SERVICE_COMMAND_META(DebugMonitorService, HookToCreateApplicationProcess),

                /* 6.0.0-* */
                MAKE_SERVICE_COMMAND_META(DebugMonitorService, ClearHook, FirmwareVersion_600),

                /* Atmosphere extensions. */
                MAKE_SERVICE_COMMAND_META(DebugMonitorService, AtmosphereGetProcessInfo),
                MAKE_SERVICE_COMMAND_META(DebugMonitorService, AtmosphereGetCurrentLimitInfo),
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
                MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, GetModuleIdList),
                MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, GetExceptionProcessIdList),
                MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, StartProcess),
                MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, GetProcessId),
                MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, HookToCreateProcess),
                MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, GetApplicationProcessId),
                MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, HookToCreateApplicationProcess),

                /* Atmosphere extensions. */
                MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, AtmosphereGetProcessInfo),
                MAKE_SERVICE_COMMAND_META(DebugMonitorServiceDeprecated, AtmosphereGetCurrentLimitInfo),
            };
    };

}
