#pragma once
#include <switch.h>
#include <stratosphere.hpp>

class FsMitmWorker {
    private:
        static Result AddWaitableCallback(Handle *handles, size_t num_handles, u64 timeout);
    public:
        static void Main(void *arg);
        static void AddWaitable(IWaitable *waitable);
};