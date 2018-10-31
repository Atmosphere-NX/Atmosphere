#pragma once

#include <mesosphere/threading/KThread.hpp>
#include <mesosphere/threading/KMutex.hpp>

namespace mesosphere
{


/// Provides an interface similar to std::condition_variable
class KConditionVariable final {
    public:

    using native_handle_type = uiptr;

    KConditionVariable() = default;
    KConditionVariable(const KConditionVariable &) = delete;
    KConditionVariable(KConditionVariable &&) = delete;
    KConditionVariable &operator=(const KConditionVariable &) = delete;
    KConditionVariable &operator=(KConditionVariable &&) = delete;

    native_handle_type native_handle() { return mutex_.native_handle(); }

    KMutex &mutex() { return mutex_; }

    void wait() noexcept
    {
        wait_until_impl(KSystemClock::never);
    }
    template<typename Predicate>
    void wait(Predicate pred)
    {
        while (!pred()) {
            wait();
        }
    }

    template<typename Clock, typename Duration>
    void wait_until(const std::chrono::time_point<Clock, Duration> &timeoutPoint) noexcept
    {
        wait_until_impl(timeoutPoint);
    }
    template<typename Clock, typename Duration, typename Predicate>
    bool wait_until(const std::chrono::time_point<Clock, Duration> &timeoutPoint, Predicate pred)
    {
        while (!pred()) {
            wait_until(timeoutPoint);
            if (Clock::now() >= timeoutPoint) {
                return pred();
            }
        }

        return true;
    }

    template<typename Rep, typename Period>
    void wait_for(const std::chrono::duration<Rep, Period>& timeout) noexcept
    {
        wait_until(KSystemClock::now() + timeout);
    }

    template<typename Rep, typename Period, typename Predicate>
    bool wait_for(const std::chrono::duration<Rep, Period>& timeout, Predicate pred)
    {
        return wait_until(KSystemClock::now() + timeout, std::move(pred));
    }

    void notify_one() noexcept;
    void notify_all() noexcept;

    private:
    void wait_until_impl(const KSystemClock::time_point &timeoutPoint) noexcept;

    KMutex mutex_{};
    KThread::WaitList waiterList{};
};

}
