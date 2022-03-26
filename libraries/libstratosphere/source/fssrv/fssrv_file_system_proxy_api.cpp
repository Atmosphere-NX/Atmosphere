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
#include <stratosphere/fs/impl/fs_service_name.hpp>
#include "fssrv_deferred_process_manager.hpp"

namespace ams::fssrv {

    namespace {

        struct FileSystemProxyServerOptions {
            static constexpr size_t PointerBufferSize   = 0x800;
            static constexpr size_t MaxDomains          = 0x40;
            static constexpr size_t MaxDomainObjects    = 0x4000;
            static constexpr bool CanDeferInvokeRequest = true;
            static constexpr bool CanManageMitmServers  = false;
        };

        enum PortIndex {
            PortIndex_FileSystemProxy,
            PortIndex_ProgramRegistry,
            PortIndex_FileSystemProxyForLoader,
            PortIndex_Count,
        };

        constexpr size_t FileSystemProxyMaxSessions          = 59;
        constexpr size_t ProgramRegistryMaxSessions          = 1;
        constexpr size_t FileSystemProxyForLoaderMaxSessions = 1;

        constexpr size_t NumSessions = FileSystemProxyMaxSessions + ProgramRegistryMaxSessions + FileSystemProxyForLoaderMaxSessions;

        constinit os::SemaphoreType g_semaphore_for_file_system_proxy_for_loader = {};
        constinit os::SemaphoreType g_semaphore_for_program_registry = {};

        class FileSystemProxyServerManager final : public ams::sf::hipc::ServerManager<PortIndex_Count, FileSystemProxyServerOptions, NumSessions> {
            private:
                virtual ams::Result OnNeedsToAccept(int port_index, Server *server) override {
                    switch (port_index) {
                        case PortIndex_FileSystemProxy:
                        {
                            R_RETURN(this->AcceptImpl(server, impl::GetFileSystemProxyServiceObject()));
                        }
                        case PortIndex_ProgramRegistry:
                        {
                            if (os::TryAcquireSemaphore(std::addressof(g_semaphore_for_program_registry))) {
                                ON_RESULT_FAILURE { os::ReleaseSemaphore(std::addressof(g_semaphore_for_program_registry)); };

                                R_RETURN(this->AcceptImpl(server, impl::GetProgramRegistryServiceObject()));
                            } else {
                                R_RETURN(this->AcceptImpl(server, impl::GetInvalidProgramRegistryServiceObject()));
                            }
                        }
                        case PortIndex_FileSystemProxyForLoader:
                        {
                            if (os::TryAcquireSemaphore(std::addressof(g_semaphore_for_file_system_proxy_for_loader))) {
                                ON_RESULT_FAILURE { os::ReleaseSemaphore(std::addressof(g_semaphore_for_file_system_proxy_for_loader)); };

                                R_RETURN(this->AcceptImpl(server, impl::GetFileSystemProxyForLoaderServiceObject()));
                            } else {
                                R_RETURN(this->AcceptImpl(server, impl::GetInvalidFileSystemProxyForLoaderServiceObject()));
                            }
                        }
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }
                }
        };

        constinit util::TypedStorage<FileSystemProxyServerManager> g_server_manager_storage = {};
        constinit FileSystemProxyServerManager *g_server_manager = nullptr;

        constinit os::BarrierType g_server_loop_barrier = {};
        constinit os::EventType g_resume_wait_event = {};

        constinit bool g_is_suspended = false;

        void TemporaryNotifyProcessDeferred(u64) { /* TODO */ }

        constinit DeferredProcessManager<FileSystemProxyServerManager, TemporaryNotifyProcessDeferred> g_deferred_process_manager;

    }

    void InitializeForFileSystemProxy(const FileSystemProxyConfiguration &config) {
        /* TODO FS-REIMPL */
        AMS_UNUSED(config);
    }

    void InitializeFileSystemProxyServer(int threads) {
        /* Initialize synchronization primitives. */
        os::InitializeBarrier(std::addressof(g_server_loop_barrier), threads + 1);
        os::InitializeEvent(std::addressof(g_resume_wait_event), false, os::EventClearMode_ManualClear);
        g_is_suspended = false;
        os::InitializeSemaphore(std::addressof(g_semaphore_for_file_system_proxy_for_loader), 1, 1);
        os::InitializeSemaphore(std::addressof(g_semaphore_for_program_registry), 1, 1);

        /* Initialize deferred process manager. */
        g_deferred_process_manager.Initialize();

        /* Create the server and register our services. */
        AMS_ASSERT(g_server_manager == nullptr);
        g_server_manager = util::ConstructAt(g_server_manager_storage);

        /* TODO: Manager handler. */

        R_ABORT_UNLESS(g_server_manager->RegisterServer(PortIndex_FileSystemProxy, fs::impl::FileSystemProxyServiceName, FileSystemProxyMaxSessions));
        R_ABORT_UNLESS(g_server_manager->RegisterServer(PortIndex_ProgramRegistry, fs::impl::ProgramRegistryServiceName, ProgramRegistryMaxSessions));
        R_ABORT_UNLESS(g_server_manager->RegisterServer(PortIndex_FileSystemProxyForLoader, fs::impl::FileSystemProxyForLoaderServiceName, FileSystemProxyForLoaderMaxSessions));

        /* Enable processing on server. */
        g_server_manager->ResumeProcessing();
    }

}
