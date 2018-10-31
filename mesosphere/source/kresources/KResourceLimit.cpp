#include <mesosphere/kresources/KResourceLimit.hpp>

namespace mesosphere
{

KResourceLimit KResourceLimit::defaultInstance{};

size_t KResourceLimit::GetCurrentValue(KResourceLimit::Category category) const
{
    // Caller should check category
    std::lock_guard guard{condvar.mutex()};
    return currentValues[(uint)category];
}

size_t KResourceLimit::GetLimitValue(KResourceLimit::Category category) const
{
    // Caller should check category
    std::lock_guard guard{condvar.mutex()};
    return limitValues[(uint)category];
}

size_t KResourceLimit::GetRemainingValue(KResourceLimit::Category category) const
{
    // Caller should check category
    std::lock_guard guard{condvar.mutex()};
    return limitValues[(uint)category] - currentValues[(uint)category];
}

bool KResourceLimit::SetLimitValue(KResourceLimit::Category category, size_t value)
{
    std::lock_guard guard{condvar.mutex()};
    if ((long)value < 0 || currentValues[(uint)category] > value) {
        return false;
    } else {
        limitValues[(uint)category] = value;
        return true;
    }
}

void KResourceLimit::Release(KResourceLimit::Category category, size_t count, size_t realCount)
{
    // Caller should ensure parameters are correct
    std::lock_guard guard{condvar.mutex()};
    currentValues[(uint)category] -= count;
    realValues[(uint)category] -= realCount;
    condvar.notify_all();
}

bool KResourceLimit::ReserveDetail(KResourceLimit::Category category, size_t count, const KSystemClock::time_point &timeoutTime)
{
    std::lock_guard guard{condvar.mutex()};
    if ((long)count <= 0 || realValues[(uint)category] >= limitValues[(uint)category]) {
        return false;
    }

    size_t newCur = currentValues[(uint)category] + count;
    bool ok = false;

    auto condition =
    [=, &newCur] {
            newCur = this->currentValues[(uint)category] + count;
            size_t lval = this->limitValues[(uint)category];
            return this->realValues[(uint)category] <= lval && newCur <= lval; // need to check here
    };

    if (timeoutTime <= KSystemClock::never) {
        // TODO, check is actually < 0
        // TODO timeout
        ok = true;
        condvar.wait(condition);
    } else {
        ok = condvar.wait_until(timeoutTime, condition);
    }

    if (ok) {
        currentValues[(uint)category] += count;
        realValues[(uint)category] += count;
    }

    return ok;
}

}
