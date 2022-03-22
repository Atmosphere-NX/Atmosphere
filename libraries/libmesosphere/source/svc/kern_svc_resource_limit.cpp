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

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        constexpr bool IsValidLimitableResource(ams::svc::LimitableResource which) {
            return which < ams::svc::LimitableResource_Count;
        }

        Result GetResourceLimitLimitValue(int64_t *out_limit_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
            /* Validate the resource. */
            R_UNLESS(IsValidLimitableResource(which), svc::ResultInvalidEnumValue());

            /* Get the resource limit. */
            KScopedAutoObject resource_limit = GetCurrentProcess().GetHandleTable().GetObject<KResourceLimit>(resource_limit_handle);
            R_UNLESS(resource_limit.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the limit value. */
            *out_limit_value = resource_limit->GetLimitValue(which);

            R_SUCCEED();
        }

        Result GetResourceLimitCurrentValue(int64_t *out_current_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
            /* Validate the resource. */
            R_UNLESS(IsValidLimitableResource(which), svc::ResultInvalidEnumValue());

            /* Get the resource limit. */
            KScopedAutoObject resource_limit = GetCurrentProcess().GetHandleTable().GetObject<KResourceLimit>(resource_limit_handle);
            R_UNLESS(resource_limit.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the current value. */
            *out_current_value = resource_limit->GetCurrentValue(which);

            R_SUCCEED();
        }

        Result GetResourceLimitPeakValue(int64_t *out_peak_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
            /* Validate the resource. */
            R_UNLESS(IsValidLimitableResource(which), svc::ResultInvalidEnumValue());

            /* Get the resource limit. */
            KScopedAutoObject resource_limit = GetCurrentProcess().GetHandleTable().GetObject<KResourceLimit>(resource_limit_handle);
            R_UNLESS(resource_limit.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the peak value. */
            *out_peak_value = resource_limit->GetPeakValue(which);

            R_SUCCEED();
        }

        Result CreateResourceLimit(ams::svc::Handle *out_handle) {
            /* Create a new resource limit. */
            KResourceLimit *resource_limit = KResourceLimit::Create();
            R_UNLESS(resource_limit != nullptr, svc::ResultOutOfResource());

            /* Ensure we don't leak a reference to the limit. */
            ON_SCOPE_EXIT { resource_limit->Close(); };

            /* Initialize the resource limit. */
            resource_limit->Initialize();

            /* Register the limit. */
            KResourceLimit::Register(resource_limit);

            /* Add the limit to the handle table. */
            R_TRY(GetCurrentProcess().GetHandleTable().Add(out_handle, resource_limit));

            R_SUCCEED();
        }

        Result SetResourceLimitLimitValue(ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which, int64_t limit_value) {
            /* Validate the resource. */
            R_UNLESS(IsValidLimitableResource(which), svc::ResultInvalidEnumValue());

            /* Get the resource limit. */
            KScopedAutoObject resource_limit = GetCurrentProcess().GetHandleTable().GetObject<KResourceLimit>(resource_limit_handle);
            R_UNLESS(resource_limit.IsNotNull(), svc::ResultInvalidHandle());

            /* Set the limit value. */
            R_TRY(resource_limit->SetLimitValue(which, limit_value));

            R_SUCCEED();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result GetResourceLimitLimitValue64(int64_t *out_limit_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
        R_RETURN(GetResourceLimitLimitValue(out_limit_value, resource_limit_handle, which));
    }

    Result GetResourceLimitCurrentValue64(int64_t *out_current_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
        R_RETURN(GetResourceLimitCurrentValue(out_current_value, resource_limit_handle, which));
    }

    Result GetResourceLimitPeakValue64(int64_t *out_peak_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
        R_RETURN(GetResourceLimitPeakValue(out_peak_value, resource_limit_handle, which));
    }

    Result CreateResourceLimit64(ams::svc::Handle *out_handle) {
        R_RETURN(CreateResourceLimit(out_handle));
    }

    Result SetResourceLimitLimitValue64(ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which, int64_t limit_value) {
        R_RETURN(SetResourceLimitLimitValue(resource_limit_handle, which, limit_value));
    }

    /* ============================= 64From32 ABI ============================= */

    Result GetResourceLimitLimitValue64From32(int64_t *out_limit_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
        R_RETURN(GetResourceLimitLimitValue(out_limit_value, resource_limit_handle, which));
    }

    Result GetResourceLimitCurrentValue64From32(int64_t *out_current_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
        R_RETURN(GetResourceLimitCurrentValue(out_current_value, resource_limit_handle, which));
    }

    Result GetResourceLimitPeakValue64From32(int64_t *out_peak_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
        R_RETURN(GetResourceLimitPeakValue(out_peak_value, resource_limit_handle, which));
    }

    Result CreateResourceLimit64From32(ams::svc::Handle *out_handle) {
        R_RETURN(CreateResourceLimit(out_handle));
    }

    Result SetResourceLimitLimitValue64From32(ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which, int64_t limit_value) {
        R_RETURN(SetResourceLimitLimitValue(resource_limit_handle, which, limit_value));
    }

}
