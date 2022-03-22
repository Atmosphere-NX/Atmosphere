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
#include <mesosphere/svc/kern_svc_results.hpp>
#include <mesosphere/kern_select_userspace_memory_access.hpp>

namespace ams::kern::svc {

    namespace impl {

        template<typename T>
        concept Pointer = std::is_pointer<T>::value;

        template<typename T>
        concept NonConstPointer   = Pointer<T> && !std::is_const<typename std::remove_pointer<T>::type>::value;

        template<typename T>
        concept ConstPointer      = Pointer<T> && std::is_const<typename std::remove_pointer<T>::type>::value;

        template<typename T, size_t N>
        concept AlignedNPointer   = Pointer<T> && alignof(typename std::remove_pointer<T>::type) >= N && util::IsAligned(sizeof(typename std::remove_pointer<T>::type), N);

        template<typename T>
        concept Aligned8Pointer   = AlignedNPointer<T, sizeof(u8)>;

        template<typename T>
        concept Aligned16Pointer  = AlignedNPointer<T, sizeof(u16)> && Aligned8Pointer<T>;

        template<typename T>
        concept Aligned32Pointer  = AlignedNPointer<T, sizeof(u32)> && Aligned16Pointer<T>;

        template<typename T>
        concept Aligned64Pointer  = AlignedNPointer<T, sizeof(u64)> && Aligned32Pointer<T>;

        template<typename _T>
        class KUserPointerImplTraits;

        template<typename _T> requires Aligned8Pointer<_T>
        class KUserPointerImplTraits<_T> {
            public:
                using T = typename std::remove_const<typename std::remove_pointer<_T>::type>::type;
            public:
                static ALWAYS_INLINE Result CopyFromUserspace(void *dst, const void *src, size_t size) {
                    R_UNLESS(UserspaceAccess::CopyMemoryFromUser(dst, src, size), svc::ResultInvalidPointer());
                    R_SUCCEED();
                }

                static ALWAYS_INLINE Result CopyToUserspace(void *dst, const void *src, size_t size) {
                    R_UNLESS(UserspaceAccess::CopyMemoryToUser(dst, src, size),   svc::ResultInvalidPointer());
                    R_SUCCEED();
                }
        };

        template<typename _T> requires Aligned32Pointer<_T>
        class KUserPointerImplTraits<_T> {
            public:
                using T = typename std::remove_const<typename std::remove_pointer<_T>::type>::type;
            public:
                static ALWAYS_INLINE Result CopyFromUserspace(void *dst, const void *src, size_t size) {
                    R_UNLESS(UserspaceAccess::CopyMemoryFromUserAligned32Bit(dst, src, size), svc::ResultInvalidPointer());
                    R_SUCCEED();
                }

                static ALWAYS_INLINE Result CopyToUserspace(void *dst, const void *src, size_t size) {
                    R_UNLESS(UserspaceAccess::CopyMemoryToUserAligned32Bit(dst, src, size),   svc::ResultInvalidPointer());
                    R_SUCCEED();
                }
        };

        template<typename _T> requires Aligned64Pointer<_T>
        class KUserPointerImplTraits<_T> {
            public:
                using T = typename std::remove_const<typename std::remove_pointer<_T>::type>::type;
            public:
                static ALWAYS_INLINE Result CopyFromUserspace(void *dst, const void *src, size_t size) {
                    R_UNLESS(UserspaceAccess::CopyMemoryFromUserAligned64Bit(dst, src, size), svc::ResultInvalidPointer());
                    R_SUCCEED();
                }

                static ALWAYS_INLINE Result CopyToUserspace(void *dst, const void *src, size_t size) {
                    R_UNLESS(UserspaceAccess::CopyMemoryToUserAligned64Bit(dst, src, size),   svc::ResultInvalidPointer());
                    R_SUCCEED();
                }
        };

        template<typename _T>
        class KUserPointerImpl;

        template<typename _T> requires Aligned8Pointer<_T>
        class KUserPointerImpl<_T> : impl::KUserPointerTag {
            private:
                using Traits = KUserPointerImplTraits<_T>;
            protected:
                using CT = typename std::remove_pointer<_T>::type;
                using T  = typename std::remove_const<CT>::type;
            private:
                CT *m_ptr;
            private:
                ALWAYS_INLINE Result CopyToImpl(void *p, size_t size) const {
                    R_RETURN(Traits::CopyFromUserspace(p, m_ptr, size));
                }

                ALWAYS_INLINE Result CopyFromImpl(const void *p, size_t size) const {
                    R_RETURN(Traits::CopyToUserspace(m_ptr, p, size));
                }
            protected:
                ALWAYS_INLINE Result CopyTo(T *p)         const { R_RETURN(this->CopyToImpl(p, sizeof(*p))); }
                ALWAYS_INLINE Result CopyFrom(const T *p) const { R_RETURN(this->CopyFromImpl(p, sizeof(*p))); }

                ALWAYS_INLINE Result CopyArrayElementTo(T *p, size_t index)         const { R_RETURN(Traits::CopyFromUserspace(p, m_ptr + index, sizeof(*p))); }
                ALWAYS_INLINE Result CopyArrayElementFrom(const T *p, size_t index) const { R_RETURN(Traits::CopyToUserspace(m_ptr + index, p, sizeof(*p))); }

                ALWAYS_INLINE Result CopyArrayTo(T *arr, size_t count)         const { R_RETURN(this->CopyToImpl(arr, sizeof(*arr) * count)); }
                ALWAYS_INLINE Result CopyArrayFrom(const T *arr, size_t count) const { R_RETURN(this->CopyFromImpl(arr, sizeof(*arr) * count)); }

                constexpr ALWAYS_INLINE bool IsNull() const { return m_ptr == nullptr; }

                constexpr ALWAYS_INLINE CT *GetUnsafePointer() const { return m_ptr; }
        };

        template<>
        class KUserPointerImpl<const char *> : impl::KUserPointerTag {
            private:
                using Traits = KUserPointerImplTraits<const char *>;
            protected:
                using CT = const char;
                using T  = char;
            private:
                const char *m_ptr;
            protected:
                ALWAYS_INLINE Result CopyStringTo(char *dst, size_t size) const {
                    static_assert(sizeof(char) == 1);
                    R_UNLESS(UserspaceAccess::CopyStringFromUser(dst, m_ptr, size) > 0, svc::ResultInvalidPointer());
                    R_SUCCEED();
                }

                ALWAYS_INLINE Result CopyArrayElementTo(char *dst, size_t index) const {
                    R_RETURN(Traits::CopyFromUserspace(dst, m_ptr + index, sizeof(*dst)));
                }

                constexpr ALWAYS_INLINE bool IsNull() const { return m_ptr == nullptr; }

                constexpr ALWAYS_INLINE const char *GetUnsafePointer() const { return m_ptr; }
        };

    }

    template<typename T>
    struct KUserPointer;

    template<typename T> requires impl::ConstPointer<T>
    struct KUserPointer<T> : public impl::KUserPointerImpl<T> {
        public:
            static constexpr bool IsInput = true;
        public:
            using impl::KUserPointerImpl<T>::CopyTo;
            using impl::KUserPointerImpl<T>::CopyArrayElementTo;
            using impl::KUserPointerImpl<T>::CopyArrayTo;
            using impl::KUserPointerImpl<T>::IsNull;

            using impl::KUserPointerImpl<T>::GetUnsafePointer;
    };

    template<typename T> requires impl::NonConstPointer<T>
    struct KUserPointer<T> : public impl::KUserPointerImpl<T> {
        public:
            static constexpr bool IsInput = false;
        public:
            using impl::KUserPointerImpl<T>::CopyFrom;
            using impl::KUserPointerImpl<T>::CopyArrayElementFrom;
            using impl::KUserPointerImpl<T>::CopyArrayFrom;
            using impl::KUserPointerImpl<T>::IsNull;

            using impl::KUserPointerImpl<T>::GetUnsafePointer;
    };

    template<>
    struct KUserPointer<const char *> : public impl::KUserPointerImpl<const char *> {
        public:
            static constexpr bool IsInput = true;
        public:
            using impl::KUserPointerImpl<const char *>::CopyStringTo;
            using impl::KUserPointerImpl<const char *>::CopyArrayElementTo;
            using impl::KUserPointerImpl<const char *>::IsNull;

            using impl::KUserPointerImpl<const char *>::GetUnsafePointer;
    };

}
