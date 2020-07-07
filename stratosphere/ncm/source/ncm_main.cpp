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

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 2;

    #define INNER_HEAP_SIZE 0x8000
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

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Ncm;

    namespace result {

        bool CallFatalOnResultAssertion = false;

    }

}

using namespace ams;

namespace {

    u8 g_heap_memory[1_MB];
    lmem::HeapHandle g_heap_handle;

    void *Allocate(size_t size) {
        void *mem = lmem::AllocateFromExpHeap(g_heap_handle, size);
        ncm::GetHeapState().Allocate(size);
        return mem;
    }

    void Deallocate(void *p, size_t size) {
        ncm::GetHeapState().Free(size != 0 ? size : lmem::GetExpHeapMemoryBlockSize(p));
        lmem::FreeToExpHeap(g_heap_handle, p);
    }

    void InitializeHeap() {
        g_heap_handle = lmem::CreateExpHeap(g_heap_memory, sizeof(g_heap_memory), lmem::CreateOption_ThreadSafe);
        ncm::GetHeapState().Initialize(g_heap_handle);
    }

}

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

    InitializeHeap();
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    fs::SetAllocator(Allocate, Deallocate);

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(fsInitialize());
        spl::Initialize();
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    spl::Finalize();
    fsExit();
}

void *operator new(size_t size) {
    return Allocate(size);
}

void *operator new(size_t size, const std::nothrow_t &) {
    return Allocate(size);
}

void operator delete(void *p) {
    return Deallocate(p, 0);
}

void *operator new[](size_t size) {
    return Allocate(size);
}

void *operator new[](size_t size, const std::nothrow_t &) {
    return Allocate(size);
}

void operator delete[](void *p) {
    return Deallocate(p, 0);
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

    alignas(os::ThreadStackAlignment) u8 g_content_manager_thread_stack[16_KB];
    alignas(os::ThreadStackAlignment) u8 g_location_resolver_thread_stack[16_KB];

    class ContentManagerServerManager : public sf::hipc::ServerManager<ContentManagerNumServers, ContentManagerServerOptions, ContentManagerMaxSessions> {
        private:
            using Interface   = ncm::IContentManager;
            using ServiceImpl = ncm::ContentManagerImpl;
        private:
            os::ThreadType thread;
            std::shared_ptr<Interface> ncm_manager;
        private:
            static void ThreadFunction(void *_this) {
                reinterpret_cast<ContentManagerServerManager *>(_this)->LoopProcess();
            }
        public:
            ContentManagerServerManager() : ncm_manager() { /* ... */ }

            ams::Result Initialize(std::shared_ptr<Interface> manager_obj) {
                this->ncm_manager = manager_obj;
                return this->RegisterServer<Interface, ServiceImpl>(ContentManagerServiceName, ContentManagerManagerSessions, this->ncm_manager);
            }

            ams::Result StartThreads() {
                R_TRY(os::CreateThread(std::addressof(this->thread), ThreadFunction, this, g_content_manager_thread_stack, sizeof(g_content_manager_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(ncm, ContentManagerServerIpcSession)));
                os::SetThreadNamePointer(std::addressof(this->thread),  AMS_GET_SYSTEM_THREAD_NAME(ncm, ContentManagerServerIpcSession));
                os::StartThread(std::addressof(this->thread));
                return ResultSuccess();
            }

            void Wait() {
                os::WaitThread(std::addressof(this->thread));
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
            using Interface   = lr::ILocationResolverManager;
            using ServiceImpl = lr::LocationResolverManagerImpl;
        private:
            os::ThreadType thread;
            std::shared_ptr<Interface> lr_manager;
        private:
            static void ThreadFunction(void *_this) {
                reinterpret_cast<LocationResolverServerManager *>(_this)->LoopProcess();
            }
        public:
            LocationResolverServerManager(ServiceImpl &m) : lr_manager(sf::GetSharedPointerTo<Interface>(m)) { /* ... */ }

            ams::Result Initialize() {
                return this->RegisterServer<Interface, ServiceImpl>(LocationResolverServiceName, LocationResolverManagerSessions, this->lr_manager);
            }

            ams::Result StartThreads() {
                R_TRY(os::CreateThread(std::addressof(this->thread), ThreadFunction, this, g_location_resolver_thread_stack, sizeof(g_location_resolver_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(ncm, LocationResolverServerIpcSession)));
                os::SetThreadNamePointer(std::addressof(this->thread), AMS_GET_SYSTEM_THREAD_NAME(ncm, LocationResolverServerIpcSession));
                os::StartThread(std::addressof(this->thread));
                return ResultSuccess();
            }

            void Wait() {
                os::WaitThread(std::addressof(this->thread));
            }
    };

    ncm::ContentManagerImpl g_ncm_manager_service_object;
    ContentManagerServerManager g_ncm_server_manager;

    lr::LocationResolverManagerImpl g_lr_manager_service_object;
    LocationResolverServerManager g_lr_server_manager(g_lr_manager_service_object);

    /* Compile-time configuration. */
#ifdef NCM_BUILD_FOR_INTITIALIZE
    constexpr inline bool BuildSystemDatabase = true;
#else
    constexpr inline bool BuildSystemDatabase = false;
#endif

#ifdef NCM_BUILD_FOR_SAFEMODE
    constexpr inline bool ImportSystemDatabaseFromSignedSystemPartitionOnSdCard = true;
#else
    constexpr inline bool ImportSystemDatabaseFromSignedSystemPartitionOnSdCard = false;
#endif

    static_assert(!(BuildSystemDatabase && ImportSystemDatabaseFromSignedSystemPartitionOnSdCard), "Invalid NCM build configuration!");

    constexpr inline ncm::ContentManagerConfig ManagerConfig = { BuildSystemDatabase, ImportSystemDatabaseFromSignedSystemPartitionOnSdCard };

}

int main(int argc, char **argv)
{
    /* Disable auto-abort in fs operations. */
    fs::SetEnabledAutoAbort(false);

    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(ncm, MainWaitThreads));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(ncm, MainWaitThreads));

    /* Create and initialize the content manager. */
    auto content_manager = sf::GetSharedPointerTo<ncm::IContentManager>(g_ncm_manager_service_object);
    R_ABORT_UNLESS(content_manager->GetImpl().Initialize(ManagerConfig));

    /* Initialize ncm's server and start threads. */
    R_ABORT_UNLESS(g_ncm_server_manager.Initialize(content_manager));
    R_ABORT_UNLESS(g_ncm_server_manager.StartThreads());

    /* Initialize ncm api. */
    ncm::InitializeWithObject(content_manager);

    /* Initialize lr's server and start threads. */
    R_ABORT_UNLESS(g_lr_server_manager.Initialize());
    R_ABORT_UNLESS(g_lr_server_manager.StartThreads());

    /* Wait indefinitely. */
    g_ncm_server_manager.Wait();
    g_lr_server_manager.Wait();

    return 0;
}
