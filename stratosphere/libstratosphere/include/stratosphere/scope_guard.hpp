/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <utility>

template<class F>
class ScopeGuard {
private:
    F f;
    bool active;
public:
    ScopeGuard(F f) : f(std::move(f)), active(true) { }
    ~ScopeGuard() { if (active) { f(); } }
    void Cancel() { active = false; }

    ScopeGuard() = delete;
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&& rhs) : f(std::move(rhs.f)), active(rhs.active) {
        rhs.Cancel();
    }
};

template<class F>
ScopeGuard<F> MakeScopeGuard(F f) {
    return ScopeGuard<F>(std::move(f));
}

enum class ScopeGuardOnExit {};

template <typename F>
ScopeGuard<F> operator+(ScopeGuardOnExit, F&& f) {
    return ScopeGuard<F>(std::forward<F>(f));
}

#define CONCATENATE_IMPL(S1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)

#ifdef __COUNTER__
#define ANONYMOUS_VARIABLE(pref) CONCATENATE(pref, __COUNTER__)
#else
#define ANONYMOUS_VARIABLE(pref) CONCATENATE(pref, __LINE__)
#endif

#define SCOPE_GUARD ScopeGuardOnExit() + [&]()
#define ON_SCOPE_EXIT auto ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE_) = SCOPE_GUARD
