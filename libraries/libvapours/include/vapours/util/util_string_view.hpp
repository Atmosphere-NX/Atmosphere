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

namespace ams::util {

    template<class _CharT, class _Traits = std::char_traits<_CharT>>
    class basic_string_view {
        static_assert(!std::is_array<_CharT>::value);
        static_assert(std::is_trivial<_CharT>::value && std::is_standard_layout<_CharT>::value);
        static_assert(std::same_as<_CharT, typename _Traits::char_type>);
        public:
            using traits_type               = _Traits;
            using value_type                = _CharT;
            using pointer                   = value_type *;
            using const_pointer             = const value_type *;
            using reference                 = value_type &;
            using const_reference           = const value_type &;
            using const_iterator            = const value_type *;
            using iterator                  = const_iterator;
            using const_reverse_iterator    = std::reverse_iterator<const_iterator>;
            using reverse_iterator          = const_reverse_iterator;
            using size_type                 = size_t;
            using difference_type           = ptrdiff_t;
            static constexpr size_type npos = size_type(-1);
        private:
            static constexpr int _s_compare(size_type lhs, size_type rhs) noexcept {
                const difference_type diff = lhs - rhs;
                if (diff > std::numeric_limits<int>::max()) {
                    return std::numeric_limits<int>::max();
                }
                if (diff < std::numeric_limits<int>::min()) {
                    return std::numeric_limits<int>::min();
                }
                return static_cast<int>(diff);
            }
        private:
            const_pointer m_str;
            size_type     m_len;
        public:
            constexpr basic_string_view() noexcept : m_str(nullptr), m_len(0) { /* ... */ }
            constexpr basic_string_view(const _CharT *str, size_type len) noexcept : m_str(str), m_len(len) { /* ... */ }
            constexpr basic_string_view(const _CharT *str) noexcept : m_str(str), m_len(str ? traits_type::length(str) : 0) { /* ... */ }

            template<std::contiguous_iterator _It, std::sized_sentinel_for<_It> _End> requires std::same_as<std::iter_value_t<_It>, _CharT> && (!std::convertible_to<_End, size_type>)
            constexpr basic_string_view(_It first, _End last) noexcept : m_str(std::to_address(first)), m_len(last - first) { /* ... */ }

            constexpr basic_string_view(const basic_string_view &) noexcept = default;
            constexpr basic_string_view &operator=(const basic_string_view &) noexcept = default;

            constexpr const_iterator begin() const noexcept { return m_str; }
            constexpr const_iterator end()   const noexcept { return m_str + m_len; }

            constexpr const_iterator cbegin() const noexcept { return m_str; }
            constexpr const_iterator cend()   const noexcept { return m_str + m_len; }

            constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(this->end()); }
            constexpr const_reverse_iterator rend()   const noexcept { return const_reverse_iterator(this->begin()); }

            constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(this->cend()); }
            constexpr const_reverse_iterator crend()   const noexcept { return const_reverse_iterator(this->cbegin()); }

            constexpr size_type size() const noexcept { return m_len; }
            constexpr size_type length() const noexcept { return m_len; }

            constexpr size_type max_size() const noexcept { return (npos - sizeof(size_type) - sizeof(void *)) / sizeof(value_type) / 4; }

            [[nodiscard]] constexpr bool empty() const noexcept { return m_len == 0; }

            constexpr const_reference operator[](size_type pos) const noexcept {
                AMS_ASSERT(pos < m_len);
                return *(m_str + pos);
            }

            constexpr const_reference at(size_type pos) const noexcept {
                AMS_ASSERT(pos < m_len);
                AMS_ABORT_UNLESS(pos < m_len);
                return *(m_str + pos);
            }

            constexpr const_reference front() const noexcept {
                AMS_ASSERT(m_len > 0);
                return *m_str;
            }

            constexpr const_reference back() const noexcept {
                AMS_ASSERT(m_len > 0);
                return *(m_str + m_len - 1);
            }

            constexpr const_pointer data() const noexcept { return m_str; }

            constexpr void remove_prefix(size_type n) noexcept {
                AMS_ASSERT(m_len >= n);
                m_str += n;
                m_len -= n;
            }

            constexpr void remove_suffix(size_type n) noexcept {
                AMS_ASSERT(m_len >= n);
                m_len -= n;
            }

            constexpr void swap(basic_string_view &rhs) noexcept {
                auto tmp = *this;
                *this = rhs;
                rhs = tmp;
            }

            constexpr size_type copy(_CharT *str, size_type n, size_type pos = 0) const noexcept {
                AMS_ASSERT(pos <= this->size());
                const size_type rlen = std::min(n, m_len - pos);
                traits_type::copy(str, this->data() + pos, rlen);
                return rlen;
            }

            constexpr basic_string_view substr(size_type pos = 0, size_type n = npos) const noexcept {
                AMS_ASSERT(pos <= this->size());
                const size_type rlen = std::min(n, m_len - pos);
                return basic_string_view{m_str + pos, rlen};
            }

            constexpr int compare(basic_string_view str) const noexcept {
                const size_type rlen = std::min(m_len, str.m_len);
                int ret = traits_type::compare(m_str, str.m_str, rlen);
                if (ret == 0) {
                    ret = _s_compare(m_len, str.m_len);
                }
                return ret;
            }

            constexpr int compare(size_type pos, size_type n, basic_string_view str) const noexcept {
                return this->substr(pos, n).compare(str);
            }

            constexpr int compare(size_type pos1, size_type n1, basic_string_view str, size_type pos2, size_type n2) const {
                return this->substr(pos1, n1).compare(str.substr(pos2, n2));
            }

            constexpr int compare(const _CharT *str) const noexcept {
                return this->compare(basic_string_view(str));
            }

            constexpr int compare(size_type pos, size_type n, const _CharT *str) const noexcept {
                return this->substr(pos, n).compare(basic_string_view(str));
            }

            constexpr int compare(size_type pos, size_type n, const _CharT *str, size_type n2) const noexcept {
                return this->substr(pos, n).compare(basic_string_view(str, n2));
            }

            constexpr bool starts_with(basic_string_view x) const noexcept { return this->substr(0, x.size()) == x; }
            constexpr bool starts_with(_CharT x) const noexcept { return !this->empty() && traits_type::eq(this->front(), x); }
            constexpr bool starts_with(const _CharT *x) const noexcept { return this->starts_with(basic_string_view(x)); }

            constexpr bool ends_with(basic_string_view x) const noexcept { return this->size() >= x.size() && this->compare(this->size() - x.size(), npos, x) == 0; }
            constexpr bool ends_with(_CharT x) const noexcept { return !this->empty() && traits_type::eq(this->back(), x); }
            constexpr bool ends_with(const _CharT *x) const noexcept { return this->ends_with(basic_string_view(x)); }

            constexpr size_type find(const _CharT *str, size_type pos, size_type n) const noexcept {
                if (n == 0) {
                    return pos + m_len ? pos : npos;
                }

                if (n <= m_len) {
                    for (/* ... */; pos <= m_len - n; ++pos) {
                        if (traits_type::eq(m_str[pos], str[0]) && traits_type::compare(m_str + pos + 1, str + 1, n - 1) == 0) {
                            return pos;
                        }
                    }
                }

                return npos;
            }

            constexpr size_type find(_CharT c, size_type pos = 0) const noexcept {
                size_type ret = npos;
                if (pos < m_len) {
                    const size_type n = m_len - pos;
                    if (const _CharT *p = traits_type::find(m_str + pos, n, c); p) {
                        ret = p - m_str;
                    }
                }
                return ret;
            }

            constexpr size_type find(basic_string_view str, size_type pos = 0) const noexcept { return this->find(str.m_str, pos, str.m_len); }

            __attribute__((nonnull(2)))
            constexpr size_type find(const _CharT *str, size_type pos = 0) const noexcept { return this->find(str, pos, traits_type::length(str)); }

            constexpr size_type rfind(const _CharT *str, size_type pos, size_type n) const noexcept {
                if (n <= m_len) {
                    pos = std::min(size_type(m_len - n), pos);
                    do {
                        if (traits_type::compare(m_str + pos, str, n) == 0) {
                            return pos;
                        }
                    } while (pos-- > 0);
                }

                return npos;
            }

            constexpr size_type rfind(_CharT c, size_type pos = 0) const noexcept {
                size_type size = m_len;
                if (size > 0) {
                    if (--size > pos) {
                        size = pos;
                    }
                    for (++size; size-- > 0; /* ... */) {
                        if (traits_type::eq(m_str[size], c)) {
                            return size;
                        }
                    }
                }
                return npos;
            }

            constexpr size_type rfind(basic_string_view str, size_type pos = 0) const noexcept { return this->rfind(str.m_str, pos, str.m_len); }

            __attribute__((nonnull(2)))
            constexpr size_type rfind(const _CharT *str, size_type pos = 0) const noexcept { return this->rfind(str, pos, traits_type::length(str)); }

            constexpr size_type find_first_of(const _CharT *str, size_type pos, size_t n) const noexcept {
                for (/* ... */; n && pos < m_len; ++pos) {
                    if (const _CharT *p = traits_type::find(str, n, m_str[pos]); p) {
                        return pos;
                    }
                }
                return npos;
            }

            constexpr size_type find_first_of(basic_string_view str, size_type pos = 0) const noexcept { return this->find_first_of(str.m_str, pos, str.m_len); }
            constexpr size_type find_first_of(_CharT c, size_type pos = 0) const noexcept { return this->find(c, pos); }

            __attribute__((nonnull(2)))
            constexpr size_type find_first_of(const _CharT *str, size_type pos = 0) const noexcept { return this->find_first_of(str, pos, traits_type::length(str)); }

            constexpr size_type find_last_of(const _CharT *str, size_type pos, size_t n) const noexcept {
                size_type size = this->size();
                if (size && n) {
                    if (--size > pos) {
                        size = pos;
                    }
                    do {
                        if (traits_type::find(str, n, m_str[size])) {
                            return size;
                        }
                    } while (size-- != 0);
                }
                return npos;
            }

            constexpr size_type find_last_of(basic_string_view str, size_type pos = 0) const noexcept { return this->find_last_of(str.m_str, pos, str.m_len); }
            constexpr size_type find_last_of(_CharT c, size_type pos = 0) const noexcept { return this->rfind(c, pos); }

            __attribute__((nonnull(2)))
            constexpr size_type find_last_of(const _CharT *str, size_type pos = 0) const noexcept { return this->find_first_of(str, pos, traits_type::length(str)); }

            constexpr size_type find_first_not_of(const _CharT *str, size_type pos, size_t n) const noexcept {
                for (/* ... */; pos < m_len; ++pos) {
                    if (!traits_type::find(str, n, m_str[pos])) {
                        return pos;
                    }
                }
                return npos;
            }

            constexpr size_type find_first_not_of(_CharT c, size_type pos = 0) const noexcept {
                for (/* ... */; pos < m_len; ++pos) {
                    if (!traits_type::eq(m_str[pos], c)) {
                        return pos;
                    }
                }
                return npos;
            }

            constexpr size_type find_first_not_of(basic_string_view str, size_type pos = 0) const noexcept { return this->find_first_not_of(str.m_str, pos, str.m_len); }

            __attribute__((nonnull(2)))
            constexpr size_type find_first_not_of(const _CharT *str, size_type pos = 0) const noexcept { return this->find_first_not_of(str, pos, traits_type::length(str)); }

            constexpr size_type find_last_not_of(const _CharT *str, size_type pos, size_t n) const noexcept {
                size_type size = this->size();
                if (size) {
                    if (--size > pos) {
                        size = pos;
                    }
                    do {
                        if (!traits_type::find(str, n, m_str[size])) {
                            return size;
                        }
                    } while (size-- != 0);
                }
                return npos;
            }

            constexpr size_type find_last_not_of(_CharT c, size_type pos = 0) const noexcept {
                size_type size = this->size();
                if (size) {
                    if (--size > pos) {
                        size = pos;
                    }
                    do {
                        if (!traits_type::eq(m_str[size], c)) {
                            return size;
                        }
                    } while (size-- != 0);
                }
                return npos;
            }

            constexpr size_type find_last_not_of(basic_string_view str, size_type pos = 0) const noexcept { return this->find_last_not_of(str.m_str, pos, str.m_len); }

            __attribute__((nonnull(2)))
            constexpr size_type find_last_not_of(const _CharT *str, size_type pos = 0) const noexcept { return this->find_last_not_of(str, pos, traits_type::length(str)); }

            constexpr friend bool operator==(const basic_string_view &lhs, const basic_string_view &rhs) noexcept { return lhs.compare(rhs) == 0; }
            constexpr friend bool operator!=(const basic_string_view &lhs, const basic_string_view &rhs) noexcept { return lhs.compare(rhs) != 0; }
            constexpr friend bool operator<=(const basic_string_view &lhs, const basic_string_view &rhs) noexcept { return lhs.compare(rhs) <= 0; }
            constexpr friend bool operator>=(const basic_string_view &lhs, const basic_string_view &rhs) noexcept { return lhs.compare(rhs) >= 0; }
            constexpr friend bool operator< (const basic_string_view &lhs, const basic_string_view &rhs) noexcept { return lhs.compare(rhs) <  0; }
            constexpr friend bool operator> (const basic_string_view &lhs, const basic_string_view &rhs) noexcept { return lhs.compare(rhs) >  0; }
    };

    template<std::contiguous_iterator _It, std::sized_sentinel_for<_It> _End>
    basic_string_view(_It, _End) -> basic_string_view<std::iter_value_t<_It>>;

    using string_view = basic_string_view<char>;

}
