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

        constinit bool g_auto_abort_enabled = true;

        /* NOTE: This generates a global constructor. */
        os::SdkThreadLocalStorage g_context_tls;

    }

    void SetEnabledAutoAbort(bool enabled) {
        g_auto_abort_enabled = enabled;
    }

    AbortSpecifier DefaultResultHandler(Result result) {
        if (g_auto_abort_enabled) {
            return AbortSpecifier::Default;
        } else {
            return AbortSpecifier::Return;
        }
    }

    AbortSpecifier AlwaysReturnResultHandler(Result result) {
        return AbortSpecifier::Return;
    }

    constinit FsContext g_default_context(DefaultResultHandler);
    constinit FsContext g_always_return_context(AlwaysReturnResultHandler);

    void SetDefaultFsContextResultHandler(const ResultHandler handler) {
        if (handler == nullptr) {
            g_default_context.SetHandler(DefaultResultHandler);
        } else {
            g_default_context.SetHandler(handler);
        }
    }

    const FsContext *GetCurrentThreadFsContext() {
        const FsContext *context = reinterpret_cast<const FsContext *>(g_context_tls.GetValue());

        if (context == nullptr) {
            context = std::addressof(g_default_context);
        }

        return context;
    }

    void SetCurrentThreadFsContext(const FsContext *context) {
        g_context_tls.SetValue(reinterpret_cast<uintptr_t>(context));
    }

    ScopedAutoAbortDisabler::ScopedAutoAbortDisabler() : prev_context(GetCurrentThreadFsContext()) {
        SetCurrentThreadFsContext(std::addressof(g_always_return_context));
    }

}
