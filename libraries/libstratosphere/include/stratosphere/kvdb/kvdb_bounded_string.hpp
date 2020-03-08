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

namespace ams::kvdb {

    /* Represents a string with a backing buffer of N bytes. */
    template<size_t N>
    class BoundedString {
        static_assert(N > 0, "BoundedString requires non-zero backing buffer!");
        private:
            char buffer[N];
        private:
            /* Utility. */
            static inline void CheckLength(size_t len) {
                AMS_ABORT_UNLESS(len < N);
            }
        public:
            /* Constructors. */
            constexpr BoundedString() {
                buffer[0] = 0;
            }

            explicit constexpr BoundedString(const char *s) {
                this->Set(s);
            }

            /* Static constructors. */
            static constexpr BoundedString<N> Make(const char *s) {
                return BoundedString<N>(s);
            }

            static constexpr BoundedString<N> MakeFormat(const char *format, ...) __attribute__((format (printf, 1, 2)))  {
                BoundedString<N> string;

                std::va_list args;
                va_start(args, format);
                CheckLength(std::vsnprintf(string.buffer, N, format, args));
                string.buffer[N - 1] = 0;
                va_end(args);

                return string;
            }

            /* Getters. */
            size_t GetLength() const {
                return strnlen(this->buffer, N);
            }

            const char *Get() const {
                return this->buffer;
            }

            operator const char *() const {
                return this->buffer;
            }

            /* Setters. */
            void Set(const char *s) {
                /* Ensure string can fit in our buffer. */
                CheckLength(strnlen(s, N));
                std::strncpy(this->buffer, s, N);
                this->buffer[N - 1] = 0;
            }

            void SetFormat(const char *format, ...) __attribute__((format (printf, 2, 3))) {
                /* Format into the buffer, abort if too large. */
                std::va_list args;
                va_start(args, format);
                CheckLength(std::vsnprintf(this->buffer, N, format, args));
                va_end(args);
            }

            /* Append to existing. */
            void Append(const char *s) {
                const size_t length = GetLength();
                CheckLength(length + strnlen(s, N));
                std::strncat(this->buffer, s, N - length - 1);
            }

            void Append(char c) {
                const size_t length = GetLength();
                CheckLength(length + 1);
                this->buffer[length] = c;
                this->buffer[length + 1] = 0;
            }

            void AppendFormat(const char *format, ...) __attribute__((format (printf, 2, 3))) {
                const size_t length = GetLength();
                std::va_list args;
                va_start(args, format);
                CheckLength(std::vsnprintf(this->buffer + length, N - length, format, args) + length);
                va_end(args);
            }

            /* Substring utilities. */
            void GetSubstring(char *dst, size_t dst_size, size_t offset, size_t length) const {
                /* Make sure output buffer can hold the substring. */
                AMS_ABORT_UNLESS(offset + length <= GetLength());
                AMS_ABORT_UNLESS(dst_size > length);
                /* Copy substring to dst. */
                std::strncpy(dst, this->buffer + offset, length);
                dst[length] = 0;
            }

            BoundedString<N> GetSubstring(size_t offset, size_t length) const {
                BoundedString<N> string;
                GetSubstring(string.buffer, N, offset, length);
                return string;
            }

            /* Comparison. */
            constexpr bool operator==(const BoundedString<N> &rhs) const {
                return std::strncmp(this->buffer, rhs.buffer, N) == 0;
            }

            constexpr bool operator!=(const BoundedString<N> &rhs) const {
                return !(*this == rhs);
            }

            bool EndsWith(const char *s, size_t offset) const {
                return std::strncmp(this->buffer + offset, s, N - offset) == 0;
            }

            bool EndsWith(const char *s) const {
                const size_t suffix_length = strnlen(s, N);
                const size_t length = GetLength();
                return suffix_length <= length && EndsWith(s, length - suffix_length);
            }
    };

}