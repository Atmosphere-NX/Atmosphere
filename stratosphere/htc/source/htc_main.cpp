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

namespace ams {

    namespace htc {

        namespace {

            alignas(0x40) constinit u8 g_heap_buffer[4_KB];
            lmem::HeapHandle g_heap_handle;

            void *Allocate(size_t size) {
                return lmem::AllocateFromExpHeap(g_heap_handle, size);
            }

            void Deallocate(void *p, size_t size) {
                AMS_UNUSED(size);

                return lmem::FreeToExpHeap(g_heap_handle, p);
            }

            void InitializeHeap() {
                /* Setup server allocator. */
                g_heap_handle = lmem::CreateExpHeap(g_heap_buffer, sizeof(g_heap_buffer), lmem::CreateOption_ThreadSafe);
            }

        }

        namespace {

            constexpr htclow::impl::DriverType DefaultHtclowDriverType = htclow::impl::DriverType::Usb;

            constexpr inline size_t NumHtcsIpcThreads = 8;

            alignas(os::ThreadStackAlignment) u8 g_htc_ipc_thread_stack[4_KB];
            alignas(os::ThreadStackAlignment) u8 g_htcfs_ipc_thread_stack[4_KB];
            alignas(os::ThreadStackAlignment) u8 g_htcs_ipc_thread_stack[NumHtcsIpcThreads][4_KB];

            htclow::impl::DriverType GetHtclowDriverType() {
                /* Get the transport type. */
                char transport[0x10];
                if (settings::fwdbg::GetSettingsItemValue(transport, sizeof(transport), "bsp0", "tm_transport") == 0) {
                    return DefaultHtclowDriverType;
                }

                /* Make the transport type case insensitive. */
                transport[util::size(transport) - 1] = '\x00';
                for (size_t i = 0; i < util::size(transport); ++i) {
                    transport[i] = std::tolower(static_cast<unsigned char>(transport[i]));
                }

                /* Select the transport. */
                if (std::strstr(transport, "usb")) {
                    return htclow::impl::DriverType::Usb;
                } else if (std::strstr(transport, "hb")) {
                    return htclow::impl::DriverType::HostBridge;
                } else if (std::strstr(transport, "plainchannel")) {
                    return htclow::impl::DriverType::PlainChannel;
                } else if (std::strstr(transport, "socket")) {
                    /* NOTE: Nintendo does not actually allow socket driver to be selected. */
                    /* Should we disallow this? Undesirable, because people will want to use docked tma. */

                    /* TODO: Right now, SocketDriver causes a hang on init. This is because */
                    /* the socket driver requires wi-fi, but wi-fi can't happen until the system is fully up. */
                    /* The system can't initialize fully until we acknowledge power state events. */
                    /* We can't acknowledge power state events until our driver is online. */
                    /* Resolving this chicken-and-egg problem without compromising design will require thought. */

                    //return htclow::impl::DriverType::Socket;
                    return DefaultHtclowDriverType;
                } else {
                    return DefaultHtclowDriverType;
                }
            }

            void HtcIpcThreadFunction(void *) {
                htc::server::LoopHtcmiscServer();
            }

            void HtcfsIpcThreadFunction(void *) {
                htcfs::LoopHipcServer();
            }

            void HtcsIpcThreadFunction(void *) {
                htcs::server::LoopHipcServer();
            }

        }

        namespace server {

            void InitializePowerStateMonitor(htclow::impl::DriverType driver_type, htclow::HtclowManager *htclow_manager);
            void FinalizePowerStateMonitor();

            void LoopMonitorPowerState();

        }

    }

    namespace htclow::driver {

        void InitializeSocketApiForSocketDriver();

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize heap. */
            htc::InitializeHeap();

            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize fs. */
            fs::InitializeForSystem();
            fs::SetAllocator(htc::Allocate, htc::Deallocate);
            fs::SetEnabledAutoAbort(false);

            /* Initialize other services we need. */
            R_ABORT_UNLESS(setsysInitialize());
            R_ABORT_UNLESS(setcalInitialize());
            R_ABORT_UNLESS(pscmInitialize());

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
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(htc, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(htc, Main));

        /* Get and set the default driver type. */
        const auto driver_type = htc::GetHtclowDriverType();
        htclow::HtclowManagerHolder::SetDefaultDriver(driver_type);

        /* If necessary, initialize the socket driver. */
        if (driver_type == htclow::impl::DriverType::Socket) {
            htclow::driver::InitializeSocketApiForSocketDriver();
        }

        /* Initialize the htclow manager. */
        htclow::HtclowManagerHolder::AddReference();
        ON_SCOPE_EXIT { htclow::HtclowManagerHolder::Release(); };

        /* Get the htclow manager. */
        auto *htclow_manager = htclow::HtclowManagerHolder::GetHtclowManager();

        /* Initialize the htc misc server. */
        htc::server::InitializeHtcmiscServer(htclow_manager);

        /* Create the htc misc ipc thread. */
        os::ThreadType htc_ipc_thread;
        os::CreateThread(std::addressof(htc_ipc_thread), htc::HtcIpcThreadFunction, nullptr, htc::g_htc_ipc_thread_stack, sizeof(htc::g_htc_ipc_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtcIpc));
        os::SetThreadNamePointer(std::addressof(htc_ipc_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, HtcIpc));

        /* Initialize the htcfs server. */
        htcfs::Initialize(htclow_manager);
        htcfs::RegisterHipcServer();

        /* Create the htcfs ipc thread. */
        os::ThreadType htcfs_ipc_thread;
        os::CreateThread(std::addressof(htcfs_ipc_thread), htc::HtcfsIpcThreadFunction, nullptr, htc::g_htcfs_ipc_thread_stack, sizeof(htc::g_htcfs_ipc_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtcfsIpc));
        os::SetThreadNamePointer(std::addressof(htcfs_ipc_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, HtcfsIpc));

        /* Initialize the htcs server. */
        htcs::server::Initialize();
        htcs::server::RegisterHipcServer();

        /* Create the htcs ipc threads. */
        os::ThreadType htcs_ipc_threads[htc::NumHtcsIpcThreads];
        for (size_t i = 0; i < htc::NumHtcsIpcThreads; ++i) {
            os::CreateThread(std::addressof(htcs_ipc_threads[i]), htc::HtcsIpcThreadFunction, nullptr, htc::g_htcs_ipc_thread_stack[i], sizeof(htc::g_htcs_ipc_thread_stack[i]), AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtcsIpc));
            os::SetThreadNamePointer(std::addressof(htcs_ipc_threads[i]), AMS_GET_SYSTEM_THREAD_NAME(htc, HtcsIpc));
        }

        /* Initialize psc. */
        htc::server::InitializePowerStateMonitor(driver_type, htclow_manager);

        /* Start all threads. */
        os::StartThread(std::addressof(htc_ipc_thread));
        os::StartThread(std::addressof(htcfs_ipc_thread));
        for (size_t i = 0; i < htc::NumHtcsIpcThreads; ++i) {
            os::StartThread(std::addressof(htcs_ipc_threads[i]));
        }

        /* Loop psc monitor. */
        htc::server::LoopMonitorPowerState();

        /* Destroy all threads. */
        for (size_t i = 0; i < htc::NumHtcsIpcThreads; ++i) {
            os::WaitThread(std::addressof(htcs_ipc_threads[i]));
            os::DestroyThread(std::addressof(htcs_ipc_threads[i]));
        }

        os::WaitThread(std::addressof(htcfs_ipc_thread));
        os::DestroyThread(std::addressof(htcfs_ipc_thread));
        os::WaitThread(std::addressof(htc_ipc_thread));
        os::DestroyThread(std::addressof(htc_ipc_thread));

        /* Finalize psc monitor. */
        htc::server::FinalizePowerStateMonitor();
    }

}
