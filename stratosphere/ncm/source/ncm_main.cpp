/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include <stratosphere/lr/lr_location_resolver_manager_impl.hpp>
#include "ncm_content_manager_impl.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x100000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::ProgramId::Ncm;

    namespace result {

        bool CallFatalOnResultAssertion = false;

    }

}

using namespace ams;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
}

void __libnx_initheap(void) {
    void * addr = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    /* Newlib */
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    hos::SetVersionForLibnx();

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(fsInitialize());
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    fsExit();
}

namespace {

    struct ContentManagerServerOptions {
        static constexpr size_t PointerBufferSize = 0x400;
        static constexpr size_t MaxDomains = 0;
        static constexpr size_t MaxDomainObjects = 0;
    };

    constexpr inline size_t ContentManagerNumServers      = 1;
    constexpr inline size_t ContentManagerManagerSessions = 16;
    constexpr inline size_t ContentManagerExtraSessions   = 16;
    constexpr inline size_t ContentManagerMaxSessions     = ContentManagerManagerSessions + ContentManagerExtraSessions;

    constexpr inline sm::ServiceName ContentManagerServiceName = sm::ServiceName::Encode("ncm");

    class ContentManagerServerManager : public sf::hipc::ServerManager<ContentManagerNumServers, ContentManagerServerOptions, ContentManagerMaxSessions> {
        private:
            static constexpr size_t ThreadStackSize = 0x4000;
            static constexpr int    ThreadPriority  = 0x15;

            using ServiceType = ncm::ContentManagerImpl;
        private:
            os::StaticThread<ThreadStackSize> thread;
            std::shared_ptr<ServiceType> ncm_manager;
        private:
            static void ThreadFunction(void *_this) {
                reinterpret_cast<ContentManagerServerManager *>(_this)->LoopProcess();
            }
        public:
            ContentManagerServerManager(ServiceType *m)
                : thread(ThreadFunction, this, ThreadPriority), ncm_manager(sf::ServiceObjectTraits<ServiceType>::SharedPointerHelper::GetEmptyDeleteSharedPointer(m))
            {
                /* ... */
            }

            ams::Result Initialize() {
                return this->RegisterServer<ServiceType>(ContentManagerServiceName, ContentManagerManagerSessions, this->ncm_manager);
            }

            ams::Result StartThreads() {
                return this->thread.Start();
            }

            void Wait() {
                this->thread.Join();
            }
    };

    struct LocationResolverServerOptions {
        static constexpr size_t PointerBufferSize = 0x400;
        static constexpr size_t MaxDomains = 0;
        static constexpr size_t MaxDomainObjects = 0;
    };

    constexpr inline size_t LocationResolverNumServers      = 1;
    constexpr inline size_t LocationResolverManagerSessions = 16;
    constexpr inline size_t LocationResolverExtraSessions   = 16;
    constexpr inline size_t LocationResolverMaxSessions     = LocationResolverManagerSessions + LocationResolverExtraSessions;

    constexpr inline sm::ServiceName LocationResolverServiceName = sm::ServiceName::Encode("lr");

    class LocationResolverServerManager : public sf::hipc::ServerManager<LocationResolverNumServers, LocationResolverServerOptions, LocationResolverMaxSessions> {
        private:
            static constexpr size_t ThreadStackSize = 0x4000;
            static constexpr int    ThreadPriority  = 0x15;

            using ServiceType = lr::LocationResolverManagerImpl;
        private:
            os::StaticThread<ThreadStackSize> thread;
            std::shared_ptr<ServiceType> lr_manager;
        private:
            static void ThreadFunction(void *_this) {
                reinterpret_cast<LocationResolverServerManager *>(_this)->LoopProcess();
            }
        public:
            LocationResolverServerManager(ServiceType *m)
                : thread(ThreadFunction, this, ThreadPriority), lr_manager(sf::ServiceObjectTraits<ServiceType>::SharedPointerHelper::GetEmptyDeleteSharedPointer(m))
            {
                /* ... */
            }

            ams::Result Initialize() {
                return this->RegisterServer<ServiceType>(LocationResolverServiceName, LocationResolverManagerSessions, this->lr_manager);
            }

            ams::Result StartThreads() {
                return this->thread.Start();
            }

            void Wait() {
                this->thread.Join();
            }
    };

    ncm::ContentManagerImpl g_ncm_manager_service_object;
    ContentManagerServerManager g_ncm_server_manager(std::addressof(g_ncm_manager_service_object));

    lr::LocationResolverManagerImpl g_lr_manager_service_object;
    LocationResolverServerManager g_lr_server_manager(std::addressof(g_lr_manager_service_object));

}

void ContentManagerServerMain(void *arg) {
    /* Create services. */
    R_ABORT_UNLESS(g_ncm_server_manager.RegisterServer<ncm::ContentManagerImpl>(ContentManagerServiceName, ContentManagerManagerSessions));

    /* Loop forever, servicing our services. */
    g_ncm_server_manager.LoopProcess();
}

int main(int argc, char **argv)
{
    /* TODO: Initialize content manager implementation. */

    R_ABORT_UNLESS(g_ncm_server_manager.Initialize());
    R_ABORT_UNLESS(g_ncm_server_manager.StartThreads());

    R_ABORT_UNLESS(g_lr_server_manager.Initialize());
    R_ABORT_UNLESS(g_lr_server_manager.StartThreads());

    g_ncm_server_manager.Wait();
    g_lr_server_manager.Wait();

    return 0;
}
