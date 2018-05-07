#include <switch.h>
#include <stratosphere.hpp>
#include "pm_process_track.hpp"
#include "pm_registration.hpp"
#include "pm_debug.hpp"

void ProcessTracking::MainLoop(void *arg) {
    /* Make a new waitable manager. */
    WaitableManager *process_waiter = new WaitableManager(U64_MAX);
    process_waiter->add_waitable(Registration::GetProcessLaunchStartEvent());
    process_waiter->add_waitable(Registration::GetProcessList());
    
    /* Service processes. */
    process_waiter->process();
    
    delete process_waiter;
    svcExitThread();
}