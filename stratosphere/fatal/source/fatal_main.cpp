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
#include "fatal_service.hpp"
#include "fatal_config.hpp"
#include "fatal_repair.hpp"
#include "fatal_font.hpp"

/* Set libnx graphics globals. */
extern "C" {

    u32 __nx_nv_transfermem_size = 0x40000;
    ViLayerFlags __nx_vi_stray_layer_flags = (ViLayerFlags)0;

}

namespace ams {

    namespace fatal::srv {

        namespace {

            constinit u8 g_fs_heap_memory[2_KB];
            lmem::HeapHandle g_fs_heap_handle;

            void *AllocateForFs(size_t size) {
                return lmem::AllocateFromExpHeap(g_fs_heap_handle, size);
            }

            void DeallocateForFs(void *p, size_t size) {
                AMS_UNUSED(size);
                return lmem::FreeToExpHeap(g_fs_heap_handle, p);
            }

            void InitializeFsHeap() {
                g_fs_heap_handle = lmem::CreateExpHeap(g_fs_heap_memory, sizeof(g_fs_heap_memory), lmem::CreateOption_ThreadSafe);
            }

        }

        namespace {

            using ServerOptions = sf::hipc::DefaultServerManagerOptions;

            constexpr sm::ServiceName UserServiceName = sm::ServiceName::Encode("fatal:u");
            constexpr size_t          UserMaxSessions = 4;

            constexpr sm::ServiceName PrivateServiceName = sm::ServiceName::Encode("fatal:p");
            constexpr size_t          PrivateMaxSessions = 4;

            /* fatal:u, fatal:p. */
            constexpr size_t NumServers  = 2;
            constexpr size_t NumSessions = UserMaxSessions + PrivateMaxSessions;

            sf::hipc::ServerManager<NumServers, ServerOptions, NumSessions> g_server_manager;

            constinit sf::UnmanagedServiceObject<fatal::impl::IService, fatal::srv::Service> g_user_service_object;
            constinit sf::UnmanagedServiceObject<fatal::impl::IPrivateService, fatal::srv::Service> g_private_service_object;

            void InitializeAndLoopIpcServer() {
                /* Create services. */
                R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_user_service_object.GetShared(),    UserServiceName, UserMaxSessions));
                R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_private_service_object.GetShared(), PrivateServiceName, PrivateMaxSessions));

                /* Add dirty event holder. */
                auto *dirty_event_holder = fatal::srv::GetFatalDirtyMultiWaitHolder();
                g_server_manager.AddUserMultiWaitHolder(dirty_event_holder);

                /* Loop forever, servicing our services. */
                /* Because fatal has a user wait holder, we need to specify how to process manually. */
                while (auto *signaled_holder = g_server_manager.WaitSignaled()) {
                    if (signaled_holder == dirty_event_holder) {
                        /* Dirty event holder was signaled. */
                        fatal::srv::OnFatalDirtyEvent();
                        g_server_manager.AddUserMultiWaitHolder(signaled_holder);
                    } else {
                        /* A server/session was signaled. Have the manager handle it. */
                        R_ABORT_UNLESS(g_server_manager.Process(signaled_holder));
                    }
                }
            }

        }

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize heap. */
            fatal::srv::InitializeFsHeap();

            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize fs. */
            fs::InitializeForSystem();
            fs::SetAllocator(fatal::srv::AllocateForFs, fatal::srv::DeallocateForFs);
            fs::SetEnabledAutoAbort(false);

            /* Initialize other services we need. */
            R_ABORT_UNLESS(setInitialize());
            R_ABORT_UNLESS(setsysInitialize());
            R_ABORT_UNLESS(pminfoInitialize());
            R_ABORT_UNLESS(i2cInitialize());
            R_ABORT_UNLESS(bpcInitialize());

            if (hos::GetVersion() >= hos::Version_8_0_0) {
                R_ABORT_UNLESS(clkrstInitialize());
            } else {
                R_ABORT_UNLESS(pcvInitialize());
            }

            R_ABORT_UNLESS(lblInitialize());
            R_ABORT_UNLESS(psmInitialize());
            R_ABORT_UNLESS(spsmInitialize());
            R_ABORT_UNLESS(plInitialize(::PlServiceType_User));
            gpio::Initialize();

            /* Mount the SD card. */
            R_ABORT_UNLESS(fs::MountSdCard("sdmc"));
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void Main() {
        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(fatal, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(fatal, Main));

        /* Load shared font. */
        R_ABORT_UNLESS(fatal::srv::font::InitializeSharedFont());

        /* Check whether we should throw fatal due to repair process. */
        fatal::srv::CheckRepairStatus();

        /* Loop processing the IPC server. */
        fatal::srv::InitializeAndLoopIpcServer();
    }

}
