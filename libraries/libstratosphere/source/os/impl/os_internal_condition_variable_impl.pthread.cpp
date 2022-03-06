/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stratosphere.hpp>
#include "os_timeout_helper.hpp"
#include "os_thread_manager.hpp"

#if defined(AMS_OS_IMPL_USE_PTHREADS)
namespace ams::os::impl {

    namespace {

    }

    void InternalConditionVariableImpl::Initialize() {
        /* TODO: What should be done here? */
    }

    void InternalConditionVariableImpl::Finalize() {
        /* TODO: What should be done here? */
    }

    void InternalConditionVariableImpl::Signal() {
        AMS_ABORT_UNLESS(pthread_cond_signal(std::addressof(m_pthread_cond)) == 0);
    }

    void InternalConditionVariableImpl::Broadcast() {
        AMS_ABORT_UNLESS(pthread_cond_broadcast(std::addressof(m_pthread_cond)) == 0);
    }

    void InternalConditionVariableImpl::Wait(InternalCriticalSection *cs) {
        AMS_ABORT_UNLESS(pthread_cond_wait(std::addressof(m_pthread_cond), std::addressof(cs->Get()->m_pthread_mutex)) == 0);
    }

    ConditionVariableStatus InternalConditionVariableImpl::TimedWait(InternalCriticalSection *cs, const TimeoutHelper &timeout_helper) {
        struct timespec ts;
        const auto gettime_res = clock_gettime(CLOCK_REALTIME, std::addressof(ts));
        AMS_ASSERT(gettime_res == 0);
        AMS_UNUSED(gettime_res);

        constexpr s64 NanoSecondsPerSecond = TimeSpan::FromSeconds(1).GetNanoSeconds();
        const s64 ns = timeout_helper.GetTimeLeftOnTarget().GetNanoSeconds();
        ts.tv_sec  += (ns / NanoSecondsPerSecond);
        ts.tv_nsec += (ns % NanoSecondsPerSecond);
        if (ts.tv_nsec >= NanoSecondsPerSecond) {
            ts.tv_sec  += ts.tv_nsec / NanoSecondsPerSecond;
            ts.tv_nsec %= NanoSecondsPerSecond;
        }

        const auto res = pthread_cond_timedwait(std::addressof(m_pthread_cond), std::addressof(cs->Get()->m_pthread_mutex), std::addressof(ts));

        if (res != 0) {
            AMS_ABORT_UNLESS(res == ETIMEDOUT);
            return ConditionVariableStatus::TimedOut;
        }

        return ConditionVariableStatus::Success;
    }

}
#endif
