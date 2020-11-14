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

namespace ams::result {

    extern bool CallFatalOnResultAssertion;

}

namespace ams::result::impl {

        NORETURN WEAK_SYMBOL void OnResultAbort(const char *file, int line, const char *func, const char *expr, Result result) {
            /* Assert that we should call fatal on result assertion. */
            /* If we shouldn't fatal, this will abort(); */
            /* If we should, we'll continue onwards. */
            if (!ams::result::CallFatalOnResultAssertion) {
                ::ams::diag::AbortImpl(file, line, func, expr, result.GetValue(), "Result Abort: %203d-%04d", result.GetModule(), result.GetDescription());
            }

            /* TODO: ams::fatal:: */
            fatalThrow(result.GetValue());
            AMS_INFINITE_LOOP();
        }

        NORETURN WEAK_SYMBOL void OnResultAbort(Result result) {
            OnResultAbort("", 0, "", "", result);
        }

        NORETURN WEAK_SYMBOL void OnResultAssertion(const char *file, int line, const char *func, const char *expr, Result result) {
            /* Assert that we should call fatal on result assertion. */
            /* If we shouldn't fatal, this will assert(); */
            /* If we should, we'll continue onwards. */
            if (!ams::result::CallFatalOnResultAssertion) {
                ::ams::diag::AssertionFailureImpl(file, line, func, expr, result.GetValue(), "Result Assertion: %203d-%04d", result.GetModule(), result.GetDescription());
            }

            /* TODO: ams::fatal:: */
            fatalThrow(result.GetValue());
            AMS_INFINITE_LOOP();
        }

        NORETURN WEAK_SYMBOL void OnResultAssertion(Result result) {
            OnResultAssertion("", 0, "", "", result);
        }

}