#pragma once

#include <mesosphere/threading/KScheduler.hpp>

namespace mesosphere
{

class KScopedCriticalSection final {
    private:
    std::scoped_lock<KCriticalSection> lk{KScheduler::GetCriticalSection()};
};

}
