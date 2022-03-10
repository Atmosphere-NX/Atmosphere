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
#include "impl/diag_get_all_backtrace.hpp"
#include "impl/diag_invoke_abort.hpp"

namespace ams::diag {

    namespace {

        inline NORETURN void AbortWithValue(u64 debug) {
            #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
            /* Just perform a data abort. */
            register u64 addr __asm__("x27") = FatalErrorContext::StdAbortMagicAddress;
            register u64 val __asm__("x28")  = FatalErrorContext::StdAbortMagicValue;
            while (true) {
                __asm__ __volatile__ (
                    "mov x0, %[debug]\n"
                    "str %[val], [%[addr]]\n"
                    :
                    : [debug]"r"(debug), [val]"r"(val), [addr]"r"(addr)
                    : "x0"
                );
            }
            #else
            AMS_UNUSED(debug);
            std::abort();
            #endif
            __builtin_unreachable();
        }

        constinit os::SdkMutex g_assert_mutex;
        constinit os::SdkMutex g_abort_mutex;

        void PrepareAbort() {
            #if defined(ATMOSPHERE_OS_HORIZON)
            {
                /* Get the thread local region. */
                auto * const tlr = svc::GetThreadLocalRegion();

                /* Clear disable count. */
                tlr->disable_count = 0;

                /* If we need to, unpin. */
                if (tlr->interrupt_flag) {
                    svc::SynchronizePreemptionState();
                }
            }
            #endif
        }

        AbortReason ToAbortReason(AssertionType type) {
            switch (type) {
                case AssertionType_Audit:  return AbortReason_Audit;
                case AssertionType_Assert: return AbortReason_Assert;
                default:
                    return AbortReason_Abort;
            }
        }

        AssertionFailureOperation DefaultAssertionFailureHandler(const AssertionInfo &) {
            return AssertionFailureOperation_Abort;
        }

        constinit AssertionFailureHandler g_assertion_failure_handler = &DefaultAssertionFailureHandler;

        void ExecuteAssertionFailureOperation(AssertionFailureOperation operation, const AssertionInfo &info)  {
            switch (operation) {
                case AssertionFailureOperation_Continue:
                    break;
                case AssertionFailureOperation_Abort:
                    {
                        const AbortInfo abort_info = {
                            ToAbortReason(info.type),
                            info.message,
                            info.expr,
                            info.func,
                            info.file,
                            info.line,
                        };

                        ::ams::diag::impl::InvokeAbortObserver(abort_info);
                        AbortWithValue(0);
                    }
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        void InvokeAssertionFailureHandler(const AssertionInfo &info) {
            const auto operation = g_assertion_failure_handler(info);
            ExecuteAssertionFailureOperation(operation, info);
        }


    }

    NOINLINE void OnAssertionFailure(AssertionType type, const char *expr, const char *func, const char *file, int line, const char *format, ...) {
        /* Prepare to abort. */
        PrepareAbort();

        /* Acquire exclusive assert rights. */
        if (g_assert_mutex.IsLockedByCurrentThread()) {
            AbortWithValue(0);
        }

        std::scoped_lock lk(g_assert_mutex);

        /* Create the assertion info. */
        std::va_list vl;
        va_start(vl, format);

        const ::ams::diag::LogMessage message = { format, std::addressof(vl) };

        const AssertionInfo info = {
            type,
            std::addressof(message),
            expr,
            func,
            file,
            line,
        };

        InvokeAssertionFailureHandler(info);
        va_end(vl);
    }

    void OnAssertionFailure(AssertionType type, const char *expr, const char *func, const char *file, int line) {
        return OnAssertionFailure(type, expr, func, file, line, "");
    }

    NORETURN void AbortImpl(const char *expr, const char *func, const char *file, int line) {
        const Result res = ResultSuccess();

        std::va_list vl{};
        VAbortImpl(expr, func, file, line, std::addressof(res), nullptr, "", vl);
    }

    NORETURN void AbortImpl(const char *expr, const char *func, const char *file, int line, const char *fmt, ...) {
        const Result res = ResultSuccess();

        std::va_list vl;
        va_start(vl, fmt);
        VAbortImpl(expr, func, file, line, std::addressof(res), nullptr, fmt, vl);
    }

    NORETURN void AbortImpl(const char *expr, const char *func, const char *file, int line, const ::ams::Result *result, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        VAbortImpl(expr, func, file, line, result, nullptr, fmt, vl);
    }

    NORETURN void AbortImpl(const char *expr, const char *func, const char *file, int line, const ::ams::Result *result, const ::ams::os::UserExceptionInfo *exc_info, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        VAbortImpl(expr, func, file, line, result, exc_info, fmt, vl);
    }

    NORETURN NOINLINE void VAbortImpl(const char *expr, const char *func, const char *file, int line, const ::ams::Result *result, const ::ams::os::UserExceptionInfo *exc_info, const char *fmt, std::va_list vl) {
        /* Prepare to abort. */
        PrepareAbort();

        /* Acquire exclusive abort rights. */
        if (g_abort_mutex.IsLockedByCurrentThread()) {
            AbortWithValue(result->GetValue());
        }

        std::scoped_lock lk(g_abort_mutex);

        /* Set the abort impl return address. */
        impl::SetAbortImplReturnAddress(reinterpret_cast<uintptr_t>(__builtin_return_address(0)));

        /* Create abort info. */
        std::va_list cvl;
        va_copy(cvl, vl);
        const diag::LogMessage message = { fmt, std::addressof(cvl) };

        const AbortInfo abort_info = {
            AbortReason_Abort,
            std::addressof(message),
            expr,
            func,
            file,
            line,
        };
        const SdkAbortInfo sdk_abort_info = {
            abort_info,
            *result,
            exc_info
        };

        /* Invoke observers. */
        ::ams::diag::impl::InvokeAbortObserver(abort_info);
        ::ams::diag::impl::InvokeSdkAbortObserver(sdk_abort_info);

        /* Abort. */
        AbortWithValue(result->GetValue());
    }

}

namespace ams::impl {

    NORETURN NOINLINE void UnexpectedDefaultImpl(const char *func, const char *file, int line) {
        /* Create abort info. */
        std::va_list vl{};
        const ::ams::diag::LogMessage message = { "" , std::addressof(vl) };
        const ::ams::diag::AbortInfo abort_info = {
            ::ams::diag::AbortReason_UnexpectedDefault,
            std::addressof(message),
            "",
            func,
            file,
            line,
        };

        /* Invoke observers. */
        ::ams::diag::impl::InvokeAbortObserver(abort_info);

        /* Abort. */
        ::ams::diag::AbortWithValue(0);
    }

}
