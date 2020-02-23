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

 /* Scope guard logic lovingly taken from Andrei Alexandrescu's "Systemic Error Handling in C++" */
#pragma once
#include <vapours/common.hpp>
#include <vapours/assert.hpp>

namespace ams::util {

    namespace impl {

        template<class F>
        class ScopeGuard {
            NON_COPYABLE(ScopeGuard);
            private:
                F f;
                bool active;
            public:
                constexpr ALWAYS_INLINE ScopeGuard(F f) : f(std::move(f)), active(true) { }
                ALWAYS_INLINE ~ScopeGuard() { if (active) { f(); } }
                ALWAYS_INLINE void Cancel() { active = false; }

                ALWAYS_INLINE ScopeGuard(ScopeGuard&& rhs) : f(std::move(rhs.f)), active(rhs.active) {
                    rhs.Cancel();
                }
        };

        template<class F>
        constexpr ALWAYS_INLINE ScopeGuard<F> MakeScopeGuard(F f) {
            return ScopeGuard<F>(std::move(f));
        }

        enum class ScopeGuardOnExit {};

        template <typename F>
        constexpr ALWAYS_INLINE ScopeGuard<F> operator+(ScopeGuardOnExit, F&& f) {
            return ScopeGuard<F>(std::forward<F>(f));
        }

    }

}

#define SCOPE_GUARD   ::ams::util::impl::ScopeGuardOnExit() + [&]() ALWAYS_INLINE_LAMBDA
#define ON_SCOPE_EXIT auto ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE_) = SCOPE_GUARD
