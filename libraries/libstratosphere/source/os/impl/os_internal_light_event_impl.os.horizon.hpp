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

        constexpr inline s32 LightEventState_NotSignaledAndNoWaiter = 0;
        constexpr inline s32 LightEventState_NotSignaledAndWaiter   = 1;
        constexpr inline s32 LightEventState_Signaled               = 2;

        ALWAYS_INLINE uintptr_t GetAddressOfAtomicInteger(std::atomic<s32> *ptr) {
            static_assert(sizeof(*ptr) == sizeof(s32));
            static_assert(alignof(*ptr) == alignof(s32));
            static_assert(std::atomic<s32>::is_always_lock_free);

            return reinterpret_cast<uintptr_t>(ptr);
        }

        ALWAYS_INLINE bool SvcSignalToAddressForAutoClear(std::atomic<s32> *state, s32 expected) {
            /* Signal. */
            R_TRY_CATCH(svc::SignalToAddress(GetAddressOfAtomicInteger(state), svc::SignalType_SignalAndModifyByWaitingCountIfEqual, expected, 1)) {
                R_CATCH(svc::ResultInvalidState) { return false; }
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

            return true;
        }

        ALWAYS_INLINE bool SvcSignalToAddressForManualClear(std::atomic<s32> *state, s32 expected) {
            /* Signal. */
            R_TRY_CATCH(svc::SignalToAddress(GetAddressOfAtomicInteger(state), svc::SignalType_SignalAndIncrementIfEqual, expected, -1)) {
                R_CATCH(svc::ResultInvalidState) { return false; }
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

            return true;
        }

        ALWAYS_INLINE bool SvcWaitForAddressIfEqual(std::atomic<s32> *state) {
            /* Wait. */
            R_TRY_CATCH(svc::WaitForAddress(GetAddressOfAtomicInteger(state), svc::ArbitrationType_WaitIfEqual, LightEventState_NotSignaledAndWaiter, -1ll)) {
                R_CATCH(svc::ResultInvalidState) { return false; }
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

            return true;
        }

        ALWAYS_INLINE bool SvcWaitForAddressIfEqual(bool *out_timed_out, std::atomic<s32> *state, s64 timeout) {
            /* Default to not timed out. */
            *out_timed_out = false;

            /* Wait. */
            R_TRY_CATCH(svc::WaitForAddress(GetAddressOfAtomicInteger(state), svc::ArbitrationType_WaitIfEqual, LightEventState_NotSignaledAndWaiter, timeout)) {
                R_CATCH(svc::ResultInvalidState) { return false; }
                R_CATCH(svc::ResultTimedOut) { *out_timed_out = true; return false; }
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

            return true;
        }

        ALWAYS_INLINE s32 LoadAcquireExclusiveForLightEvent(s32 *p) {
            s32 v;
            __asm__ __volatile__("ldaxr %w[v], %[p]" : [v]"=&r"(v) : [p]"Q"(*p) : "memory");
            return v;
        }

        ALWAYS_INLINE bool StoreReleaseExclusiveForLightEvent(s32 *p, s32 v) {
            int result;
            __asm__ __volatile__("stlxr %w[result], %w[v], %[p]" : [result]"=&r"(result) : [v]"r"(v), [p]"Q"(*p) : "memory");
            return result == 0;
        }

        ALWAYS_INLINE void ClearExclusiveForLightEvent() {
            __asm__ __volatile__("clrex" ::: "memory");
        }

        ALWAYS_INLINE bool CompareAndSwap(std::atomic<s32> *state, s32 expected, s32 desired) {
            /* NOTE: This works around gcc not emitting clrex on ldaxr fail. */
            s32 * const state_ptr = reinterpret_cast<s32 *>(GetAddressOfAtomicInteger(state));

            while (true) {
                if (AMS_UNLIKELY(LoadAcquireExclusiveForLightEvent(state_ptr) != expected)) {
                    ClearExclusiveForLightEvent();
                    return false;
                }

                if (AMS_LIKELY(StoreReleaseExclusiveForLightEvent(state_ptr, desired))) {
                    return true;
                }
            }
        }

    }

    void InternalLightEventImpl::Initialize(bool signaled) {
        /* Set initial state. */
        m_state.store(signaled ? LightEventState_Signaled : LightEventState_NotSignaledAndNoWaiter, std::memory_order_relaxed);
    }

    void InternalLightEventImpl::Finalize() {
        /* ... */
    }

    void InternalLightEventImpl::SignalWithAutoClear() {
        /* Loop until signaled */
        while (true) {
            /* Fence memory. */
            std::atomic_thread_fence(std::memory_order_seq_cst);

            /* Get the current state. */
            const auto state = m_state.load(std::memory_order_relaxed);
            switch (state) {
                case LightEventState_NotSignaledAndNoWaiter:
                    /* Try to signal. */
                    if (CompareAndSwap(std::addressof(m_state), state, LightEventState_Signaled)) {
                        return;
                    }
                    break;
                case LightEventState_NotSignaledAndWaiter:
                    /* Try to signal. */
                    if (SvcSignalToAddressForAutoClear(std::addressof(m_state), state)) {
                        return;
                    }
                    break;
                case LightEventState_Signaled:
                    /* If we're already signaled, we're done. */
                    return;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }

    void InternalLightEventImpl::SignalWithManualClear() {
        /* Loop until signaled */
        while (true) {
            /* Fence memory. */
            std::atomic_thread_fence(std::memory_order_seq_cst);

            /* Get the current state. */
            const auto state = m_state.load(std::memory_order_relaxed);
            switch (state) {
                case LightEventState_NotSignaledAndNoWaiter:
                    /* Try to signal. */
                    if (CompareAndSwap(std::addressof(m_state), state, LightEventState_Signaled)) {
                        return;
                    }
                    break;
                case LightEventState_NotSignaledAndWaiter:
                    /* Try to signal. */
                    if (SvcSignalToAddressForManualClear(std::addressof(m_state), state)) {
                        return;
                    }
                    break;
                case LightEventState_Signaled:
                    /* If we're already signaled, we're done. */
                    return;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }

    void InternalLightEventImpl::Clear() {
        /* Fence memory. */
        std::atomic_thread_fence(std::memory_order_acquire);
        ON_SCOPE_EXIT { std::atomic_thread_fence(std::memory_order_release); };

        /* Change state from signaled to not-signaled. */
        CompareAndSwap(std::addressof(m_state), LightEventState_Signaled, LightEventState_NotSignaledAndNoWaiter);
    }

    void InternalLightEventImpl::WaitWithAutoClear() {
        /* When we're done, fence memory. */
        ON_SCOPE_EXIT { std::atomic_thread_fence(std::memory_order_seq_cst); };

        /* Loop waiting. */
        while (true) {
            /* Get the current state. */
            const auto state = m_state.load(std::memory_order_acquire);
            switch (state) {
                case LightEventState_NotSignaledAndNoWaiter:
                    /* Change state to have waiter. */
                    if (!CompareAndSwap(std::addressof(m_state), state, LightEventState_NotSignaledAndWaiter)) {
                        break;
                    }
                    [[fallthrough]];
                case LightEventState_NotSignaledAndWaiter:
                    /* Wait for the address. */
                    if (SvcWaitForAddressIfEqual(std::addressof(m_state))) {
                        return;
                    }
                    break;
                case LightEventState_Signaled:
                    /* Change state to no-waiters. */
                    if (CompareAndSwap(std::addressof(m_state), state, LightEventState_NotSignaledAndNoWaiter)) {
                        return;
                    }
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }

    void InternalLightEventImpl::WaitWithManualClear() {
        /* When we're done, fence memory. */
        ON_SCOPE_EXIT { std::atomic_thread_fence(std::memory_order_seq_cst); };

        /* Loop waiting. */
        while (true) {
            /* Get the current state. */
            const auto state = m_state.load(std::memory_order_acquire);
            switch (state) {
                case LightEventState_NotSignaledAndNoWaiter:
                    /* Change state to have waiter. */
                    if (!CompareAndSwap(std::addressof(m_state), state, LightEventState_NotSignaledAndWaiter)) {
                        break;
                    }
                    [[fallthrough]];
                case LightEventState_NotSignaledAndWaiter:
                    /* Wait for the address. */
                    SvcWaitForAddressIfEqual(std::addressof(m_state));
                    return;
                case LightEventState_Signaled:
                    /* We're signaled, so return. */
                    return;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }

    bool InternalLightEventImpl::TryWaitWithAutoClear() {
        /* When we're done, fence memory. */
        ON_SCOPE_EXIT { std::atomic_thread_fence(std::memory_order_seq_cst); };

        return CompareAndSwap(std::addressof(m_state), LightEventState_Signaled, LightEventState_NotSignaledAndNoWaiter);
    }

    bool InternalLightEventImpl::TryWaitWithManualClear() {
        /* When we're done, fence memory. */
        ON_SCOPE_EXIT { std::atomic_thread_fence(std::memory_order_seq_cst); };

        return m_state.load(std::memory_order_acquire) == LightEventState_Signaled;
    }

    bool InternalLightEventImpl::TimedWaitWithAutoClear(const TimeoutHelper &timeout_helper) {
        /* When we're done, fence memory. */
        ON_SCOPE_EXIT { std::atomic_thread_fence(std::memory_order_seq_cst); };

        /* Loop waiting. */
        while (true) {
            /* Get the current state. */
            const auto state = m_state.load(std::memory_order_acquire);
            switch (state) {
                case LightEventState_NotSignaledAndNoWaiter:
                    /* Change state to have waiter. */
                    if (!CompareAndSwap(std::addressof(m_state), state, LightEventState_NotSignaledAndWaiter)) {
                        break;
                    }
                    [[fallthrough]];
                case LightEventState_NotSignaledAndWaiter:
                    /* Wait for the address, checking timeout. */
                    {
                        bool timed_out;
                        if (SvcWaitForAddressIfEqual(std::addressof(timed_out), std::addressof(m_state), timeout_helper.GetTimeLeftOnTarget().GetNanoSeconds())) {
                            return true;
                        }
                        if (timed_out) {
                            return false;
                        }
                    }
                    break;
                case LightEventState_Signaled:
                    /* Try to clear. */
                    if (CompareAndSwap(std::addressof(m_state), LightEventState_Signaled, LightEventState_NotSignaledAndNoWaiter)) {
                        return true;
                    }
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }

    bool InternalLightEventImpl::TimedWaitWithManualClear(const TimeoutHelper &timeout_helper) {
        /* When we're done, fence memory. */
        ON_SCOPE_EXIT { std::atomic_thread_fence(std::memory_order_seq_cst); };

        /* Loop waiting. */
        while (true) {
            /* Get the current state. */
            const auto state = m_state.load(std::memory_order_acquire);
            switch (state) {
                case LightEventState_NotSignaledAndNoWaiter:
                    /* Change state to have waiter. */
                    if (!CompareAndSwap(std::addressof(m_state), state, LightEventState_NotSignaledAndWaiter)) {
                        break;
                    }
                    [[fallthrough]];
                case LightEventState_NotSignaledAndWaiter:
                    /* Wait and check for timeout. */
                    {
                        bool timed_out;
                        SvcWaitForAddressIfEqual(std::addressof(timed_out), std::addressof(m_state), timeout_helper.GetTimeLeftOnTarget().GetNanoSeconds());
                        return !timed_out;
                    }
                    break;
                case LightEventState_Signaled:
                    /* We're signaled, so return. */
                    return true;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }

}
