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
 
#include <switch.h>
#include <stratosphere.hpp>
#include <cstring>
#include "debug.hpp"

#include "amsmitm_modules.hpp"

#include "fs_mitm/fsmitm_main.hpp"
#include "set_mitm/setmitm_main.hpp"
#include "bpc_mitm/bpcmitm_main.hpp"
#include "ns_mitm/nsmitm_main.hpp"

static HosThread g_module_threads[MitmModuleId_Count];

static const struct {
    ThreadFunc main;
    u32 priority;
    u32 stack_size;
} g_module_definitions[MitmModuleId_Count] = {
    { &FsMitmMain,  FsMitmPriority,  FsMitmStackSize },  /* FsMitm */
    { &SetMitmMain, SetMitmPriority, SetMitmStackSize }, /* SetMitm */
    { &BpcMitmMain, BpcMitmPriority, BpcMitmStackSize }, /* BpcMitm */
    { &NsMitmMain,  NsMitmPriority,  NsMitmStackSize },  /* NsMitm */
};

void LaunchAllMitmModules() {
    /* Create thread for each module. */
    for (u32 i = 0; i < static_cast<u32>(MitmModuleId_Count); i++) {
        const auto cur_module = &g_module_definitions[i];
        if (R_FAILED(g_module_threads[i].Initialize(cur_module->main, nullptr, cur_module->stack_size, cur_module->priority))) {
            std::abort();
        }
    }
    
    /* Start thread for each module. */
    for (u32 i = 0; i < static_cast<u32>(MitmModuleId_Count); i++) {
        if (R_FAILED(g_module_threads[i].Start())) {
            std::abort();
        }
    }
}

void WaitAllMitmModules() {
    /* Wait on thread for each module. */
    for (u32 i = 0; i < static_cast<u32>(MitmModuleId_Count); i++) {
        g_module_threads[i].Join();
    }
}