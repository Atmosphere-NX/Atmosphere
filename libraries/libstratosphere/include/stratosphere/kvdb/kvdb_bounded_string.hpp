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

namespace ams::kvdb {

    /* Represents a string with a backing buffer of N bytes. */
    template<size_t N>
    class BoundedString {
        static_assert(N > 0, "BoundedString requires non-zero backing buffer!");
        private:
            char m_buffer[N];
        public:
            /* Constructors. */
            constexpr BoundedString() {
                m_buffer[0] = 0;
            }

            explicit constexpr BoundedString(const char *s) {
                AMS_ABORT_UNLESS(static_cast<size_t>(util::Strnlen(s, N)) < N);
                util::Strlcpy(m_buffer, s, N);
            }

            /* Static constructors. */
            static constexpr BoundedString<N> Make(const char *s) {
                return BoundedString<N>(s);
            }

            static BoundedString<N> MakeFormat(const char *format, ...) __attribute__((format (printf, 1, 2)))  {
                BoundedString<N> string;

                std::va_list vl;
                va_start(vl, format);
                AMS_ABORT_UNLESS(static_cast<size_t>(util::VSNPrintf(string.m_buffer, N, format, vl)) < N);
                va_end(vl);

                return string;
            }

            /* Getters. */
            constexpr size_t GetLength() const {
                return util::Strnlen(m_buffer, N);
            }

            constexpr const char *Get() const {
                return m_buffer;
            }

            constexpr operator const char *() const {
                return m_buffer;
            }

            /* Assignment. */
            constexpr BoundedString<N> &Assign(const char *s) {
                AMS_ABORT_UNLESS(static_cast<size_t>(util::Strnlen(s, N)) < N);
                util::Strlcpy(m_buffer, s, N);

                return *this;
            }

            BoundedString<N> &AssignFormat(const char *format, ...) __attribute__((format (printf, 2, 3))) {
                std::va_list vl;
                va_start(vl, format);
                AMS_ABORT_UNLESS(static_cast<size_t>(util::VSNPrintf(m_buffer, N, format, vl)) < N);
                va_end(vl);

                return *this;
            }

            /* Append to existing. */
            BoundedString<N> &Append(const char *s) {
                const size_t length = GetLength();
                AMS_ABORT_UNLESS(length + static_cast<size_t>(util::Strnlen(s, N)) < N);
                std::strncat(m_buffer, s, N - length - 1);

                return *this;
            }

            BoundedString<N> &Append(char c) {
                const size_t length = GetLength();
                AMS_ABORT_UNLESS(length + 1 < N);
                m_buffer[length] = c;
                m_buffer[length + 1] = 0;

                return *this;
            }

            BoundedString<N> &AppendFormat(const char *format, ...) __attribute__((format (printf, 2, 3))) {
                const size_t length = GetLength();

                std::va_list vl;
                va_start(vl, format);
                AMS_ABORT_UNLESS(static_cast<size_t>(util::VSNPrintf(m_buffer + length, N - length, format, vl)) < static_cast<size_t>(N - length));
                va_end(vl);

                return *this;
            }

            /* Substring utilities. */
            void GetSubString(char *dst, size_t dst_size, size_t offset, size_t length) const {
                /* Make sure output buffer can hold the substring. */
                AMS_ABORT_UNLESS(offset + length <= GetLength());
                AMS_ABORT_UNLESS(dst_size > length);

                /* Copy substring to dst. */
                std::strncpy(dst, m_buffer + offset, length);
                dst[length] = 0;
            }

            BoundedString<N> MakeSubString(size_t offset, size_t length) const {
                BoundedString<N> string;
                GetSubString(string.m_buffer, N, offset, length);
                return string;
            }

            /* Comparison. */
            constexpr bool Equals(const char *s, size_t offset = 0) const {
                if (std::is_constant_evaluated()) {
                    return util::Strncmp(m_buffer + offset, s, N - offset) == 0;
                } else {
                    return std::strncmp(m_buffer + offset, s, N - offset) == 0;
                }
            }

            constexpr bool operator==(const BoundedString<N> &rhs) const {
                return this->Equals(rhs.m_buffer);
            }

            constexpr bool operator!=(const BoundedString<N> &rhs) const {
                return !(*this == rhs);
            }

            constexpr bool EqualsPostfix(const char *s) const {
                const size_t suffix_length = util::Strnlen(s, N);
                const size_t length = GetLength();
                return suffix_length <= length && this->Equals(s, length - suffix_length);
            }
    };

}