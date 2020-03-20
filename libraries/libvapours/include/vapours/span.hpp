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

namespace ams {

    /* TODO C++20 switch to template<typename T> using Span = std::span<T> */

    namespace impl {

        template<typename Span>
        class SpanConstIterator;

        template<typename Span, typename Derived, typename Reference>
        class SpanIteratorImpl {
            public:
                friend class SpanConstIterator<Span>;

                using index_type        = typename Span::index_type;
                using difference_type   = typename Span::difference_type;
                using value_type        = typename std::remove_cv<typename Span::element_type>::type;
                using pointer           = typename std::add_pointer<Reference>::type;
                using reference         = Reference;
                using iterator_category = std::random_access_iterator_tag;
            private:
                const Span *span = nullptr;
                index_type index = 0;
            public:
                constexpr ALWAYS_INLINE SpanIteratorImpl() = default;
                constexpr ALWAYS_INLINE SpanIteratorImpl(const Span *s, index_type idx) : span(s), index(idx) { /* ... */ }

                constexpr ALWAYS_INLINE pointer operator->() const {
                    return this->span->data() + this->index;
                }

                constexpr ALWAYS_INLINE reference operator*() const {
                    return *this->operator->();
                }

                constexpr ALWAYS_INLINE Derived operator++(int) {
                    auto prev = static_cast<Derived &>(*this);
                    ++(*this);
                    return prev;
                }

                constexpr ALWAYS_INLINE Derived operator--(int) {
                    auto prev = static_cast<Derived &>(*this);
                    --(*this);
                    return prev;
                }

                constexpr ALWAYS_INLINE Derived &operator++() { ++this->index; return static_cast<Derived &>(*this); }
                constexpr ALWAYS_INLINE Derived &operator--() { --this->index; return static_cast<Derived &>(*this); }

                constexpr ALWAYS_INLINE Derived &operator+=(difference_type n) { this->index += n; return static_cast<Derived &>(*this); }
                constexpr ALWAYS_INLINE Derived &operator-=(difference_type n) { this->index -= n; return static_cast<Derived &>(*this); }

                constexpr ALWAYS_INLINE Derived operator+(difference_type n) const { auto r = static_cast<const Derived &>(*this); return r += n; }
                constexpr ALWAYS_INLINE Derived operator-(difference_type n) const { auto r = static_cast<const Derived &>(*this); return r -= n; }

                constexpr ALWAYS_INLINE friend Derived  operator+(difference_type n, Derived it) { return it + n; }
                constexpr ALWAYS_INLINE difference_type operator-(Derived rhs) const { AMS_ASSERT(this->span == rhs.span); return this->index - rhs.index; }

                constexpr ALWAYS_INLINE reference operator[](difference_type n) const { return *(*this + n); }

                constexpr ALWAYS_INLINE friend bool operator==(Derived lhs, Derived rhs) {
                    return lhs.span == rhs.span && lhs.index == rhs.index;
                }

                constexpr ALWAYS_INLINE friend bool operator<(Derived lhs, Derived rhs) {
                    AMS_ASSERT(lhs.span == rhs.span);
                    return lhs.index < rhs.index;
                }

                constexpr ALWAYS_INLINE friend bool operator!=(Derived lhs, Derived rhs) { return !(lhs == rhs); }

                constexpr ALWAYS_INLINE friend bool operator>(Derived lhs, Derived rhs)  { return rhs < lhs; }

                constexpr ALWAYS_INLINE friend bool operator<=(Derived lhs, Derived rhs) { return !(lhs > rhs); }
                constexpr ALWAYS_INLINE friend bool operator>=(Derived lhs, Derived rhs) { return !(lhs < rhs); }
        };

        template<typename Span>
        class SpanIterator : public SpanIteratorImpl<Span, SpanIterator<Span>, typename Span::element_type&> {
            public:
                using SpanIteratorImpl<Span, SpanIterator<Span>, typename Span::element_type&>::SpanIteratorImpl;
        };

        template<typename Span>
        class SpanConstIterator : public SpanIteratorImpl<Span, SpanConstIterator<Span>, const typename Span::element_type&> {
            public:
                using SpanIteratorImpl<Span, SpanConstIterator<Span>, const typename Span::element_type&>::SpanIteratorImpl;

                constexpr ALWAYS_INLINE SpanConstIterator() = default;
                constexpr ALWAYS_INLINE SpanConstIterator(const SpanIterator<Span> &rhs) : SpanConstIterator(rhs.span, rhs.index) { /* ... */ }
        };

    }

    template<typename T>
    class Span {
        public:
            using element_type           = T;
            using value_type             = typename std::remove_cv<element_type>::type;
            using index_type             = std::ptrdiff_t;
            using difference_type        = std::ptrdiff_t;
            using pointer                = element_type *;
            using reference              = element_type &;
            using iterator               = ::ams::impl::SpanIterator<Span>;
            using const_iterator         = ::ams::impl::SpanConstIterator<Span>;
            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        private:
            T *ptr;
            index_type num_elements;
        public:
            constexpr ALWAYS_INLINE Span() : ptr(), num_elements() { /* ... */ }

            constexpr ALWAYS_INLINE Span(T *p, index_type size) : ptr(p), num_elements(size) {
                AMS_ASSERT(this->num_elements > 0 || this->ptr == nullptr);
            }

            constexpr ALWAYS_INLINE Span(T *start, T *end) : Span(start, end - start) { /* ... */ }

            template<size_t Size>
            constexpr ALWAYS_INLINE Span(T (&arr)[Size]) : Span(static_cast<T *>(arr), static_cast<index_type>(Size)) { /* ... */ }

            template<size_t Size>
            constexpr ALWAYS_INLINE Span(std::array<value_type, Size> &arr) : Span(arr.data(), static_cast<index_type>(Size)) { /* ... */ }

            template<size_t Size>
            constexpr ALWAYS_INLINE Span(const std::array<value_type, Size> &arr) : Span(arr.data(), static_cast<index_type>(Size)) { /* ... */ }

            template<typename U, typename = typename std::enable_if<std::is_convertible<U(*)[], T(*)[]>::value>::type>
            constexpr ALWAYS_INLINE Span(const Span<U> &rhs) : Span(rhs.data(), rhs.size()) { /* ... */ }
        public:
            constexpr ALWAYS_INLINE iterator begin() const { return { this, 0 }; }
            constexpr ALWAYS_INLINE iterator end()   const { return { this, this->num_elements }; }

            constexpr ALWAYS_INLINE const_iterator cbegin() const { return { this, 0 }; }
            constexpr ALWAYS_INLINE const_iterator cend()   const { return { this, this->num_elements }; }

            constexpr ALWAYS_INLINE reverse_iterator rbegin() const { return reverse_iterator(this->end()); }
            constexpr ALWAYS_INLINE reverse_iterator rend() const { return reverse_iterator(this->begin()); }

            constexpr ALWAYS_INLINE const_reverse_iterator crbegin() const { return reverse_iterator(this->cend()); }
            constexpr ALWAYS_INLINE const_reverse_iterator crend() const { return reverse_iterator(this->cbegin()); }

            constexpr ALWAYS_INLINE pointer data() const { return this->ptr; }

            constexpr ALWAYS_INLINE index_type size()       const { return this->num_elements; }
            constexpr ALWAYS_INLINE index_type size_bytes() const { return this->size() * sizeof(T); }

            constexpr ALWAYS_INLINE bool empty() const { return this->size() == 0; }

            constexpr ALWAYS_INLINE T &operator[](index_type idx) const {
                AMS_ASSERT(idx < this->size());
                return this->ptr[idx];
            }

            constexpr ALWAYS_INLINE T &operator()(index_type idx) const { return (*this)[idx]; }

            constexpr ALWAYS_INLINE Span first(index_type size) const {
                AMS_ASSERT(size <= this->size());
                return { this->ptr, size };
            }

            constexpr ALWAYS_INLINE Span last(index_type size) const {
                AMS_ASSERT(size <= this->size());
                return { this->ptr + (this->size() - size), size };
            }

            constexpr ALWAYS_INLINE Span subspan(index_type idx, index_type size) const {
                AMS_ASSERT(size <= this->size());
                AMS_ASSERT(this->size() - size >= idx);
                return { this->ptr + idx, size };
            }

            constexpr ALWAYS_INLINE Span subspan(index_type idx) const {
                AMS_ASSERT(idx <= this->size());
                return { this->ptr + idx, this->size() - idx };
            }
    };

    template<typename T>
    constexpr ALWAYS_INLINE Span<T> MakeSpan(T *start, T *end) {
        return { start, end };
    }

    template<typename T>
    constexpr ALWAYS_INLINE Span<T> MakeSpan(T *p, typename Span<T>::index_type size) {
        return { p, size };
    }

    template<typename T, size_t Size>
    constexpr ALWAYS_INLINE Span<T> MakeSpan(T (&arr)[Size]) {
        return Span<T>(arr);
    }

    template<typename T, size_t Size>
    constexpr ALWAYS_INLINE Span<T> MakeSpan(std::array<T, Size> &arr) {
        return Span<T>(arr);
    }

    template<typename T, size_t Size>
    constexpr ALWAYS_INLINE Span<const T> MakeSpan(const std::array<T, Size> &arr) {
        return Span<const T>(arr);
    }

}
