#include <switch.h>
#include <stratosphere.hpp>
#include "fsmitm_worker.hpp"

static SystemEvent *g_new_waitable_event = NULL;
static ChildWaitableHolder *g_child_holder = NULL;

static HosMutex g_new_waitable_mutex;
static IWaitable *g_new_waitable = NULL;

Result FsMitmWorker::AddWaitableInternal(Handle *handles, size_t num_handles, u64 timeout) {
    svcClearEvent(handles[0]);
    g_child_holder->add_child(g_new_waitable);
    return 0;
}

void FsMitmWorker::AddWaitable(IWaitable *waitable) {
    g_new_waitable_mutex.Lock();
    g_new_waitable = waitable;
    g_new_waitable_event->signal_event();
    g_new_waitable_mutex.Unlock();
}

void FsMitmWorker::Main(void *arg) {
    /* Initialize waitable event. */
    g_new_waitable_event = new SystemEvent(&FsMitmWorker::AddWaitableInternal);
    g_child_holder = new ChildWaitableHolder();

    /* Make a new waitable manager. */
    WaitableManager *worker_waiter = new WaitableManager(U64_MAX);
    worker_waiter->add_waitable(g_new_waitable_event);
    
    /* Service processes. */
    worker_waiter->process();
    
    delete worker_waiter;
}
