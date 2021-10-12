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
#include "../lm_service_name.hpp"
#include "lm_log_service_impl.hpp"
#include "lm_log_getter.hpp"

namespace ams::lm::srv {

    namespace {

        constexpr inline size_t LogSessionCountMax   = 42;
        constexpr inline size_t LogGetterSessionCountMax = 1;

        constexpr inline size_t SessionCountMax = LogSessionCountMax + LogGetterSessionCountMax;

        constexpr inline size_t PortCountMax = 2;

        struct ServerManagerOptions {
            static constexpr size_t PointerBufferSize   = 0x400;
            static constexpr size_t MaxDomains          = 31;
            static constexpr size_t MaxDomainObjects    = 61;
            static constexpr bool CanDeferInvokeRequest = false;
            static constexpr bool CanManageMitmServers  = false;
        };

        using ServerManager = sf::hipc::ServerManager<PortCountMax, ServerManagerOptions, SessionCountMax>;

        constinit util::TypedStorage<ServerManager> g_server_manager_storage;
        constinit ServerManager *g_server_manager = nullptr;

        constinit util::TypedStorage<psc::PmModule> g_pm_module_storage;
        constinit psc::PmModule *g_pm_module;
        constinit os::MultiWaitHolderType g_pm_module_holder;

        constexpr const psc::PmModuleId PmModuleDependencies[] = { psc::PmModuleId_TmaHostIo, psc::PmModuleId_Fs };

        /* Service objects. */
        constinit sf::UnmanagedServiceObject<lm::ILogService, lm::srv::LogServiceImpl> g_log_service_object;
        constinit sf::UnmanagedServiceObject<lm::ILogGetter, lm::srv::LogGetter> g_log_getter_service_object;

        constinit std::atomic<bool> g_is_sleeping = false;

    }

    bool IsSleeping() {
        return g_is_sleeping;
    }

    void InitializeIpcServer() {
        /* Check that we're not already initialized. */
        AMS_ABORT_UNLESS(g_server_manager == nullptr);
        AMS_ABORT_UNLESS(g_pm_module == nullptr);

        /* Create and initialize the psc module. */
        g_pm_module = util::ConstructAt(g_pm_module_storage);
        R_ABORT_UNLESS(g_pm_module->Initialize(psc::PmModuleId_Lm, PmModuleDependencies, util::size(PmModuleDependencies), os::EventClearMode_ManualClear));

        /* Create the psc module multi wait holder. */
        os::InitializeMultiWaitHolder(std::addressof(g_pm_module_holder), g_pm_module->GetEventPointer()->GetBase());
        os::SetMultiWaitHolderUserData(std::addressof(g_pm_module_holder), psc::PmModuleId_Lm);

        /* Create the server manager. */
        g_server_manager = util::ConstructAt(g_server_manager_storage);

        /* Add the pm module holder. */
        g_server_manager->AddUserMultiWaitHolder(std::addressof(g_pm_module_holder));

        /* Create services. */
        R_ABORT_UNLESS(g_server_manager->RegisterObjectForServer(g_log_service_object.GetShared(),        LogServiceName, LogSessionCountMax));
        R_ABORT_UNLESS(g_server_manager->RegisterObjectForServer(g_log_getter_service_object.GetShared(), LogGetterServiceName, LogGetterSessionCountMax));

        /* Start the server manager. */
        g_server_manager->ResumeProcessing();
    }

    void LoopIpcServer() {
        /* Check that we're initialized. */
        AMS_ABORT_UNLESS(g_server_manager != nullptr);

        /* Loop forever, servicing the server. */
        auto prev_state = psc::PmState_Unknown;
        while (true) {
            /* Get the next signaled holder. */
            auto *signaled_holder = g_server_manager->WaitSignaled();
            if (signaled_holder != std::addressof(g_pm_module_holder)) {
                /* If ipc, process. */
                R_ABORT_UNLESS(g_server_manager->Process(signaled_holder));
            } else {
                /* If pm module, clear the event. */
                g_pm_module->GetEventPointer()->Clear();
                g_server_manager->AddUserMultiWaitHolder(signaled_holder);

                /* Get the power state. */
                psc::PmState   pm_state;
                psc::PmFlagSet pm_flags;
                R_ABORT_UNLESS(g_pm_module->GetRequest(std::addressof(pm_state), std::addressof(pm_flags)));

                /* Handle the power state. */
                if (prev_state == psc::PmState_EssentialServicesAwake && pm_state == psc::PmState_MinimumAwake) {
                    g_is_sleeping = false;
                } else if (prev_state == psc::PmState_MinimumAwake && pm_state == psc::PmState_SleepReady) {
                    g_is_sleeping = true;
                } else if (pm_state == psc::PmState_ShutdownReady) {
                    g_is_sleeping = true;
                }

                /* Set the previous state. */
                prev_state = pm_state;

                /* Acknowledge the state transition. */
                R_ABORT_UNLESS(g_pm_module->Acknowledge(pm_state, ResultSuccess()));
            }
        }
    }

    void StopIpcServer() {
        /* Check that we're initialized. */
        AMS_ABORT_UNLESS(g_server_manager != nullptr);

        /* Stop the server manager. */
        g_server_manager->RequestStopProcessing();
    }

    void FinalizeIpcServer() {
        /* Check that we're initialized. */
        AMS_ABORT_UNLESS(g_server_manager != nullptr);

        /* Destroy the server manager. */
        std::destroy_at(g_server_manager);
        g_server_manager = nullptr;
    }

}
