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
#include "pgl_srv_shell.hpp"
#include "pgl_srv_shell_event_observer.hpp"
#include "pgl_srv_tipc_utils.hpp"

namespace ams::pgl::srv {

    namespace {

        /* pgl. */
        enum PortIndex {
            PortIndex_Shell,
            PortIndex_Count,
        };

        constexpr sm::ServiceName ShellServiceName = sm::ServiceName::Encode("pgl");
        constexpr size_t          ShellMaxSessions = 8; /* Official maximum is 6. */

        using CmifServerManager = ams::sf::hipc::ServerManager<PortIndex_Count>;

        constexpr size_t          ObserverMaxSessions = 4;

        using ShellPortMeta     = ams::tipc::PortMeta<ShellMaxSessions + ObserverMaxSessions, pgl::tipc::IShellInterface, pgl::srv::ShellInterfaceTipc, ams::tipc::SingletonAllocator>;

        using TipcServerManager = ams::tipc::ServerManager<ShellPortMeta>;

        /* NOTE: Nintendo reserves only 0x2000 bytes for heap, which is used "mostly" to allocate shell event observers. */
        /* However, we would like very much for homebrew sysmodules to be able to subscribe to events if they so choose */
        /* And so we will use a larger heap (32 KB). Note that we reduce the heap size for tipc, where objects are */
        /* allocated statically. */
        /* We should have a smaller memory footprint than N in the end, regardless. */

        struct CmifGlobals {
            u8 heap_memory[32_KB];
            lmem::HeapHandle heap_handle;
            ams::sf::ExpHeapAllocator server_allocator;
            ams::sf::UnmanagedServiceObject<pgl::sf::IShellInterface, pgl::srv::ShellInterfaceCmif> shell_interface{std::addressof(server_allocator)};
            CmifServerManager server_manager;
        };

        struct TipcGlobals {
            u8 heap_memory[24_KB];
            lmem::HeapHandle heap_handle;
            TipcServerManager server_manager;
            ams::tipc::SlabAllocator<ams::tipc::ServiceObject<pgl::tipc::IEventObserver, pgl::srv::ShellEventObserverTipc>, ObserverMaxSessions> observer_allocator;
        };

        constinit union {
            util::TypedStorage<CmifGlobals> cmif;
            util::TypedStorage<TipcGlobals> tipc;
        } g_globals;

        ALWAYS_INLINE CmifGlobals &GetGlobalsForCmif() {
            return GetReference(g_globals.cmif);
        }

        ALWAYS_INLINE TipcGlobals &GetGlobalsForTipc() {
            return GetReference(g_globals.tipc);
        }

        ALWAYS_INLINE bool UseTipcServer() {
            return hos::GetVersion() >= hos::Version_12_0_0;
        }

        ALWAYS_INLINE lmem::HeapHandle GetHeapHandle() {
            if (UseTipcServer()) {
                return GetGlobalsForTipc().heap_handle;
            } else {
                return GetGlobalsForCmif().heap_handle;
            }
        }

        template<typename T>
        ALWAYS_INLINE void InitializeHeapImpl(util::TypedStorage<T> &globals_storage) {
            /* Construct the globals object. */
            util::ConstructAt(globals_storage);

            /* Get reference to the globals. */
            auto &globals = GetReference(globals_storage);

            /* Set the heap handle. */
            globals.heap_handle = lmem::CreateExpHeap(globals.heap_memory, sizeof(globals.heap_memory), lmem::CreateOption_ThreadSafe);

            /* If we should, setup the server allocator. */
            if constexpr (requires (T &t) { t.server_allocator; }) {
                globals.server_allocator.Attach(globals.heap_handle);
            }
        }


        void RegisterServiceSession() {
            /* Register "pgl" with the appropriate server manager. */
            if (UseTipcServer()) {
                /* Get the globals. */
                auto &globals = GetGlobalsForTipc();

                /* Initialize the server manager. */
                globals.server_manager.Initialize();

                /* Register the pgl service. */
                globals.server_manager.RegisterPort(ShellServiceName, ShellMaxSessions);
            } else {
                /* Get the globals. */
                auto &globals = GetGlobalsForCmif();

                /* Register the shell server with the cmif server manager. */
                R_ABORT_UNLESS(globals.server_manager.RegisterObjectForServer(globals.shell_interface.GetShared(), ShellServiceName, ShellMaxSessions));
            }
        }

        void LoopProcessServer() {
            /* Loop processing for the appropriate server manager. */
            if (UseTipcServer()) {
                GetGlobalsForTipc().server_manager.LoopAuto();
            } else {
                GetGlobalsForCmif().server_manager.LoopProcess();
            }
        }

    }

    void InitializeHeap() {
        /* Initialize the heap (and construct the globals object) for the appropriate ipc protocol. */
        if (UseTipcServer()) {
            /* We're servicing via tipc. */
            InitializeHeapImpl(g_globals.tipc);
        } else {
            /* We're servicing via cmif. */
            InitializeHeapImpl(g_globals.cmif);
        }
    }

    void *Allocate(size_t size) {
        return lmem::AllocateFromExpHeap(GetHeapHandle(), size);
    }

    void Deallocate(void *p, size_t size) {
        AMS_UNUSED(size);
        return lmem::FreeToExpHeap(GetHeapHandle(), p);
    }

    void StartServer() {
        /* Enable extra application threads, if we should. */
        u8 enable_application_extra_thread;
        const size_t sz = settings::fwdbg::GetSettingsItemValue(std::addressof(enable_application_extra_thread), sizeof(enable_application_extra_thread), "application_extra_thread", "enable_application_extra_thread");
        if (sz == sizeof(enable_application_extra_thread) && enable_application_extra_thread != 0) {
            /* NOTE: Nintendo does not check that this succeeds. */
            pm::shell::EnableApplicationExtraThread();
        }

        /* Register service session. */
        RegisterServiceSession();

        /* Start the Process Tracking thread. */
        pgl::srv::InitializeProcessControlTask();

        /* Loop process. */
        LoopProcessServer();
    }

    Result AllocateShellEventObserverForTipc(os::NativeHandle *out) {
        /* Get the shell event observer allocator. */
        auto &allocator = GetGlobalsForTipc().observer_allocator;

        /* Allocate an object. */
        auto *object = allocator.Allocate();
        R_UNLESS(object != nullptr, pgl::ResultOutOfMemory());

        /* Set the object's deleter. */
        object->SetDeleter(std::addressof(allocator));

        /* Add the session to the server manager. */
        /* NOTE: If this fails, the object will be leaked. */
        /* TODO: Should we avoid leaking the object? Nintendo does not. */
        R_TRY(GetGlobalsForTipc().server_manager.AddSession(out, object));

        return ResultSuccess();
    }

}