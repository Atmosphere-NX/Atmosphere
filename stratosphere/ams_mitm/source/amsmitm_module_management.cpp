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
#include <stratosphere.hpp>
#include "amsmitm_module_management.hpp"
#include "amsmitm_module.hpp"

#include "fs_mitm/fsmitm_module.hpp"
#include "set_mitm/setmitm_module.hpp"
#include "bpc_mitm/bpcmitm_module.hpp"
#include "bpc_mitm/bpc_ams_module.hpp"
#include "ns_mitm/nsmitm_module.hpp"
#include "hid_mitm/hidmitm_module.hpp"
#include "sysupdater/sysupdater_module.hpp"

namespace ams::mitm {

    namespace {

        enum ModuleId : u32 {
            ModuleId_FsMitm,
            ModuleId_SetMitm,
            ModuleId_BpcMitm,
            ModuleId_BpcAms,
            ModuleId_NsMitm,
            ModuleId_HidMitm,
            ModuleId_Sysupdater,

            ModuleId_Count,
        };

        struct ModuleDefinition {
            ThreadFunc main;
            void *stack_mem;
            s32 priority;
            u32 stack_size;
        };

        template<class M>
        constexpr ModuleDefinition GetModuleDefinition() {
            using Traits = ModuleTraits<M>;

            return ModuleDefinition {
                .main = Traits::ThreadFunction,
                .stack_mem = Traits::Stack,
                .priority = Traits::ThreadPriority,
                .stack_size = static_cast<u32>(Traits::StackSize),
            };
        }

        ams::os::ThreadType g_module_threads[ModuleId_Count];

        constexpr ModuleDefinition g_module_definitions[ModuleId_Count] = {
            GetModuleDefinition<fs::MitmModule>(),
            GetModuleDefinition<settings::MitmModule>(),
            GetModuleDefinition<bpc::MitmModule>(),
            GetModuleDefinition<bpc_ams::MitmModule>(),
            GetModuleDefinition<ns::MitmModule>(),
            GetModuleDefinition<hid::MitmModule>(),
            GetModuleDefinition<sysupdater::MitmModule>(),
        };

    }

    void LaunchAllModules() {
        /* Create thread for each module. */
        for (u32 i = 0; i < static_cast<u32>(ModuleId_Count); i++) {
            const ModuleDefinition &cur_module = g_module_definitions[i];
            R_ABORT_UNLESS(os::CreateThread(g_module_threads + i, cur_module.main, nullptr, cur_module.stack_mem, cur_module.stack_size, cur_module.priority));
        }

        /* Start thread for each module. */
        for (u32 i = 0; i < static_cast<u32>(ModuleId_Count); i++) {
            os::StartThread(g_module_threads + i);
        }
    }

    void WaitAllModules() {
        /* Wait on thread for each module. */
        for (u32 i = 0; i < static_cast<u32>(ModuleId_Count); i++) {
            os::WaitThread(g_module_threads + i);
        }
    }

}
