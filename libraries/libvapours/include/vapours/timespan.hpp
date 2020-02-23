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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <chrono>

namespace ams {

    class TimeSpan {
        private:
            s64 ns;
        private:
            constexpr explicit ALWAYS_INLINE TimeSpan(s64 v) : ns(v) { /* ... */ }
        public:
            constexpr explicit ALWAYS_INLINE TimeSpan() : TimeSpan(0) { /* ... */ }

            static constexpr ALWAYS_INLINE TimeSpan FromNanoSeconds(s64 ns)  { return TimeSpan(ns); }
            static constexpr ALWAYS_INLINE TimeSpan FromMicroSeconds(s64 ms) { return FromNanoSeconds(ms * INT64_C(1000)); }
            static constexpr ALWAYS_INLINE TimeSpan FromMilliSeconds(s64 ms) { return FromMicroSeconds(ms * INT64_C(1000)); }
            static constexpr ALWAYS_INLINE TimeSpan FromSeconds(s64 s)       { return FromMilliSeconds(s * INT64_C(1000)); }
            static constexpr ALWAYS_INLINE TimeSpan FromMinutes(s64 m)       { return FromSeconds(m * INT64_C(60)); }
            static constexpr ALWAYS_INLINE TimeSpan FromHours(s64 h)         { return FromMinutes(h * INT64_C(60)); }
            static constexpr ALWAYS_INLINE TimeSpan FromDays(s64 d)          { return FromMinutes(d * INT64_C(24)); }

            template<typename R, typename P>
            constexpr explicit ALWAYS_INLINE TimeSpan(const std::chrono::duration<R, P>& c) : TimeSpan(static_cast<std::chrono::nanoseconds>(c).count()) { /* ... */ }
        public:
            constexpr ALWAYS_INLINE s64 GetNanoSeconds()  const { return this->ns; }
            constexpr ALWAYS_INLINE s64 GetMicroSeconds() const { return this->GetNanoSeconds() / (INT64_C(1000)); }
            constexpr ALWAYS_INLINE s64 GetMilliSeconds() const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000)); }
            constexpr ALWAYS_INLINE s64 GetSeconds()      const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000) * INT64_C(1000)); }
            constexpr ALWAYS_INLINE s64 GetMinutes()      const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000) * INT64_C(1000) * INT64_C(  60)); }
            constexpr ALWAYS_INLINE s64 GetHours()        const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000) * INT64_C(1000) * INT64_C(  60) * INT64_C(  60)); }
            constexpr ALWAYS_INLINE s64 GetDays()         const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000) * INT64_C(1000) * INT64_C(  60) * INT64_C(  60) * INT64_C(  24)); }

            constexpr ALWAYS_INLINE bool operator==(const TimeSpan &rhs) const { return this->ns == rhs.ns; }
            constexpr ALWAYS_INLINE bool operator!=(const TimeSpan &rhs) const { return this->ns != rhs.ns; }
            constexpr ALWAYS_INLINE bool operator<=(const TimeSpan &rhs) const { return this->ns <= rhs.ns; }
            constexpr ALWAYS_INLINE bool operator>=(const TimeSpan &rhs) const { return this->ns >= rhs.ns; }
            constexpr ALWAYS_INLINE bool operator< (const TimeSpan &rhs) const { return this->ns <  rhs.ns; }
            constexpr ALWAYS_INLINE bool operator> (const TimeSpan &rhs) const { return this->ns >  rhs.ns; }

            constexpr ALWAYS_INLINE TimeSpan &operator+=(TimeSpan rhs) { this->ns += rhs.ns; return *this; }
            constexpr ALWAYS_INLINE TimeSpan &operator-=(TimeSpan rhs) { this->ns -= rhs.ns; return *this; }
            constexpr ALWAYS_INLINE TimeSpan operator+(TimeSpan rhs) const { TimeSpan r(*this); return r += rhs; }
            constexpr ALWAYS_INLINE TimeSpan operator-(TimeSpan rhs) const { TimeSpan r(*this); return r -= rhs; }
    };

}