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

    struct TimeSpanType {
        public:
            s64 ns;
        public:
            static constexpr ALWAYS_INLINE TimeSpanType FromNanoSeconds(s64 ns)  { return {ns}; }
            static constexpr ALWAYS_INLINE TimeSpanType FromMicroSeconds(s64 ms) { return FromNanoSeconds(ms * INT64_C(1000)); }
            static constexpr ALWAYS_INLINE TimeSpanType FromMilliSeconds(s64 ms) { return FromMicroSeconds(ms * INT64_C(1000)); }
            static constexpr ALWAYS_INLINE TimeSpanType FromSeconds(s64 s)       { return FromMilliSeconds(s * INT64_C(1000)); }
            static constexpr ALWAYS_INLINE TimeSpanType FromMinutes(s64 m)       { return FromSeconds(m * INT64_C(60)); }
            static constexpr ALWAYS_INLINE TimeSpanType FromHours(s64 h)         { return FromMinutes(h * INT64_C(60)); }
            static constexpr ALWAYS_INLINE TimeSpanType FromDays(s64 d)          { return FromHours(d * INT64_C(24)); }

            constexpr ALWAYS_INLINE s64 GetNanoSeconds()  const { return this->ns; }
            constexpr ALWAYS_INLINE s64 GetMicroSeconds() const { return this->GetNanoSeconds() / (INT64_C(1000)); }
            constexpr ALWAYS_INLINE s64 GetMilliSeconds() const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000)); }
            constexpr ALWAYS_INLINE s64 GetSeconds()      const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000) * INT64_C(1000)); }
            constexpr ALWAYS_INLINE s64 GetMinutes()      const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000) * INT64_C(1000) * INT64_C(  60)); }
            constexpr ALWAYS_INLINE s64 GetHours()        const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000) * INT64_C(1000) * INT64_C(  60) * INT64_C(  60)); }
            constexpr ALWAYS_INLINE s64 GetDays()         const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000) * INT64_C(1000) * INT64_C(  60) * INT64_C(  60) * INT64_C(  24)); }

            constexpr ALWAYS_INLINE friend bool operator==(const TimeSpanType &lhs, const TimeSpanType &rhs) { return lhs.ns == rhs.ns; }
            constexpr ALWAYS_INLINE friend bool operator!=(const TimeSpanType &lhs, const TimeSpanType &rhs) { return lhs.ns != rhs.ns; }
            constexpr ALWAYS_INLINE friend bool operator<=(const TimeSpanType &lhs, const TimeSpanType &rhs) { return lhs.ns <= rhs.ns; }
            constexpr ALWAYS_INLINE friend bool operator>=(const TimeSpanType &lhs, const TimeSpanType &rhs) { return lhs.ns >= rhs.ns; }
            constexpr ALWAYS_INLINE friend bool operator< (const TimeSpanType &lhs, const TimeSpanType &rhs) { return lhs.ns <  rhs.ns; }
            constexpr ALWAYS_INLINE friend bool operator> (const TimeSpanType &lhs, const TimeSpanType &rhs) { return lhs.ns >  rhs.ns; }

            constexpr ALWAYS_INLINE TimeSpanType &operator+=(const TimeSpanType &rhs) { this->ns += rhs.ns; return *this; }
            constexpr ALWAYS_INLINE TimeSpanType &operator-=(const TimeSpanType &rhs) { this->ns -= rhs.ns; return *this; }

            constexpr ALWAYS_INLINE friend TimeSpanType operator+(const TimeSpanType &lhs, const TimeSpanType &rhs) { TimeSpanType r(lhs); return r += rhs; }
            constexpr ALWAYS_INLINE friend TimeSpanType operator-(const TimeSpanType &lhs, const TimeSpanType &rhs) { TimeSpanType r(lhs); return r -= rhs; }
    };

    class TimeSpan {
        private:
            using ZeroTag = const class ZeroTagImpl{} *;
        private:
            TimeSpanType ts;
        public:
            constexpr ALWAYS_INLINE TimeSpan(ZeroTag z = nullptr) : ts(TimeSpanType::FromNanoSeconds(0)) { AMS_UNUSED(z); /* ... */ }
            constexpr ALWAYS_INLINE TimeSpan(const TimeSpanType &t) : ts(t) { /* ... */ }

            template<typename R, typename P>
            constexpr ALWAYS_INLINE TimeSpan(const std::chrono::duration<R, P>& c) : ts(TimeSpanType::FromNanoSeconds(static_cast<std::chrono::nanoseconds>(c).count())) { /* ... */ }
        public:
            static constexpr ALWAYS_INLINE TimeSpan FromNanoSeconds(s64 ns)  { return TimeSpanType::FromNanoSeconds(ns); }
            static constexpr ALWAYS_INLINE TimeSpan FromMicroSeconds(s64 ms) { return TimeSpanType::FromMicroSeconds(ms); }
            static constexpr ALWAYS_INLINE TimeSpan FromMilliSeconds(s64 ms) { return TimeSpanType::FromMilliSeconds(ms); }
            static constexpr ALWAYS_INLINE TimeSpan FromSeconds(s64 s)       { return TimeSpanType::FromSeconds(s); }
            static constexpr ALWAYS_INLINE TimeSpan FromMinutes(s64 m)       { return TimeSpanType::FromMinutes(m); }
            static constexpr ALWAYS_INLINE TimeSpan FromHours(s64 h)         { return TimeSpanType::FromHours(h); }
            static constexpr ALWAYS_INLINE TimeSpan FromDays(s64 d)          { return TimeSpanType::FromDays(d); }

            constexpr ALWAYS_INLINE s64 GetNanoSeconds()  const { return this->ts.GetNanoSeconds(); }
            constexpr ALWAYS_INLINE s64 GetMicroSeconds() const { return this->ts.GetMicroSeconds(); }
            constexpr ALWAYS_INLINE s64 GetMilliSeconds() const { return this->ts.GetMilliSeconds(); }
            constexpr ALWAYS_INLINE s64 GetSeconds()      const { return this->ts.GetSeconds(); }
            constexpr ALWAYS_INLINE s64 GetMinutes()      const { return this->ts.GetMinutes(); }
            constexpr ALWAYS_INLINE s64 GetHours()        const { return this->ts.GetHours(); }
            constexpr ALWAYS_INLINE s64 GetDays()         const { return this->ts.GetDays(); }

            constexpr ALWAYS_INLINE friend bool operator==(const TimeSpan &lhs, const TimeSpan &rhs) { return lhs.ts == rhs.ts; }
            constexpr ALWAYS_INLINE friend bool operator!=(const TimeSpan &lhs, const TimeSpan &rhs) { return lhs.ts != rhs.ts; }
            constexpr ALWAYS_INLINE friend bool operator<=(const TimeSpan &lhs, const TimeSpan &rhs) { return lhs.ts <= rhs.ts; }
            constexpr ALWAYS_INLINE friend bool operator>=(const TimeSpan &lhs, const TimeSpan &rhs) { return lhs.ts >= rhs.ts; }
            constexpr ALWAYS_INLINE friend bool operator< (const TimeSpan &lhs, const TimeSpan &rhs) { return lhs.ts <  rhs.ts; }
            constexpr ALWAYS_INLINE friend bool operator> (const TimeSpan &lhs, const TimeSpan &rhs) { return lhs.ts >  rhs.ts; }

            constexpr ALWAYS_INLINE TimeSpan &operator+=(const TimeSpan &rhs) { this->ts += rhs.ts; return *this; }
            constexpr ALWAYS_INLINE TimeSpan &operator-=(const TimeSpan &rhs) { this->ts -= rhs.ts; return *this; }

            constexpr ALWAYS_INLINE friend TimeSpan operator+(const TimeSpan &lhs, const TimeSpan &rhs) { TimeSpan r(lhs); return r += rhs; }
            constexpr ALWAYS_INLINE friend TimeSpan operator-(const TimeSpan &lhs, const TimeSpan &rhs) { TimeSpan r(lhs); return r -= rhs; }

            constexpr ALWAYS_INLINE operator TimeSpanType() const {
                return this->ts;
            }
    };

}