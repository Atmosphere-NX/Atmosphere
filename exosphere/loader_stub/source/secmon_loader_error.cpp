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
#include "secmon_loader_error.hpp"

namespace ams::diag {

    NORETURN void AbortImpl(const char *file, int line, const char *func, const char *expr, u64 value, const char *format, ...) {
        AMS_UNUSED(file, line, func, expr, value, format);
        ams::secmon::loader::ErrorReboot();
    }

    NORETURN void AbortImpl(const char *file, int line, const char *func, const char *expr, u64 value) {
        AMS_UNUSED(file, line, func, expr, value);
        ams::secmon::loader::ErrorReboot();
    }

    NORETURN void AbortImpl() {
        ams::secmon::loader::ErrorReboot();
    }

}

namespace ams::secmon::loader {

    NORETURN void ErrorReboot() {
        /* Invalidate the security engine. */
        /* TODO */

        /* Reboot. */
        while (true) {
            wdt::Reboot();
        }
    }

}