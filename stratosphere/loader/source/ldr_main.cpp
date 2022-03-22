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
#include "ldr_development_manager.hpp"
#include "ldr_loader_service.hpp"

namespace ams {

    namespace ldr {

        namespace {

            constinit u8 g_heap_memory[16_KB];
            lmem::HeapHandle g_server_heap_handle;
            constinit ams::sf::ExpHeapAllocator g_server_allocator;

            void *Allocate(size_t size) {
                return lmem::AllocateFromExpHeap(g_server_heap_handle, size);
            }

            void Deallocate(void *p, size_t size) {
                AMS_UNUSED(size);
                return lmem::FreeToExpHeap(g_server_heap_handle, p);
            }

            void InitializeHeap() {
                g_server_heap_handle = lmem::CreateExpHeap(g_heap_memory, sizeof(g_heap_memory), lmem::CreateOption_None);
                g_server_allocator.Attach(g_server_heap_handle);
            }

        }

        namespace {

            struct ServerOptions {
                static constexpr size_t PointerBufferSize   = 0x400;
                static constexpr size_t MaxDomains          = 0;
                static constexpr size_t MaxDomainObjects    = 0;
                static constexpr bool CanDeferInvokeRequest = false;
                static constexpr bool CanManageMitmServers  = false;
            };

            /* ldr:pm, ldr:shel, ldr:dmnt. */
            enum PortIndex {
                PortIndex_ProcessManager,
                PortIndex_Shell,
                PortIndex_DebugMonitor,
                PortIndex_Count,
            };

            constexpr sm::ServiceName ProcessManagerServiceName = sm::ServiceName::Encode("ldr:pm");
            constexpr size_t          ProcessManagerMaxSessions = 1;

            constexpr sm::ServiceName ShellServiceName = sm::ServiceName::Encode("ldr:shel");
            constexpr size_t          ShellMaxSessions = 3;

            constexpr sm::ServiceName DebugMonitorServiceName = sm::ServiceName::Encode("ldr:dmnt");
            constexpr size_t          DebugMonitorMaxSessions = 3;

            constinit sf::UnmanagedServiceObject<impl::IProcessManagerInterface, LoaderService> g_pm_service;
            constinit sf::UnmanagedServiceObject<impl::IShellInterface, LoaderService> g_shell_service;
            constinit sf::UnmanagedServiceObject<impl::IDebugMonitorInterface, LoaderService> g_dmnt_service;

            constexpr size_t MaxSessions = ProcessManagerMaxSessions + ShellMaxSessions + DebugMonitorMaxSessions + 1;

            using ServerManager = ams::sf::hipc::ServerManager<PortIndex_Count, ServerOptions, MaxSessions>;

            ServerManager g_server_manager;

            void RegisterServiceSessions() {
                R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_pm_service.GetShared(), ProcessManagerServiceName, ProcessManagerMaxSessions));
                R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_shell_service.GetShared(), ShellServiceName, ShellMaxSessions));
                R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_dmnt_service.GetShared(), DebugMonitorServiceName, DebugMonitorMaxSessions));
            }

            void LoopProcess() {
                g_server_manager.LoopProcess();
            }

        }

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize heap. */
            ldr::InitializeHeap();

            /* Set fs allocator. */
            fs::SetAllocator(ldr::Allocate, ldr::Deallocate);

            /* Initialize services we need. */
            R_ABORT_UNLESS(sm::Initialize());

            fs::InitializeForSystem();
            lr::Initialize();
            R_ABORT_UNLESS(fsldrInitialize());
            spl::Initialize();

            /* Verify that we can sanely execute. */
            ams::CheckApiVersion();
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void NORETURN Exit(int rc) {
        AMS_UNUSED(rc);
        AMS_ABORT("Exit called by immortal process");
    }

    void Main() {
        /* Disable auto-abort in fs operations. */
        fs::SetEnabledAutoAbort(false);

        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(ldr, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(ldr, Main));

        /* Configure development. */
        /* NOTE: Nintendo really does call the getter function three times instead of caching the value. */
        ldr::SetDevelopmentForAcidProductionCheck(spl::IsDevelopment());
        ldr::SetDevelopmentForAntiDowngradeCheck(spl::IsDevelopment());
        ldr::SetDevelopmentForAcidSignatureCheck(spl::IsDevelopment());

        /* Register the loader services. */
        ldr::RegisterServiceSessions();

        /* Loop forever, servicing our services. */
        ldr::LoopProcess();

        /* This can never be reached. */
        AMS_ASSUME(false);
    }

}

/* Override operator new. */
void *operator new(size_t size) {
    return ams::ldr::Allocate(size);
}

void *operator new(size_t size, const std::nothrow_t &) {
    return ams::ldr::Allocate(size);
}

void operator delete(void *p) {
    return ams::ldr::Deallocate(p, 0);
}

void operator delete(void *p, size_t size) {
    return ams::ldr::Deallocate(p, size);
}
