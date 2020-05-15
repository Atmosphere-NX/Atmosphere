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
        /* Try to lock the SE. */
        if (!TryLockSecurityEngine()) {
            return SmcResult::Busy;
        }
        ON_SCOPE_EXIT { UnlockSecurityEngine(); };

        return impl(args);
    }

    SmcResult LockSecurityEngineAndInvokeAsync(SmcArguments &args, SmcHandler impl, GetResultHandler result_handler) {
        SmcResult result = SmcResult::Busy;

        /* Try to lock the security engine. */
        if (TryLockSecurityEngine()) {
            /* Try to start an async operation. */
            if (const u64 async_key = BeginAsyncOperation(result_handler); async_key != InvalidAsyncKey) {
                /* Invoke the operation. */
                result = impl(args);

                /* If the operation was successful, return the key. */
                if (result == SmcResult::Success) {
                    args.r[1] = async_key;
                    return SmcResult::Success;
                }

                /* Otherwise, cancel the async operation. */
                CancelAsyncOperation(async_key);
            }

            /* We failed to invoke the async op, so unlock the security engine. */
            UnlockSecurityEngine();
        }

        return result;
    }

}
