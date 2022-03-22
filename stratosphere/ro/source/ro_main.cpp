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
#include "ro_debug_monitor_service.hpp"
#include "ro_ro_service.hpp"

namespace ams {

    namespace ro {

        namespace {

            /* ldr:ro, ro:dmnt, ro:1. */
            enum PortIndex {
                PortIndex_DebugMonitor,
                PortIndex_User,
                PortIndex_JitPlugin,
                PortIndex_Count,
            };

            constexpr sm::ServiceName DebugMonitorServiceName = sm::ServiceName::Encode("ro:dmnt");
            constexpr size_t          DebugMonitorMaxSessions = 2;

            /* NOTE: Official code passes 32 for ldr:ro max sessions. We will pass 2, because that's the actual limit. */
            constexpr sm::ServiceName UserServiceName = sm::ServiceName::Encode("ldr:ro");
            constexpr size_t          UserMaxSessions = 2;

            constexpr sm::ServiceName JitPluginServiceName = sm::ServiceName::Encode("ro:1");
            constexpr size_t          JitPluginMaxSessions = 2;

            static constexpr size_t MaxSessions = DebugMonitorMaxSessions + UserMaxSessions + JitPluginMaxSessions;

            using ServerOptions = sf::hipc::DefaultServerManagerOptions;

            class ServerManager final : public sf::hipc::ServerManager<PortIndex_Count, ServerOptions, MaxSessions> {
                private:
                    virtual ams::Result OnNeedsToAccept(int port_index, Server *server) override;
            };

            using Allocator     = sf::ExpHeapAllocator;
            using ObjectFactory = sf::ObjectFactory<sf::ExpHeapAllocator::Policy>;

            alignas(0x40) constinit u8 g_server_allocator_buffer[4_KB];
            lmem::HeapHandle g_server_heap_handle;
            Allocator g_server_allocator;

            ServerManager g_server_manager;

            ams::Result ServerManager::OnNeedsToAccept(int port_index, Server *server) {
                switch (port_index) {
                    case PortIndex_DebugMonitor:
                        return this->AcceptImpl(server, ObjectFactory::CreateSharedEmplaced<ro::impl::IDebugMonitorInterface, ro::DebugMonitorService>(std::addressof(g_server_allocator)));
                    case PortIndex_User:
                        return this->AcceptImpl(server, ObjectFactory::CreateSharedEmplaced<ro::impl::IRoInterface, ro::RoService>(std::addressof(g_server_allocator), ro::NrrKind_User));
                    case PortIndex_JitPlugin:
                        return this->AcceptImpl(server, ObjectFactory::CreateSharedEmplaced<ro::impl::IRoInterface, ro::RoService>(std::addressof(g_server_allocator), ro::NrrKind_JitPlugin));
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }

            void *Allocate(size_t size) {
                return lmem::AllocateFromExpHeap(g_server_heap_handle, size);
            }

            void Deallocate(void *p, size_t size) {
                AMS_UNUSED(size);

                return lmem::FreeToExpHeap(g_server_heap_handle, p);
            }

            void InitializeHeap() {
                /* Setup server allocator. */
                g_server_heap_handle = lmem::CreateExpHeap(g_server_allocator_buffer, sizeof(g_server_allocator_buffer), lmem::CreateOption_None);
            }

            void LoopServer() {
                /* Create services. */
                R_ABORT_UNLESS(ro::g_server_manager.RegisterServer(PortIndex_DebugMonitor, DebugMonitorServiceName, DebugMonitorMaxSessions));

                R_ABORT_UNLESS(ro::g_server_manager.RegisterServer(PortIndex_User, UserServiceName, UserMaxSessions));
                if (hos::GetVersion() >= hos::Version_7_0_0) {
                    R_ABORT_UNLESS(ro::g_server_manager.RegisterServer(PortIndex_JitPlugin, JitPluginServiceName, JitPluginMaxSessions));
                }

                /* Loop forever, servicing our services. */
                ro::g_server_manager.LoopProcess();
            }

        }

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize heap. */
            ro::InitializeHeap();

            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize fs. */
            fs::InitializeForSystem();
            fs::SetAllocator(ro::Allocate, ro::Deallocate);
            fs::SetEnabledAutoAbort(false);

            /* Initialize other services we need. */
            R_ABORT_UNLESS(setsysInitialize());

            /* Mount the SD card. */
            R_ABORT_UNLESS(fs::MountSdCard("sdmc"));

            /* Verify that we can sanely execute. */
            ams::CheckApiVersion();
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void Main() {
        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(ro, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(ro, Main));

        /* Attach the server allocator. */
        ro::g_server_allocator.Attach(ro::g_server_heap_handle);

        /* Initialize Debug config. */
        {
            spl::Initialize();
            ON_SCOPE_EXIT { spl::Finalize(); };

            ro::SetDevelopmentHardware(spl::IsDevelopment());
            ro::SetDevelopmentFunctionEnabled(spl::IsDevelopmentFunctionEnabled());
        }

        /* Run the ro server. */
        ro::LoopServer();
    }

}
