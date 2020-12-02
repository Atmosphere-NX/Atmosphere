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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_light_lock.hpp>
#include <mesosphere/kern_k_light_condition_variable.hpp>

namespace ams::kern {

    class KResourceLimit final : public KAutoObjectWithSlabHeapAndContainer<KResourceLimit, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KResourceLimit, KAutoObject);
        private:
            s64 limit_values[ams::svc::LimitableResource_Count];
            s64 current_values[ams::svc::LimitableResource_Count];
            s64 current_hints[ams::svc::LimitableResource_Count];
            s64 peak_values[ams::svc::LimitableResource_Count];
            mutable KLightLock lock;
            s32 waiter_count;
            KLightConditionVariable cond_var;
        public:
            constexpr ALWAYS_INLINE KResourceLimit() : limit_values(), current_values(), current_hints(), peak_values(), lock(), waiter_count(), cond_var() { /* ... */ }
            virtual ~KResourceLimit() { /* ... */ }

            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            void Initialize();
            virtual void Finalize() override;

            s64 GetLimitValue(ams::svc::LimitableResource which) const;
            s64 GetCurrentValue(ams::svc::LimitableResource which) const;
            s64 GetPeakValue(ams::svc::LimitableResource which) const;
            s64 GetFreeValue(ams::svc::LimitableResource which) const;

            Result SetLimitValue(ams::svc::LimitableResource which, s64 value);

            bool Reserve(ams::svc::LimitableResource which, s64 value);
            bool Reserve(ams::svc::LimitableResource which, s64 value, s64 timeout);
            void Release(ams::svc::LimitableResource which, s64 value);
            void Release(ams::svc::LimitableResource which, s64 value, s64 hint);
    };

}
