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

    namespace cs {

        namespace {

            alignas(os::ThreadStackAlignment) constinit u8 g_shell_stack[4_KB];
            alignas(os::ThreadStackAlignment) constinit u8 g_runner_stack[4_KB];

            alignas(os::MemoryPageSize) constinit u8 g_heap_memory[32_KB];

            alignas(0x40) constinit u8 g_htcs_buffer[2_KB];

            constinit os::SdkMutex g_heap_mutex;
            constinit lmem::HeapHandle g_heap_handle;

            void *Allocate(size_t size) {
                std::scoped_lock lk(g_heap_mutex);
                void *mem = lmem::AllocateFromExpHeap(g_heap_handle, size);
                return mem;
            }

            void Deallocate(void *p, size_t size) {
                AMS_UNUSED(size);

                std::scoped_lock lk(g_heap_mutex);
                lmem::FreeToExpHeap(g_heap_handle, p);
            }

            void InitializeHeap() {
                std::scoped_lock lk(g_heap_mutex);
                g_heap_handle = lmem::CreateExpHeap(g_heap_memory, sizeof(g_heap_memory), lmem::CreateOption_None);
            }

        }

        namespace {

            constinit ::ams::cs::CommandProcessor g_command_processor;

            constinit ::ams::scs::ShellServer g_shell_server;
            constinit ::ams::scs::ShellServer g_runner_server;

            constinit sf::UnmanagedServiceObject<htc::tenv::IServiceManager, htc::tenv::ServiceManager> g_tenv_service_manager;

            void InitializeCommandProcessor() {
                g_command_processor.Initialize();
            }

            void InitializeShellServers() {
                g_shell_server.Initialize("iywys@$cs", g_shell_stack, sizeof(g_shell_stack), std::addressof(g_command_processor));
                g_shell_server.Start();

                g_runner_server.Initialize("iywys@$csForRunnerTools", g_runner_stack, sizeof(g_runner_stack), std::addressof(g_command_processor));
                g_runner_server.Start();
            }

        }

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize heap. */
            cs::InitializeHeap();

            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize fs. */
            fs::InitializeForSystem();
            fs::SetAllocator(cs::Allocate, cs::Deallocate);
            fs::SetEnabledAutoAbort(false);

            /* Initialize other services we need. */
            lr::Initialize();
            R_ABORT_UNLESS(ldr::InitializeForShell());
            R_ABORT_UNLESS(pgl::Initialize());
            R_ABORT_UNLESS(setsysInitialize());

            /* Verify that we can sanely execute. */
            ams::CheckApiVersion();
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void Main() {
        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(cs, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(cs, Main));

        /* Initialize htcs. */
        constexpr auto HtcsSocketCountMax = 6;
        const size_t buffer_size = htcs::GetWorkingMemorySize(2 * HtcsSocketCountMax);
        AMS_ABORT_UNLESS(sizeof(cs::g_htcs_buffer) >= buffer_size);
        htcs::InitializeForSystem(cs::g_htcs_buffer, buffer_size, HtcsSocketCountMax);

        /* Initialize audio server. */
        cs::InitializeAudioServer();

        /* Initialize remote video server. */
        cs::InitializeRemoteVideoServer();

        /* Initialize hid server. */
        cs::InitializeHidServer();

        /* Initialize target io server. */
        cs::InitializeTargetIoServer();

        /* Initialize command processor. */
        cs::InitializeCommandProcessor();

        /* Setup scs. */
        scs::InitializeShell();

        /* Setup target environment service. */
        scs::InitializeTenvServiceManager();

        /* Initialize the shell servers. */
        cs::InitializeShellServers();

        /* Register htc:tenv. */
        R_ABORT_UNLESS(scs::GetServerManager()->RegisterObjectForServer(cs::g_tenv_service_manager.GetShared(), htc::tenv::ServiceName, scs::SessionCount[scs::Port_HtcTenv]));

        /* Start the scs ipc server. */
        scs::StartServer();
    }

}
