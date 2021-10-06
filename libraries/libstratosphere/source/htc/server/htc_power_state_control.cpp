/*
 * Copyright (c) Atmosphère-NX
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
#include "../../htclow/htclow_manager.hpp"

namespace ams::htc::server {

    namespace {

        constinit htclow::HtclowManager *g_htclow_manager = nullptr;

        constexpr const psc::PmModuleId PscModuleDependencies[] = { psc::PmModuleId_Pcie, psc::PmModuleId_Usb };
        psc::PmModule  g_pm_module;

        constinit bool g_is_asleep    = false;
        constinit bool g_is_suspended = false;

    }

    void InitializePowerStateMonitor(htclow::impl::DriverType driver_type, htclow::HtclowManager *htclow_manager) {
        AMS_UNUSED(driver_type);

        /* Set the htclow manager. */
        g_htclow_manager = htclow_manager;

        /* Initialize pm module. */
        R_ABORT_UNLESS(g_pm_module.Initialize(psc::PmModuleId_TmaHostIo, PscModuleDependencies, util::size(PscModuleDependencies), os::EventClearMode_AutoClear));

        /* We're neither asleep nor suspended. */
        g_is_asleep    = false;
        g_is_suspended = false;
    }

    void FinalizePowerStateMonitor() {
        R_ABORT_UNLESS(g_pm_module.Finalize());
    }

    void LoopMonitorPowerState() {
        /* Get the psc module's event pointer. */
        auto *event = g_pm_module.GetEventPointer();
        while (true) {
            /* Wait for a new power state event. */
            event->Wait();

            /* Get the power state. */
            psc::PmState   pm_state;
            psc::PmFlagSet pm_flags;
            R_ABORT_UNLESS(g_pm_module.GetRequest(std::addressof(pm_state), std::addressof(pm_flags)));

            /* Update sleeping state. */
            switch (pm_state) {
                case psc::PmState_FullAwake:
                    if (g_is_asleep) {
                        g_htclow_manager->NotifyAwake();
                        g_is_asleep = false;
                    }
                    break;
                case psc::PmState_MinimumAwake:
                case psc::PmState_SleepReady:
                case psc::PmState_EssentialServicesSleepReady:
                case psc::PmState_EssentialServicesAwake:
                    if (!g_is_asleep) {
                        g_htclow_manager->NotifyAsleep();
                        g_is_asleep = true;
                    }
                    break;
                default:
                    break;
            }

            /* Update suspend state. */
            switch (pm_state) {
                case psc::PmState_FullAwake:
                case psc::PmState_MinimumAwake:
                    if (g_is_suspended) {
                        g_htclow_manager->Resume();
                        g_is_suspended = false;
                    }
                    break;
                case psc::PmState_SleepReady:
                case psc::PmState_EssentialServicesSleepReady:
                case psc::PmState_EssentialServicesAwake:
                    if (!g_is_suspended) {
                        g_htclow_manager->Suspend();
                        g_is_suspended = true;
                    }
                    break;
                default:
                    break;
            }

            /* Acknowledge the pm request. */
            R_ABORT_UNLESS(g_pm_module.Acknowledge(pm_state, ResultSuccess()));
        }
    }

}
