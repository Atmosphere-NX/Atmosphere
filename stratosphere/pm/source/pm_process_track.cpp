#include <switch.h>
#include <stratosphere.hpp>
#include "pm_process_track.hpp"
#include "pm_registration.hpp"

void ProcessTracking::MainLoop(void *arg) {
    /* Make a new waitable manager. */
    MultiThreadedWaitableManager *process_waiter = new MultiThreadedWaitableManager(1, U64_MAX);
    process_waiter->add_waitable(Registration::GetProcessLaunchStartEvent());
    Registration::SetProcessListManager(process_waiter);
    
    /* Service processes. */
    process_waiter->process();
    
    delete process_waiter;
    svcExitThread();
}