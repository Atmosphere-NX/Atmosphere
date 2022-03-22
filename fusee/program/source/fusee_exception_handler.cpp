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
#include <exosphere.hpp>
#include "fusee_exception_handler.hpp"
#include "fusee_fatal.hpp"

namespace ams::nxboot {

    NORETURN void ExceptionHandlerImpl(s32 which, u32 lr, u32 svc_lr) {
        ShowFatalError("Exception: which=%" PRId32 ", lr=%p, svc_lr=%p\n", which, reinterpret_cast<void *>(lr), reinterpret_cast<void *>(svc_lr));
    }

}

namespace ams::diag {

    NORETURN void AbortImpl(const char *file, int line, const char *func, const char *expr, u64 value, const char *format, ...) {
        AMS_UNUSED(file, line, func, expr, format);

        u32 lr;
        __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
        nxboot::ShowFatalError("Abort called, lr=%p, value=%" PRIx64 "\n", reinterpret_cast<void *>(lr), value);
    }

    NORETURN void AbortImpl(const char *file, int line, const char *func, const char *expr, u64 value) {
        AMS_UNUSED(file, line, func, expr);

        u32 lr;
        __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
        nxboot::ShowFatalError("Abort called, lr=%p, value=%" PRIx64 "\n", reinterpret_cast<void *>(lr), value);
    }

    NORETURN void AbortImpl() {
        u32 lr;
        __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
        nxboot::ShowFatalError("Abort called, lr=%p\n", reinterpret_cast<void *>(lr));
    }

}

namespace ams::result::impl {

        NORETURN void OnResultAbort(const char *file, int line, const char *func, const char *expr, Result result) {
            ::ams::diag::AbortImpl(file, line, func, expr, result.GetValue(), "Result Abort: 2%03" PRId32 "-%04" PRId32 "", result.GetModule(), result.GetDescription());
            AMS_INFINITE_LOOP();
            __builtin_unreachable();
        }

        NORETURN void OnResultAbort(Result result) {
            OnResultAbort("", 0, "", "", result);
        }

}