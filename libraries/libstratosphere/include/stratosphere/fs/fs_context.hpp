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
#pragma once
#include <vapours.hpp>

namespace ams::fs {

    enum class AbortSpecifier {
        Default,
        Abort,
        Return,
    };

    using ResultHandler = AbortSpecifier (*)(Result);

    class FsContext {
        private:
            ResultHandler m_handler;
        public:
            constexpr explicit FsContext(ResultHandler h) : m_handler(h) { /* ... */ }

            constexpr void SetHandler(ResultHandler h) { m_handler = h; }

            constexpr AbortSpecifier HandleResult(Result result) const { return m_handler(result); }
    };

    void SetDefaultFsContextResultHandler(const ResultHandler handler);

    const FsContext *GetCurrentThreadFsContext();
    void SetCurrentThreadFsContext(const FsContext *context);

    class ScopedFsContext {
        private:
            const FsContext * const m_prev_context;
        public:
            ALWAYS_INLINE ScopedFsContext(const FsContext &ctx) : m_prev_context(GetCurrentThreadFsContext()) {
                SetCurrentThreadFsContext(std::addressof(ctx));
            }

            ALWAYS_INLINE ~ScopedFsContext() {
                SetCurrentThreadFsContext(m_prev_context);
            }
    };

    class ScopedAutoAbortDisabler {
        private:
            const FsContext * const m_prev_context;
        public:
            ScopedAutoAbortDisabler();
            ALWAYS_INLINE ~ScopedAutoAbortDisabler() {
                SetCurrentThreadFsContext(m_prev_context);
            }
    };

}
