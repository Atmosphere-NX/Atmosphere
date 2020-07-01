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
#include <stratosphere.hpp>

namespace ams::fs {

    namespace {

        constexpr bool IsValidPriority(fs::Priority priority) {
            return priority == Priority_Low || priority == Priority_Normal || priority == Priority_Realtime;
        }

        constexpr bool IsValidPriorityRaw(fs::PriorityRaw priority_raw) {
            return priority_raw == PriorityRaw_Background || priority_raw == PriorityRaw_Low || priority_raw == PriorityRaw_Normal || priority_raw == PriorityRaw_Realtime;
        }

        fs::PriorityRaw ConvertPriorityToPriorityRaw(fs::Priority priority) {
            AMS_ASSERT(IsValidPriority(priority));

            switch (priority) {
                case Priority_Low:      return PriorityRaw_Low;
                case Priority_Normal:   return PriorityRaw_Normal;
                case Priority_Realtime: return PriorityRaw_Realtime;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        fs::Priority ConvertPriorityRawToPriority(fs::PriorityRaw priority_raw) {
            AMS_ASSERT(IsValidPriorityRaw(priority_raw));

            switch (priority_raw) {
                case PriorityRaw_Background: return Priority_Low;
                case PriorityRaw_Low:        return Priority_Low;
                case PriorityRaw_Normal:     return Priority_Normal;
                case PriorityRaw_Realtime:   return Priority_Realtime;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        void UpdateTlsIoPriority(os::ThreadType *thread, u8 tls_io) {
            sf::SetFsInlineContext(thread, (tls_io & impl::TlsIoPriorityMask) | (sf::GetFsInlineContext(thread) & ~impl::TlsIoPriorityMask));
        }

        Result GetPriorityRawImpl(fs::PriorityRaw *out, os::ThreadType *thread) {
            /* Validate arguments. */
            R_UNLESS(thread != nullptr, fs::ResultNullptrArgument());

            /* Get the raw priority. */
            PriorityRaw priority_raw;
            R_TRY(impl::ConvertTlsIoPriorityToFsPriority(std::addressof(priority_raw), impl::GetTlsIoPriority(thread)));

            /* Set output. */
            *out = priority_raw;
            return ResultSuccess();
        }

        Result GetPriorityImpl(fs::Priority *out, os::ThreadType *thread) {
            /* Validate arguments. */
            R_UNLESS(thread != nullptr, fs::ResultNullptrArgument());

            /* Get the raw priority. */
            PriorityRaw priority_raw;
            R_TRY(impl::ConvertTlsIoPriorityToFsPriority(std::addressof(priority_raw), impl::GetTlsIoPriority(thread)));

            /* Set output. */
            *out = ConvertPriorityRawToPriority(priority_raw);
            return ResultSuccess();
        }

        Result SetPriorityRawImpl(os::ThreadType *thread, fs::PriorityRaw priority_raw) {
            /* Validate arguments. */
            R_UNLESS(thread != nullptr,                fs::ResultNullptrArgument());
            R_UNLESS(IsValidPriorityRaw(priority_raw), fs::ResultInvalidArgument());

            /* Convert to tls io. */
            u8 tls_io;
            R_TRY(impl::ConvertFsPriorityToTlsIoPriority(std::addressof(tls_io), priority_raw));

            /* Update the priority. */
            UpdateTlsIoPriority(thread, tls_io);
            return ResultSuccess();
        }

        Result SetPriorityImpl(os::ThreadType *thread, fs::Priority priority) {
            /* Validate arguments. */
            R_UNLESS(thread != nullptr,                fs::ResultNullptrArgument());
            R_UNLESS(IsValidPriority(priority), fs::ResultInvalidArgument());

            /* Convert to tls io. */
            u8 tls_io;
            R_TRY(impl::ConvertFsPriorityToTlsIoPriority(std::addressof(tls_io), ConvertPriorityToPriorityRaw(priority)));

            /* Update the priority. */
            UpdateTlsIoPriority(thread, tls_io);
            return ResultSuccess();
        }

    }

    Priority GetPriorityOnCurrentThread() {
        fs::Priority priority;
        AMS_FS_R_ABORT_UNLESS(AMS_FS_IMPL_ACCESS_LOG(GetPriorityImpl(std::addressof(priority), os::GetCurrentThread()), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_NONE));
        return priority;
    }

    Priority GetPriority(os::ThreadType *thread) {
        fs::Priority priority;
        AMS_FS_R_ABORT_UNLESS(AMS_FS_IMPL_ACCESS_LOG(GetPriorityImpl(std::addressof(priority), thread), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_THREAD_ID, thread != nullptr ? os::GetThreadId(thread) : static_cast<os::ThreadId>(0)));
        return priority;
    }

    PriorityRaw GetPriorityRawOnCurrentThread() {
        fs::PriorityRaw priority_raw;
        AMS_FS_R_ABORT_UNLESS(AMS_FS_IMPL_ACCESS_LOG(GetPriorityRawImpl(std::addressof(priority_raw), os::GetCurrentThread()), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_NONE));
        return priority_raw;
    }

    PriorityRaw GetPriorityRawOnCurrentThreadInternal() {
        fs::PriorityRaw priority_raw;
        R_ABORT_UNLESS(GetPriorityRawImpl(std::addressof(priority_raw), os::GetCurrentThread()));
        return priority_raw;

    }

    PriorityRaw GetPriorityRaw(os::ThreadType *thread) {
        fs::PriorityRaw priority_raw;
        AMS_FS_R_ABORT_UNLESS(AMS_FS_IMPL_ACCESS_LOG(GetPriorityRawImpl(std::addressof(priority_raw), thread), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_THREAD_ID, thread != nullptr ? os::GetThreadId(thread) : static_cast<os::ThreadId>(0)));
        return priority_raw;
    }

    void SetPriorityOnCurrentThread(Priority priority) {
        AMS_FS_R_ABORT_UNLESS(AMS_FS_IMPL_ACCESS_LOG(SetPriorityImpl(os::GetCurrentThread(), priority), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_NONE));
    }

    void SetPriority(os::ThreadType *thread, Priority priority) {
        AMS_FS_R_ABORT_UNLESS(AMS_FS_IMPL_ACCESS_LOG(SetPriorityImpl(os::GetCurrentThread(), priority), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_THREAD_ID, thread != nullptr ? os::GetThreadId(thread) : static_cast<os::ThreadId>(0)));
    }

    void SetPriorityRawOnCurrentThread(PriorityRaw priority_raw) {
        AMS_FS_R_ABORT_UNLESS(AMS_FS_IMPL_ACCESS_LOG(SetPriorityRawImpl(os::GetCurrentThread(), priority_raw), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_NONE));
    }

    void SetPriorityRaw(os::ThreadType *thread, PriorityRaw priority_raw) {
        AMS_FS_R_ABORT_UNLESS(AMS_FS_IMPL_ACCESS_LOG(SetPriorityRawImpl(os::GetCurrentThread(), priority_raw), nullptr, AMS_FS_IMPL_ACCESS_LOG_FORMAT_THREAD_ID, thread != nullptr ? os::GetThreadId(thread) : static_cast<os::ThreadId>(0)));
    }

}
