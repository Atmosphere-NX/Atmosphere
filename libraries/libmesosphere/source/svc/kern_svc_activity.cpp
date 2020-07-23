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

        constexpr bool IsValidThreadActivity(ams::svc::ThreadActivity thread_activity) {
            switch (thread_activity) {
                case ams::svc::ThreadActivity_Runnable:
                case ams::svc::ThreadActivity_Paused:
                    return true;
                default:
                    return false;
            }
        }

        constexpr bool IsValidProcessActivity(ams::svc::ProcessActivity process_activity) {
            switch (process_activity) {
                case ams::svc::ProcessActivity_Runnable:
                case ams::svc::ProcessActivity_Paused:
                    return true;
                default:
                    return false;
            }
        }

        Result SetThreadActivity(ams::svc::Handle thread_handle, ams::svc::ThreadActivity thread_activity) {
            /* Validate the activity. */
            R_UNLESS(IsValidThreadActivity(thread_activity), svc::ResultInvalidEnumValue());

            /* Get the thread from its handle. */
            KScopedAutoObject thread = GetCurrentProcess().GetHandleTable().GetObject<KThread>(thread_handle);
            R_UNLESS(thread.IsNotNull(), svc::ResultInvalidHandle());

            /* Check that the activity is being set on a non-current thread for the current process. */
            R_UNLESS(thread->GetOwnerProcess() == GetCurrentProcessPointer(), svc::ResultInvalidHandle());
            R_UNLESS(thread.GetPointerUnsafe() != GetCurrentThreadPointer(),  svc::ResultBusy());

            /* Set the activity. */
            R_TRY(thread->SetActivity(thread_activity));

            return ResultSuccess();
        }

        Result SetProcessActivity(ams::svc::Handle process_handle, ams::svc::ProcessActivity process_activity) {
            /* Validate the activity. */
            R_UNLESS(IsValidProcessActivity(process_activity), svc::ResultInvalidEnumValue());

            /* Get the process from its handle. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Check that the activity isn't being set on the current process. */
            R_UNLESS(process.GetPointerUnsafe() != GetCurrentProcessPointer(), svc::ResultBusy());

            /* Set the activity. */
            R_TRY(process->SetActivity(process_activity));

            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result SetThreadActivity64(ams::svc::Handle thread_handle, ams::svc::ThreadActivity thread_activity) {
        return SetThreadActivity(thread_handle, thread_activity);
    }

    Result SetProcessActivity64(ams::svc::Handle process_handle, ams::svc::ProcessActivity process_activity) {
        return SetProcessActivity(process_handle, process_activity);
    }

    /* ============================= 64From32 ABI ============================= */

    Result SetThreadActivity64From32(ams::svc::Handle thread_handle, ams::svc::ThreadActivity thread_activity) {
        return SetThreadActivity(thread_handle, thread_activity);
    }

    Result SetProcessActivity64From32(ams::svc::Handle process_handle, ams::svc::ProcessActivity process_activity) {
        return SetProcessActivity(process_handle, process_activity);
    }

}
