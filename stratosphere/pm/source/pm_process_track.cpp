#include <switch.h>
#include <stratosphere.hpp>
#include "pm_process_track.hpp"
#include "pm_registration.hpp"
#include "pm_debug.hpp"

void ProcessTracking::MainLoop(void *arg) {
    /* Make a new waitable manager. */
    WaitableManager *process_waiter = new WaitableManager(1000);
    process_waiter->add_waitable(Registration::GetProcessList());
    
    static unsigned int i = 0;
    /* Service processes. */
    while (true) {
        process_waiter->process_until_timeout();
        
        i++;
        if (Registration::TryWaitProcessLaunchStartEvent()) {
            Registration::HandleProcessLaunch();
        }
        if (i > 1000000) {
            Reboot();
        }
    }
    
    delete process_waiter;
    svcExitThread();
}