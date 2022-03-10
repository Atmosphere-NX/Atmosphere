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

    NORETURN void AbortImpl(const char *expr, const char *func, const char *file, int line) {
        AMS_UNUSED(expr, func, line, file);

        u32 lr;
        __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
        nxboot::ShowFatalError("Abort called, lr=%p\n", reinterpret_cast<void *>(lr));
    }

    NORETURN void AbortImpl(const char *expr, const char *func, const char *file, int line, const char *format, ...) {
        AMS_UNUSED(expr, func, line, file, format);

        u32 lr;
        __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
        nxboot::ShowFatalError("Abort called, lr=%p\n", reinterpret_cast<void *>(lr));
    }

    NORETURN void AbortImpl(const char *expr, const char *func, const char *file, int line, const ::ams::Result *result, const char *format, ...) {
        AMS_UNUSED(expr, func, line, file, result, format);

        u32 lr;
        __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
        nxboot::ShowFatalError("Abort called, lr=%p, result=0x%08" PRIX32 "\n", reinterpret_cast<void *>(lr), result != nullptr ? result->GetValue() : 0);
    }

    NORETURN void AbortImpl(const char *expr, const char *func, const char *file, int line, const ::ams::Result *result, const ::ams::os::UserExceptionInfo *exception_info, const char *format, ...) {
        AMS_UNUSED(expr, func, line, file, result, exception_info, format);

        u32 lr;
        __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
        nxboot::ShowFatalError("Abort called, lr=%p, result=0x%08" PRIX32 "\n", reinterpret_cast<void *>(lr), result != nullptr ? result->GetValue() : 0);
    }

    NORETURN void AbortImpl() {
        u32 lr;
        __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
        nxboot::ShowFatalError("Abort called, lr=%p\n", reinterpret_cast<void *>(lr));
    }

    NORETURN void OnAssertionFailure(AssertionType type, const char *expr, const char *func, const char *file, int line) {
        AMS_UNUSED(type, expr, func, file, line);

        u32 lr;
        __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
        nxboot::ShowFatalError("Assert called, lr=%p\n", reinterpret_cast<void *>(lr));
    }

   NORETURN void OnAssertionFailure(AssertionType type, const char *expr, const char *func, const char *file, int line, const char *format, ...) {
        AMS_UNUSED(type, expr, func, file, line, format);

        u32 lr;
        __asm__ __volatile__("mov %0, lr" : "=r"(lr) :: "memory");
        nxboot::ShowFatalError("Assert called, lr=%p\n", reinterpret_cast<void *>(lr));
    }

}
