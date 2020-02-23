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

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {



    }

    /* =============================    64 ABI    ============================= */

    Result GetResourceLimitLimitValue64(int64_t *out_limit_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
        MESOSPHERE_PANIC("Stubbed SvcGetResourceLimitLimitValue64 was called.");
    }

    Result GetResourceLimitCurrentValue64(int64_t *out_current_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
        MESOSPHERE_PANIC("Stubbed SvcGetResourceLimitCurrentValue64 was called.");
    }

    Result CreateResourceLimit64(ams::svc::Handle *out_handle) {
        MESOSPHERE_PANIC("Stubbed SvcCreateResourceLimit64 was called.");
    }

    Result SetResourceLimitLimitValue64(ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which, int64_t limit_value) {
        MESOSPHERE_PANIC("Stubbed SvcSetResourceLimitLimitValue64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result GetResourceLimitLimitValue64From32(int64_t *out_limit_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
        MESOSPHERE_PANIC("Stubbed SvcGetResourceLimitLimitValue64From32 was called.");
    }

    Result GetResourceLimitCurrentValue64From32(int64_t *out_current_value, ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which) {
        MESOSPHERE_PANIC("Stubbed SvcGetResourceLimitCurrentValue64From32 was called.");
    }

    Result CreateResourceLimit64From32(ams::svc::Handle *out_handle) {
        MESOSPHERE_PANIC("Stubbed SvcCreateResourceLimit64From32 was called.");
    }

    Result SetResourceLimitLimitValue64From32(ams::svc::Handle resource_limit_handle, ams::svc::LimitableResource which, int64_t limit_value) {
        MESOSPHERE_PANIC("Stubbed SvcSetResourceLimitLimitValue64From32 was called.");
    }

}
