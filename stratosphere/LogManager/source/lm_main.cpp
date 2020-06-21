#include "lm_service.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;
    
    #define INNER_HEAP_SIZE 0x20000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::LogManager;

    namespace result {

        /* Fatal is launched after we are launched, so disable this. */
        bool CallFatalOnResultAssertion = false;

    }

}

using namespace ams;

void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    /* Initialize services. */
    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(pminfoInitialize());
        R_ABORT_UNLESS(fsInitialize());
        R_ABORT_UNLESS(pscmInitialize());
        R_ABORT_UNLESS(setsysInitialize());
    });

    R_ABORT_UNLESS(fs::MountSdCard("sdmc"));

    ams::CheckApiVersion();
}

void __appExit(void) {
    fs::Unmount("sdmc");

    /* Cleanup services. */
    setsysExit();
    pscmExit();
    fsExit();
    pminfoExit();
}

namespace {

    /* TODO: these domain/domain object amounts work fine, but which ones does N actually use? */
    struct ServerOptions {
        static constexpr size_t PointerBufferSize = 0;
        static constexpr size_t MaxDomains = 0x40;
        static constexpr size_t MaxDomainObjects = 0x200;
    };

    constexpr sm::ServiceName LmServiceName = sm::ServiceName::Encode("lm");
    constexpr size_t LmMaxSessions = 42;

    /* lm */
    constexpr size_t NumServers = 1;
    sf::hipc::ServerManager<NumServers, ServerOptions, LmMaxSessions> g_server_manager;

    psc::PmModule g_pm_module;
    os::WaitableHolderType g_module_waitable_holder;

}

namespace ams::lm {

    void StartAndLoopProcess() {
        /* Get our psc:m module to handle requests. */
        R_ABORT_UNLESS(g_pm_module.Initialize(psc::PmModuleId_Lm, nullptr, 0, os::EventClearMode_ManualClear));
        os::InitializeWaitableHolder(std::addressof(g_module_waitable_holder), g_pm_module.GetEventPointer()->GetBase());
        os::SetWaitableHolderUserData(std::addressof(g_module_waitable_holder), static_cast<uintptr_t>(psc::PmModuleId_Lm));
        g_server_manager.AddUserWaitableHolder(std::addressof(g_module_waitable_holder));
        
        psc::PmState pm_state;
        psc::PmFlagSet pm_flags;
        while(true) {
            auto *signaled_holder = g_server_manager.WaitSignaled();
            if(signaled_holder != std::addressof(g_module_waitable_holder)) {
                R_ABORT_UNLESS(g_server_manager.Process(signaled_holder));
            }
            else {
                g_pm_module.GetEventPointer()->Clear();
                if(R_SUCCEEDED(g_pm_module.GetRequest(std::addressof(pm_state), std::addressof(pm_flags)))) {
                    switch(pm_state) {
                        case psc::PmState_Awake:
                        case psc::PmState_ReadyAwaken:
                            /* Awake, enable logging. */
                            impl::SetCanAccessFs(true);
                            break;
                        case psc::PmState_ReadySleep:
                        case psc::PmState_ReadyShutdown:
                            /* Sleep, disable logging. */
                            impl::SetCanAccessFs(false);
                            break;
                        default:
                            break;
                    }
                    R_ABORT_UNLESS(g_pm_module.Acknowledge(pm_state, ResultSuccess()));
                }
                g_server_manager.AddUserWaitableHolder(signaled_holder);
            }
        }
    }

}

int main(int argc, char **argv) {
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(lm, IpcServer));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(lm, IpcServer));

    /* Clear logs directory. */
    lm::impl::ClearDebugLogs();

    /* Add service to manager. */
    R_ABORT_UNLESS(g_server_manager.RegisterServer<lm::LogService>(LmServiceName, LmMaxSessions));
 
    /* Loop forever, servicing our services. */
    lm::StartAndLoopProcess();
 
    /* Cleanup */
    return 0;
}