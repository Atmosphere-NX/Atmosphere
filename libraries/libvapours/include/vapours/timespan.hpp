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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/util_type_traits.hpp>
#include <chrono>

namespace ams {

    struct TimeSpanType {
        public:
            s64 _ns;
        public:
            static constexpr ALWAYS_INLINE TimeSpanType FromNanoSeconds(s64 ns)  { return {ns}; }
            static constexpr ALWAYS_INLINE TimeSpanType FromMicroSeconds(s64 ms) { return FromNanoSeconds(ms * INT64_C(1000)); }
            static constexpr ALWAYS_INLINE TimeSpanType FromMilliSeconds(s64 ms) { return FromMicroSeconds(ms * INT64_C(1000)); }
            static constexpr ALWAYS_INLINE TimeSpanType FromSeconds(s64 s)       { return FromMilliSeconds(s * INT64_C(1000)); }
            static constexpr ALWAYS_INLINE TimeSpanType FromMinutes(s64 m)       { return FromSeconds(m * INT64_C(60)); }
            static constexpr ALWAYS_INLINE TimeSpanType FromHours(s64 h)         { return FromMinutes(h * INT64_C(60)); }
            static constexpr ALWAYS_INLINE TimeSpanType FromDays(s64 d)          { return FromHours(d * INT64_C(24)); }

            constexpr ALWAYS_INLINE s64 GetNanoSeconds()  const { return _ns; }
            constexpr ALWAYS_INLINE s64 GetMicroSeconds() const { return this->GetNanoSeconds() / (INT64_C(1000)); }
            constexpr ALWAYS_INLINE s64 GetMilliSeconds() const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000)); }
            constexpr ALWAYS_INLINE s64 GetSeconds()      const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000) * INT64_C(1000)); }
            constexpr ALWAYS_INLINE s64 GetMinutes()      const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000) * INT64_C(1000) * INT64_C(  60)); }
            constexpr ALWAYS_INLINE s64 GetHours()        const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000) * INT64_C(1000) * INT64_C(  60) * INT64_C(  60)); }
            constexpr ALWAYS_INLINE s64 GetDays()         const { return this->GetNanoSeconds() / (INT64_C(1000) * INT64_C(1000) * INT64_C(1000) * INT64_C(  60) * INT64_C(  60) * INT64_C(  24)); }

            constexpr ALWAYS_INLINE friend bool operator==(const TimeSpanType &lhs, const TimeSpanType &rhs) { return lhs._ns == rhs._ns; }
            constexpr ALWAYS_INLINE friend bool operator!=(const TimeSpanType &lhs, const TimeSpanType &rhs) { return lhs._ns != rhs._ns; }
            constexpr ALWAYS_INLINE friend bool operator<=(const TimeSpanType &lhs, const TimeSpanType &rhs) { return lhs._ns <= rhs._ns; }
            constexpr ALWAYS_INLINE friend bool operator>=(const TimeSpanType &lhs, const TimeSpanType &rhs) { return lhs._ns >= rhs._ns; }
            constexpr ALWAYS_INLINE friend bool operator< (const TimeSpanType &lhs, const TimeSpanType &rhs) { return lhs._ns <  rhs._ns; }
            constexpr ALWAYS_INLINE friend bool operator> (const TimeSpanType &lhs, const TimeSpanType &rhs) { return lhs._ns >  rhs._ns; }

            constexpr ALWAYS_INLINE TimeSpanType &operator+=(const TimeSpanType &rhs) { _ns += rhs._ns; return *this; }
            constexpr ALWAYS_INLINE TimeSpanType &operator-=(const TimeSpanType &rhs) { _ns -= rhs._ns; return *this; }

            constexpr ALWAYS_INLINE friend TimeSpanType operator+(const TimeSpanType &lhs, const TimeSpanType &rhs) { TimeSpanType r(lhs); return r += rhs; }
            constexpr ALWAYS_INLINE friend TimeSpanType operator-(const TimeSpanType &lhs, const TimeSpanType &rhs) { TimeSpanType r(lhs); return r -= rhs; }
    };
    static_assert(util::is_pod<TimeSpanType>::value);

    class TimeSpan {
        private:
            using ZeroTag = const class ZeroTagImpl{} *;
        private:
            TimeSpanType m_ts;
        public:
            constexpr ALWAYS_INLINE TimeSpan(ZeroTag z = nullptr) : m_ts(TimeSpanType::FromNanoSeconds(0)) { AMS_UNUSED(z); /* ... */ }
            constexpr ALWAYS_INLINE TimeSpan(const TimeSpanType &t) : m_ts(t) { /* ... */ }

            template<typename R, typename P>
            constexpr ALWAYS_INLINE TimeSpan(const std::chrono::duration<R, P>& c) : m_ts(TimeSpanType::FromNanoSeconds(static_cast<std::chrono::nanoseconds>(c).count())) { /* ... */ }
        public:
            static constexpr ALWAYS_INLINE TimeSpan FromNanoSeconds(s64 ns)  { return TimeSpanType::FromNanoSeconds(ns); }
            static constexpr ALWAYS_INLINE TimeSpan FromMicroSeconds(s64 ms) { return TimeSpanType::FromMicroSeconds(ms); }
            static constexpr ALWAYS_INLINE TimeSpan FromMilliSeconds(s64 ms) { return TimeSpanType::FromMilliSeconds(ms); }
            static constexpr ALWAYS_INLINE TimeSpan FromSeconds(s64 s)       { return TimeSpanType::FromSeconds(s); }
            static constexpr ALWAYS_INLINE TimeSpan FromMinutes(s64 m)       { return TimeSpanType::FromMinutes(m); }
            static constexpr ALWAYS_INLINE TimeSpan FromHours(s64 h)         { return TimeSpanType::FromHours(h); }
            static constexpr ALWAYS_INLINE TimeSpan FromDays(s64 d)          { return TimeSpanType::FromDays(d); }

            constexpr ALWAYS_INLINE s64 GetNanoSeconds()  const { return m_ts.GetNanoSeconds(); }
            constexpr ALWAYS_INLINE s64 GetMicroSeconds() const { return m_ts.GetMicroSeconds(); }
            constexpr ALWAYS_INLINE s64 GetMilliSeconds() const { return m_ts.GetMilliSeconds(); }
            constexpr ALWAYS_INLINE s64 GetSeconds()      const { return m_ts.GetSeconds(); }
            constexpr ALWAYS_INLINE s64 GetMinutes()      const { return m_ts.GetMinutes(); }
            constexpr ALWAYS_INLINE s64 GetHours()        const { return m_ts.GetHours(); }
            constexpr ALWAYS_INLINE s64 GetDays()         const { return m_ts.GetDays(); }

            constexpr ALWAYS_INLINE friend bool operator==(const TimeSpan &lhs, const TimeSpan &rhs) { return lhs.m_ts == rhs.m_ts; }
            constexpr ALWAYS_INLINE friend bool operator!=(const TimeSpan &lhs, const TimeSpan &rhs) { return lhs.m_ts != rhs.m_ts; }
            constexpr ALWAYS_INLINE friend bool operator<=(const TimeSpan &lhs, const TimeSpan &rhs) { return lhs.m_ts <= rhs.m_ts; }
            constexpr ALWAYS_INLINE friend bool operator>=(const TimeSpan &lhs, const TimeSpan &rhs) { return lhs.m_ts >= rhs.m_ts; }
            constexpr ALWAYS_INLINE friend bool operator< (const TimeSpan &lhs, const TimeSpan &rhs) { return lhs.m_ts <  rhs.m_ts; }
            constexpr ALWAYS_INLINE friend bool operator> (const TimeSpan &lhs, const TimeSpan &rhs) { return lhs.m_ts >  rhs.m_ts; }

            constexpr ALWAYS_INLINE TimeSpan &operator+=(const TimeSpan &rhs) { m_ts += rhs.m_ts; return *this; }
            constexpr ALWAYS_INLINE TimeSpan &operator-=(const TimeSpan &rhs) { m_ts -= rhs.m_ts; return *this; }

            constexpr ALWAYS_INLINE friend TimeSpan operator+(const TimeSpan &lhs, const TimeSpan &rhs) { TimeSpan r(lhs); return r += rhs; }
            constexpr ALWAYS_INLINE friend TimeSpan operator-(const TimeSpan &lhs, const TimeSpan &rhs) { TimeSpan r(lhs); return r -= rhs; }

            constexpr ALWAYS_INLINE operator TimeSpanType() const {
                return m_ts;
            }
    };

}