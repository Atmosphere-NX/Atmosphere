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

        constinit bool g_handled_by_application = false;

    }

    void SetResultHandledByApplication(bool application) {
        g_handled_by_application = application;
    }

    namespace impl {

        bool IsAbortNeeded(Result result) {
            /* If the result succeeded, we never need to abort. */
            if (R_SUCCEEDED(result)) {
                return false;
            }

            /* Get the abort specifier from current context. */
            switch (GetCurrentThreadFsContext()->HandleResult(result)) {
                case AbortSpecifier::Default:
                    if (g_handled_by_application) {
                        return !fs::ResultHandledByAllProcess::Includes(result);
                    } else {
                        return !(fs::ResultHandledByAllProcess::Includes(result) || fs::ResultHandledBySystemProcess::Includes(result));
                    }
                case AbortSpecifier::Abort:
                    return true;
                case AbortSpecifier::Return:
                    return false;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        void LogResultErrorMessage(Result result) {
            /* TODO: log specific results */
        }

        void LogErrorMessage(Result result, const char *function) {
            /* If the result succeeded, there's nothing to log. */
            if (R_SUCCEEDED(result)) {
                return;
            }

            /* TODO: Actually log stuff. */
        }

    }

}
