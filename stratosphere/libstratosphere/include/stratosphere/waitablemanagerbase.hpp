#pragma once
#include <switch.h>
#include <atomic>
#include <vector>

class WaitableManagerBase {
    std::atomic<u64> cur_priority = 0;
    public:
        WaitableManagerBase() = default;
        virtual ~WaitableManagerBase() = default;

        u64 get_priority() {
            return std::atomic_fetch_add(&cur_priority, (u64)1);
        }
};
