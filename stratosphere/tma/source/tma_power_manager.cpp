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
#include "tma_power_manager.hpp"

static constexpr u16 PscPmModuleId_Usb  = 0x04;
static constexpr u16 PscPmModuleId_Pcie = 0x13;
static constexpr u16 PscPmModuleId_Tma  = 0x1E;

static const u16 g_tma_pm_dependencies[] = {
    PscPmModuleId_Usb,
};

static void (*g_pm_callback)(PscPmState, u32) = nullptr;
static HosThread g_pm_thread;

static void PowerManagerThread(void *arg) {
    /* Setup psc module. */
    Result rc;
    PscPmModule tma_module = {0};
    if (R_FAILED((rc = pscGetPmModule(&tma_module, PscPmModuleId_Tma, g_tma_pm_dependencies, sizeof(g_tma_pm_dependencies)/sizeof(u16), true)))) {
        fatalSimple(rc);
    }
    
    /* For now, just do what dummy tma does -- loop forever, acknowledging everything. */
    while (true) {
        if (R_FAILED((rc = eventWait(&tma_module.event, U64_MAX)))) {
            fatalSimple(rc);
        }
        
        PscPmState state;
        u32 flags;
        if (R_FAILED((rc = pscPmModuleGetRequest(&tma_module, &state, &flags)))) {
            fatalSimple(rc);
        }
        
        g_pm_callback(state, flags);
        
        if (R_FAILED((rc = pscPmModuleAcknowledge(&tma_module, state)))) {
            fatalSimple(rc);
        }
    }
}

void TmaPowerManager::Initialize(void (*callback)(PscPmState, u32)) {
    g_pm_callback = callback;
    g_pm_thread.Initialize(PowerManagerThread, nullptr, 0x4000, 0x15);
    g_pm_thread.Start();
}

void TmaPowerManager::Finalize() {
    /* TODO */
}