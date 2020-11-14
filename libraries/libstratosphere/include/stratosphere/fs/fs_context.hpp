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
            ResultHandler handler;
        public:
            constexpr explicit FsContext(ResultHandler h) : handler(h) { /* ... */ }

            constexpr void SetHandler(ResultHandler h) { this->handler = h; }

            constexpr AbortSpecifier HandleResult(Result result) const { return this->handler(result); }
    };

    void SetDefaultFsContextResultHandler(const ResultHandler handler);

    const FsContext *GetCurrentThreadFsContext();
    void SetCurrentThreadFsContext(const FsContext *context);

    class ScopedFsContext {
        private:
            const FsContext * const prev_context;
        public:
            ALWAYS_INLINE ScopedFsContext(const FsContext &ctx) : prev_context(GetCurrentThreadFsContext()) {
                SetCurrentThreadFsContext(std::addressof(ctx));
            }

            ALWAYS_INLINE ~ScopedFsContext() {
                SetCurrentThreadFsContext(this->prev_context);
            }
    };

    class ScopedAutoAbortDisabler {
        private:
            const FsContext * const prev_context;
        public:
            ScopedAutoAbortDisabler();
            ALWAYS_INLINE ~ScopedAutoAbortDisabler() {
                SetCurrentThreadFsContext(this->prev_context);
            }
    };

}
