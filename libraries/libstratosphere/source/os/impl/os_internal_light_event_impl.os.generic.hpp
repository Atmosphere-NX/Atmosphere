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
#pragma once
#include <stratosphere.hpp>

namespace ams::os::impl {

    namespace {

        constexpr inline u8 LightEventState_NotSignaled = -1;
        constexpr inline u8 LightEventState_Invalid     = 0;
        constexpr inline u8 LightEventState_Signaled    = 1;

        static_assert(LightEventState_NotSignaled == 0xFF);

        ALWAYS_INLINE bool AtomicAutoClearLightEvent(std::atomic<u8> &signal_state) {
            u8 expected = LightEventState_Signaled;
            return signal_state.compare_exchange_strong(expected, LightEventState_NotSignaled);
        }

        ALWAYS_INLINE u32 GetBroadcastCounterUnsafe(u16 &low, u8 &high) {
            const u32 upper = high;
            return (upper << BITSIZEOF(low)) | low;
        }

        ALWAYS_INLINE void IncrementBroadcastCounterUnsafe(u16 &low, u8 &high) {
            if ((++low) == 0) {
                ++high;
            }
        }

    }

    void InternalLightEventImpl::Initialize(bool signaled) {
        /* Set broadcast counter. */
        m_counter_low  = 0;
        m_counter_high = 0;

        /* Set initial state. */
        m_signal_state = signaled ? LightEventState_Signaled : LightEventState_NotSignaled;
    }

    void InternalLightEventImpl::Finalize() {
        /* Set final state. */
        m_signal_state = LightEventState_Invalid;
    }

    void InternalLightEventImpl::SignalWithAutoClear() {
        /* Signal only if we need to. */
        if (m_signal_state.load(std::memory_order_acquire) == LightEventState_NotSignaled) {
            /* Lock ourselves. */
            std::scoped_lock lk(m_cs);

            /* Set our state to signaled. */
            m_signal_state = LightEventState_Signaled;

            /* Signal to any waiters. */
            m_cv.Signal();
        }
    }

    void InternalLightEventImpl::SignalWithManualClear() {
        /* Signal only if we need to. */
        if (m_signal_state.load(std::memory_order_acquire) == LightEventState_NotSignaled) {
            /* Lock ourselves. */
            std::scoped_lock lk(m_cs);

            /* Set our state to signaled. */
            m_signal_state = LightEventState_Signaled;

            /* Increment our broadcast counter. */
            IncrementBroadcastCounterUnsafe(m_counter_low, m_counter_high);

            /* Broadcast to any waiters. */
            m_cv.Broadcast();
        }
    }

    void InternalLightEventImpl::Clear() {
        /* Clear only if we need to. */
        if (m_signal_state.load(std::memory_order_acquire) == LightEventState_Signaled) {
            /* Lock ourselves. */
            std::scoped_lock lk(m_cs);

            /* Set our state to signaled. */
            m_signal_state = LightEventState_NotSignaled;
        }
    }

    void InternalLightEventImpl::WaitWithAutoClear() {
        /* Loop waiting. */
        while (true) {
            /* Get the current state. */
            const auto state = m_signal_state.load(std::memory_order_acquire);

            /* If we're not signaled, stop looping so we can properly wait to be signaled. */
            if (state == LightEventState_NotSignaled) {
                break;
            }

            /* If we're signaled, try to reset. */
            if (state == LightEventState_Signaled && AtomicAutoClearLightEvent(m_signal_state)) {
                return;
            }
        }

        /* Wait explicitly on the not-signaled event. */
        std::scoped_lock lk(m_cs);

        while (m_signal_state == LightEventState_NotSignaled) {
            m_cv.Wait(std::addressof(m_cs));
        }

        /* Set our state to not signaled. */
        m_signal_state = LightEventState_NotSignaled;
    }

    void InternalLightEventImpl::WaitWithManualClear() {
        /* Loop waiting. */
        while (true) {
            /* Get the current m_signal_state. */
            const auto state = m_signal_state.load(std::memory_order_acquire);

            /* If we're not signaled, stop looping so we can properly wait to be signaled. */
            if (state == LightEventState_NotSignaled) {
                break;
            }

            /* If we're signaled, we're done. */
            if (state == LightEventState_Signaled) {
                return;
            }
        }

        /* Wait explicitly on the not-signaled event. */
        std::scoped_lock lk(m_cs);

        /* Get the broadcast counter. */
        const auto bc = GetBroadcastCounterUnsafe(m_counter_low, m_counter_high);

        while (m_signal_state == LightEventState_NotSignaled) {
            /* Check if a broadcast has occurred. */
            if (bc != GetBroadcastCounterUnsafe(m_counter_low, m_counter_high)) {
                break;
            }

            m_cv.Wait(std::addressof(m_cs));
        }
    }

    bool InternalLightEventImpl::TryWaitWithAutoClear() {
        return m_signal_state.load(std::memory_order_acquire) == LightEventState_Signaled && AtomicAutoClearLightEvent(m_signal_state);
    }

    bool InternalLightEventImpl::TryWaitWithManualClear() {
        return m_signal_state.load(std::memory_order_acquire) == LightEventState_Signaled;
    }

    bool InternalLightEventImpl::TimedWaitWithAutoClear(const TimeoutHelper &timeout_helper) {
        /* Loop waiting. */
        while (true) {
            /* Get the current state. */
            const auto state = m_signal_state.load(std::memory_order_acquire);

            /* If we're not signaled, stop looping so we can properly wait to be signaled. */
            if (state == LightEventState_NotSignaled) {
                break;
            }

            /* If we're signaled, try to reset. */
            if (state == LightEventState_Signaled && AtomicAutoClearLightEvent(m_signal_state)) {
                return true;
            }
        }

        /* Wait explicitly on the not-signaled event. */
        std::scoped_lock lk(m_cs);

        while (m_signal_state == LightEventState_NotSignaled) {
            if (m_cv.TimedWait(std::addressof(m_cs), timeout_helper) == ConditionVariableStatus::TimedOut) {
                return false;
            }
        }

        /* Set our state to not signaled. */
        m_signal_state = LightEventState_NotSignaled;

        return true;
    }

    bool InternalLightEventImpl::TimedWaitWithManualClear(const TimeoutHelper &timeout_helper) {
        /* Loop waiting. */
        while (true) {
            /* Get the current m_signal_state. */
            const auto state = m_signal_state.load(std::memory_order_acquire);

            /* If we're not signaled, stop looping so we can properly wait to be signaled. */
            if (state == LightEventState_NotSignaled) {
                break;
            }

            /* If we're signaled, we're done. */
            if (state == LightEventState_Signaled) {
                return true;
            }
        }

        /* Wait explicitly on the not-signaled event. */
        std::scoped_lock lk(m_cs);

        /* Get the broadcast counter. */
        const auto bc = GetBroadcastCounterUnsafe(m_counter_low, m_counter_high);

        while (m_signal_state == LightEventState_NotSignaled) {
            /* Check if a broadcast has occurred. */
            if (bc != GetBroadcastCounterUnsafe(m_counter_low, m_counter_high)) {
                break;
            }

            if (m_cv.TimedWait(std::addressof(m_cs), timeout_helper) == ConditionVariableStatus::TimedOut) {
                return false;
            }
        }

        return true;
    }

}
