#include <mutex>
#include <switch.h>
#include <stratosphere.hpp>
#include "fsmitm_worker.hpp"

static SystemEvent *g_new_waitable_event = NULL;

static HosMutex g_new_waitable_mutex;
static HosSemaphore g_sema_new_waitable_finish;

static std::unique_ptr<WaitableManager> g_worker_waiter;

Result FsMitMWorker::AddWaitableCallback(void *arg, Handle *handles, size_t num_handles, u64 timeout) {
    (void)arg;
    svcClearEvent(handles[0]);
    g_sema_new_waitable_finish.Signal();
    return 0;
}

void FsMitMWorker::AddWaitable(IWaitable *waitable) {
    g_worker_waiter->add_waitable(waitable);
    std::scoped_lock lk{g_new_waitable_mutex};
    g_new_waitable_event->signal_event();
    g_sema_new_waitable_finish.Wait();
}

void FsMitMWorker::Main(void *arg) {
    /* Initialize waitable event. */
    g_new_waitable_event = new SystemEvent(NULL, &FsMitMWorker::AddWaitableCallback);

    /* Make a new waitable manager. */
    g_worker_waiter = std::make_unique<WaitableManager>(U64_MAX);
    g_worker_waiter->add_waitable(g_new_waitable_event);
    
    /* Service processes. */
    g_worker_waiter->process();
}
