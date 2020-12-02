/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
        /* This should be unnecessary for us, because our constructor will clear all fields. */
        /* The following is analagous to what Nintendo's implementation (no constexpr constructor) would do, though. */
        /*
            this->waiter_count = 0;
            for (size_t i = 0; i < util::size(this->limit_values); i++) {
                this->limit_values[i]    = 0;
                this->current_values[i]  = 0;
                this->current_hints[i]   = 0;
                this->peak_values[i]     = 0;
            }
        */
    }

    void KResourceLimit::Finalize() {
        /* ... */
    }

    s64 KResourceLimit::GetLimitValue(ams::svc::LimitableResource which) const {
        MESOSPHERE_ASSERT_THIS();

        s64 value;
        {
            KScopedLightLock lk(this->lock);
            value = this->limit_values[which];
            MESOSPHERE_ASSERT(value >= 0);
            MESOSPHERE_ASSERT(this->current_values[which] <= this->limit_values[which]);
            MESOSPHERE_ASSERT(this->current_hints[which]  <= this->current_values[which]);
        }

        return value;
    }

    s64 KResourceLimit::GetCurrentValue(ams::svc::LimitableResource which) const {
        MESOSPHERE_ASSERT_THIS();

        s64 value;
        {
            KScopedLightLock lk(this->lock);
            value = this->current_values[which];
            MESOSPHERE_ASSERT(value >= 0);
            MESOSPHERE_ASSERT(this->current_values[which] <= this->limit_values[which]);
            MESOSPHERE_ASSERT(this->current_hints[which]  <= this->current_values[which]);
        }

        return value;
    }

    s64 KResourceLimit::GetPeakValue(ams::svc::LimitableResource which) const {
        MESOSPHERE_ASSERT_THIS();

        s64 value;
        {
            KScopedLightLock lk(this->lock);
            value = this->peak_values[which];
            MESOSPHERE_ASSERT(value >= 0);
            MESOSPHERE_ASSERT(this->current_values[which] <= this->limit_values[which]);
            MESOSPHERE_ASSERT(this->current_hints[which]  <= this->current_values[which]);
        }

        return value;
    }

    s64 KResourceLimit::GetFreeValue(ams::svc::LimitableResource which) const {
        MESOSPHERE_ASSERT_THIS();

        s64 value;
        {
            KScopedLightLock lk(this->lock);
            MESOSPHERE_ASSERT(this->current_values[which] >= 0);
            MESOSPHERE_ASSERT(this->current_values[which] <= this->limit_values[which]);
            MESOSPHERE_ASSERT(this->current_hints[which]  <= this->current_values[which]);
            value = this->limit_values[which] - this->current_values[which];
        }

        return value;
    }

    Result KResourceLimit::SetLimitValue(ams::svc::LimitableResource which, s64 value) {
        MESOSPHERE_ASSERT_THIS();

        KScopedLightLock lk(this->lock);
        R_UNLESS(this->current_values[which] <= value, svc::ResultInvalidState());

        this->limit_values[which] = value;

        return ResultSuccess();
    }

    bool KResourceLimit::Reserve(ams::svc::LimitableResource which, s64 value) {
        return this->Reserve(which, value, KHardwareTimer::GetTick() + DefaultTimeout);
    }

    bool KResourceLimit::Reserve(ams::svc::LimitableResource which, s64 value, s64 timeout) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(value >= 0);

        KScopedLightLock lk(this->lock);

        MESOSPHERE_ASSERT(this->current_hints[which] <= this->current_values[which]);
        if (this->current_hints[which] >= this->limit_values[which]) {
            return false;
        }

        /* Loop until we reserve or run out of time. */
        while (true) {
            MESOSPHERE_ASSERT(this->current_values[which] <= this->limit_values[which]);
            MESOSPHERE_ASSERT(this->current_hints[which]  <= this->current_values[which]);

            /* If we would overflow, don't allow to succeed. */
            if (this->current_values[which] + value <= this->current_values[which]) {
                break;
            }

            if (this->current_values[which] + value <= this->limit_values[which]) {
                this->current_values[which] += value;
                this->current_hints[which]  += value;
                this->peak_values[which]    = std::max(this->peak_values[which], this->current_values[which]);
                return true;
            }

            if (this->current_hints[which] + value <= this->limit_values[which] && (timeout < 0 || KHardwareTimer::GetTick() < timeout)) {
                this->waiter_count++;
                this->cond_var.Wait(&this->lock, timeout);
                this->waiter_count--;
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

        KScopedLightLock lk(this->lock);
        MESOSPHERE_ASSERT(this->current_values[which] <= this->limit_values[which]);
        MESOSPHERE_ASSERT(this->current_hints[which]  <= this->current_values[which]);
        MESOSPHERE_ASSERT(value <= this->current_values[which]);
        MESOSPHERE_ASSERT(hint  <= this->current_hints[which]);

        this->current_values[which] -= value;
        this->current_hints[which] -= hint;

        if (this->waiter_count != 0) {
            this->cond_var.Broadcast();
        }
    }

}
