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
#include <stratosphere.hpp>
#include "diag_dump_stack_trace.hpp"

#if defined(ATMOSPHERE_OS_WINDOWS)
#include <stratosphere/windows.hpp>
#endif

namespace ams::diag::impl {

    #if defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_DEBUGGING)
    namespace {

        constexpr const char *ToString(AbortReason reason) {
            switch (reason) {
                case AbortReason_Audit:
                    return "Auditing Assertion Failure";
                case AbortReason_Assert:
                    return "Assertion Failure";
                case AbortReason_UnexpectedDefault:
                    return "Unexpected Default";
                case AbortReason_Abort:
                default:
                    return "Abort";
            }
        }

        void DefaultPrinter(const AbortInfo &info) {
            /* Get the thread name. */
            const char *thread_name;
            if (auto *cur_thread = os::GetCurrentThread(); cur_thread != nullptr) {
                thread_name = os::GetThreadNamePointer(cur_thread);
            } else {
                thread_name = "unknown";
            }

            #if defined(ATMOSPHERE_OS_HORIZON)
            {
                u64 process_id = 0;
                u64 thread_id  = 0;
                svc::GetProcessId(std::addressof(process_id), svc::PseudoHandle::CurrentProcess);
                svc::GetThreadId(std::addressof(thread_id), svc::PseudoHandle::CurrentThread);
                AMS_SDK_LOG("%s: '%s' in %s, process=0x%02" PRIX64 ", thread=%" PRIu64 " (%s)\n%s:%d\n", ToString(info.reason), info.expr, info.func, process_id, thread_id, thread_name, info.file, info.line);
            }
            #elif defined(ATMOSPHERE_OS_WINDOWS)
            {
                DWORD process_id = ::GetCurrentProcessId();
                DWORD thread_id  = ::GetCurrentThreadId();
                AMS_SDK_LOG("%s: '%s' in %s, process=0x%" PRIX64 ", thread=%" PRIu64 " (%s)\n%s:%d\n", ToString(info.reason), info.expr, info.func, static_cast<u64>(process_id), static_cast<u64>(thread_id), thread_name, info.file, info.line);
            }
            #else
            {
                AMS_SDK_LOG("%s: '%s' in %s, thread=%s\n%s:%d\n", ToString(info.reason), info.expr, info.func, thread_name, info.file, info.line);
            }
            #endif

            AMS_SDK_VLOG(info.message->fmt, *(info.message->vl));
            AMS_SDK_LOG("\n");

            TentativeDumpStackTrace();
        }

    }
    #endif

    void DefaultAbortObserver(const AbortInfo &info) {
        #if defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING)
        DefaultPrinter(info);
        #else
        AMS_UNUSED(info);
        #endif
    }

}
