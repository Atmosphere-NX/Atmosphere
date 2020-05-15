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
#include <exosphere.hpp>
#include "../secmon_error.hpp"
#include "secmon_smc_se_lock.hpp"

namespace ams::secmon::smc {

    namespace {

        constinit std::atomic_bool g_is_locked = false;

    }

    bool TryLockSecurityEngine() {
        bool value = false;
        return g_is_locked.compare_exchange_strong(value, true);
    }

    void UnlockSecurityEngine() {
        g_is_locked = false;
    }

    bool IsSecurityEngineLocked() {
        return g_is_locked;
    }

    SmcResult LockSecurityEngineAndInvoke(SmcArguments &args, SmcHandler impl) {
        /* Try to lock the security engine. */
        SMC_R_UNLESS(TryLockSecurityEngine(), Busy);
        ON_SCOPE_EXIT { UnlockSecurityEngine(); };

        return impl(args);
    }

    SmcResult LockSecurityEngineAndInvokeAsync(SmcArguments &args, SmcHandler impl, GetResultHandler result_handler) {
        /* Try to lock the security engine. */
        SMC_R_UNLESS(TryLockSecurityEngine(), Busy);
        auto se_guard = SCOPE_GUARD { UnlockSecurityEngine(); };

        /* Try to start an async operation. */
        const u64 async_key = BeginAsyncOperation(result_handler);
        SMC_R_UNLESS(async_key != InvalidAsyncKey, Busy);
        auto async_guard = SCOPE_GUARD { CancelAsyncOperation(async_key); };

        /* Try to invoke the operation. */
        SMC_R_TRY(impl(args));

        /* We succeeded! Cancel our guards, and return the async key to our caller. */
        async_guard.Cancel();
        se_guard.Cancel();

        args.r[1] = async_key;
        return SmcResult::Success;
    }

}
