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
#include <mesosphere.hpp>

namespace ams::kern {

    namespace {

        constexpr s64 DefaultTimeout = ams::svc::Tick(TimeSpan::FromSeconds(10));

    }

    void KResourceLimit::Initialize() {
        m_waiter_count = 0;
        std::memset(m_limit_values,   0, sizeof(m_limit_values));
        std::memset(m_current_values, 0, sizeof(m_current_values));
        std::memset(m_current_hints,  0, sizeof(m_current_hints));
        std::memset(m_peak_values,    0, sizeof(m_peak_values));
    }

    void KResourceLimit::Finalize() {
        /* ... */
    }

    s64 KResourceLimit::GetLimitValue(ams::svc::LimitableResource which) const {
        MESOSPHERE_ASSERT_THIS();

        s64 value;
        {
            KScopedLightLock lk(m_lock);
            value = m_limit_values[which];
            MESOSPHERE_ASSERT(value >= 0);
            MESOSPHERE_ASSERT(m_current_values[which] <= m_peak_values[which]);
            MESOSPHERE_ASSERT(m_peak_values[which] <= m_limit_values[which]);
            MESOSPHERE_ASSERT(m_current_hints[which]  <= m_current_values[which]);
        }

        return value;
    }

    s64 KResourceLimit::GetCurrentValue(ams::svc::LimitableResource which) const {
        MESOSPHERE_ASSERT_THIS();

        s64 value;
        {
            KScopedLightLock lk(m_lock);
            value = m_current_values[which];
            MESOSPHERE_ASSERT(value >= 0);
            MESOSPHERE_ASSERT(m_current_values[which] <= m_peak_values[which]);
            MESOSPHERE_ASSERT(m_peak_values[which] <= m_limit_values[which]);
            MESOSPHERE_ASSERT(m_current_hints[which]  <= m_current_values[which]);
        }

        return value;
    }

    s64 KResourceLimit::GetPeakValue(ams::svc::LimitableResource which) const {
        MESOSPHERE_ASSERT_THIS();

        s64 value;
        {
            KScopedLightLock lk(m_lock);
            value = m_peak_values[which];
            MESOSPHERE_ASSERT(value >= 0);
            MESOSPHERE_ASSERT(m_current_values[which] <= m_peak_values[which]);
            MESOSPHERE_ASSERT(m_peak_values[which] <= m_limit_values[which]);
            MESOSPHERE_ASSERT(m_current_hints[which]  <= m_current_values[which]);
        }

        return value;
    }

    s64 KResourceLimit::GetFreeValue(ams::svc::LimitableResource which) const {
        MESOSPHERE_ASSERT_THIS();

        s64 value;
        {
            KScopedLightLock lk(m_lock);
            MESOSPHERE_ASSERT(m_current_values[which] >= 0);
            MESOSPHERE_ASSERT(m_current_values[which] <= m_peak_values[which]);
            MESOSPHERE_ASSERT(m_peak_values[which] <= m_limit_values[which]);
            MESOSPHERE_ASSERT(m_current_hints[which]  <= m_current_values[which]);
            value = m_limit_values[which] - m_current_values[which];
        }

        return value;
    }

    Result KResourceLimit::SetLimitValue(ams::svc::LimitableResource which, s64 value) {
        MESOSPHERE_ASSERT_THIS();

        KScopedLightLock lk(m_lock);
        R_UNLESS(m_current_values[which] <= value, svc::ResultInvalidState());

        m_limit_values[which] = value;
        m_peak_values[which]  = m_current_values[which];

        return ResultSuccess();
    }

    void KResourceLimit::Add(ams::svc::LimitableResource which, s64 value) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KTargetSystem::IsDynamicResourceLimitsEnabled());

        KScopedLightLock lk(m_lock);

        /* Check that this is a true increase. */
        MESOSPHERE_ABORT_UNLESS(value > 0);

        /* Check that we can perform an increase. */
        MESOSPHERE_ABORT_UNLESS(m_current_values[which] <= m_peak_values[which]);
        MESOSPHERE_ABORT_UNLESS(m_peak_values[which] <= m_limit_values[which]);
        MESOSPHERE_ABORT_UNLESS(m_current_hints[which] <= m_current_values[which]);

        /* Check that the increase doesn't cause an overflow. */
        const auto increased_limit   = m_limit_values[which] + value;
        const auto increased_current = m_current_values[which] + value;
        const auto increased_hint    = m_current_hints[which] + value;
        MESOSPHERE_ABORT_UNLESS(m_limit_values[which] < increased_limit);
        MESOSPHERE_ABORT_UNLESS(m_current_values[which] < increased_current);
        MESOSPHERE_ABORT_UNLESS(m_current_hints[which] < increased_hint);

        /* Add the value. */
        m_limit_values[which]   = increased_limit;
        m_current_values[which] = increased_current;
        m_current_hints[which]  = increased_hint;

        /* Update our peak. */
        m_peak_values[which] = std::max(m_peak_values[which], increased_current);
    }

    bool KResourceLimit::Reserve(ams::svc::LimitableResource which, s64 value) {
        return this->Reserve(which, value, KHardwareTimer::GetTick() + DefaultTimeout);
    }

    bool KResourceLimit::Reserve(ams::svc::LimitableResource which, s64 value, s64 timeout) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(value >= 0);

        KScopedLightLock lk(m_lock);

        MESOSPHERE_ASSERT(m_current_hints[which] <= m_current_values[which]);
        if (m_current_hints[which] >= m_limit_values[which]) {
            return false;
        }

        /* Loop until we reserve or run out of time. */
        while (true) {
            MESOSPHERE_ASSERT(m_current_values[which] <= m_limit_values[which]);
            MESOSPHERE_ASSERT(m_current_hints[which]  <= m_current_values[which]);

            /* If we would overflow, don't allow to succeed. */
            if (m_current_values[which] + value <= m_current_values[which]) {
                break;
            }

            if (m_current_values[which] + value <= m_limit_values[which]) {
                m_current_values[which] += value;
                m_current_hints[which]  += value;
                m_peak_values[which]    = std::max(m_peak_values[which], m_current_values[which]);
                return true;
            }

            if (m_current_hints[which] + value <= m_limit_values[which] && (timeout < 0 || KHardwareTimer::GetTick() < timeout)) {
                m_waiter_count++;
                m_cond_var.Wait(std::addressof(m_lock), timeout, false);
                m_waiter_count--;

                if (GetCurrentThread().IsTerminationRequested()) {
                    return false;
                }
            } else {
                break;
            }
        }

        return false;
    }

    void KResourceLimit::Release(ams::svc::LimitableResource which, s64 value) {
        this->Release(which, value, value);
    }

    void KResourceLimit::Release(ams::svc::LimitableResource which, s64 value, s64 hint) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(value >= 0);
        MESOSPHERE_ASSERT(hint  >= 0);

        KScopedLightLock lk(m_lock);
        MESOSPHERE_ASSERT(m_current_values[which] <= m_limit_values[which]);
        MESOSPHERE_ASSERT(m_current_hints[which]  <= m_current_values[which]);
        MESOSPHERE_ASSERT(value <= m_current_values[which]);
        MESOSPHERE_ASSERT(hint  <= m_current_hints[which]);

        m_current_values[which] -= value;
        m_current_hints[which] -= hint;

        if (m_waiter_count != 0) {
            m_cond_var.Broadcast();
        }
    }

}
