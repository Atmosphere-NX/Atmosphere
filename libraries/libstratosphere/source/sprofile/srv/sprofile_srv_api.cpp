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
#include "sprofile_srv_profile_manager.hpp"
#include "sprofile_srv_service_for_bg_agent.hpp"
#include "sprofile_srv_service_for_system_process.hpp"

namespace ams::sprofile::srv {

    namespace {

        constexpr const ProfileManager::SaveDataInfo SaveDataInfo = {
            .id           = 0x8000000000000220,
            .mount_name   = "sprof",
            .size         = 0x1C0000,
            .journal_size = 0x80000,
            .flags        = fs::SaveDataFlags_KeepAfterResettingSystemSaveData,
        };

        constexpr const sm::ServiceName ServiceNameForBgAgent       = sm::ServiceName::Encode("sprof:bg");
        constexpr const sm::ServiceName ServiceNameForSystemProcess = sm::ServiceName::Encode("sprof:sp");

        constexpr inline size_t BgAgentSessionCountMax       = 2;
        constexpr inline size_t SystemProcessSessionCountMax = 10;

        constexpr inline size_t SessionCountMax = BgAgentSessionCountMax + SystemProcessSessionCountMax;

        constexpr inline size_t PortCountMax = 2;

        struct ServerManagerOptions {
            static constexpr size_t PointerBufferSize   = 0x0;
            static constexpr size_t MaxDomains          = SessionCountMax; /* NOTE: Official is 9 */
            static constexpr size_t MaxDomainObjects    = 16;              /* NOTE: Official is 14 */
            static constexpr bool CanDeferInvokeRequest = false;
            static constexpr bool CanManageMitmServers  = false;
        };

        using ServerManager = sf::hipc::ServerManager<PortCountMax, ServerManagerOptions, SessionCountMax>;

        constinit util::TypedStorage<ProfileManager> g_profile_manager;

        constinit util::TypedStorage<sf::UnmanagedServiceObject<ISprofileServiceForBgAgent, ServiceForBgAgent>> g_bg_service_object;
        constinit util::TypedStorage<sf::UnmanagedServiceObject<ISprofileServiceForSystemProcess, ServiceForSystemProcess>> g_sp_service_object;

        constinit util::TypedStorage<ServerManager> g_server_manager;

        alignas(os::ThreadStackAlignment) constinit u8 g_ipc_thread_stack[0x3000];
        constinit u8 g_heap[16_KB];

        constinit os::ThreadType g_ipc_thread;

        constinit lmem::HeapHandle g_heap_handle;
        constinit sf::ExpHeapMemoryResource g_sf_memory_resource;

        void IpcServerThreadFunction(void *) {
            /* Get the server manager. */
            auto &server_manager = util::GetReference(g_server_manager);

            /* Resume processing. */
            server_manager.ResumeProcessing();

            /* Loop processing. */
            server_manager.LoopProcess();
        }

    }

    void Initialize() {
        /* Initialize heap. */
        g_heap_handle = lmem::CreateExpHeap(g_heap, sizeof(g_heap), lmem::CreateOption_ThreadSafe);

        /* Attach the memory resource to heap. */
        g_sf_memory_resource.Attach(g_heap_handle);

        /* Create the profile manager. */
        util::ConstructAt(g_profile_manager, SaveDataInfo);

        /* Process profile manager savedata. */
        util::GetReference(g_profile_manager).InitializeSaveData();

        /* Create the service objects. */
        util::ConstructAt(g_bg_service_object, std::addressof(g_sf_memory_resource), util::GetPointer(g_profile_manager));
        util::ConstructAt(g_sp_service_object, std::addressof(g_sf_memory_resource), util::GetPointer(g_profile_manager));

        /* Create the server manager. */
        util::ConstructAt(g_server_manager);

        /* Create services. */
        R_ABORT_UNLESS(util::GetReference(g_server_manager).RegisterObjectForServer(util::GetReference(g_bg_service_object).GetShared(), ServiceNameForBgAgent, BgAgentSessionCountMax));
        R_ABORT_UNLESS(util::GetReference(g_server_manager).RegisterObjectForServer(util::GetReference(g_sp_service_object).GetShared(), ServiceNameForSystemProcess, SystemProcessSessionCountMax));
    }

    void StartIpcServer() {
        /* Create the ipc server thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_ipc_thread), IpcServerThreadFunction, nullptr, g_ipc_thread_stack, sizeof(g_ipc_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(sprofile, IpcServer)));
        os::SetThreadNamePointer(std::addressof(g_ipc_thread), AMS_GET_SYSTEM_THREAD_NAME(sprofile, IpcServer));

        /* Start the ipc server thread. */
        os::StartThread(std::addressof(g_ipc_thread));
    }

}
