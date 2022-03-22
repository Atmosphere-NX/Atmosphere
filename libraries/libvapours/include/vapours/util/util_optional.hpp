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
#include <vapours/util/util_in_place.hpp>
#include <vapours/util/impl/util_enable_copy_move.hpp>

namespace ams::util {

    namespace impl {

        class NulloptHelper {
            public:
                template<typename T>
                static consteval T CreateInstance() {
                    return T(T::ConstructionArgument::Token);
                }
        };

    }

    struct nullopt_t {
        private:
            friend class impl::NulloptHelper;

            enum class ConstructionArgument {
                Token,
            };
        public:
            consteval nullopt_t(ConstructionArgument) { /* ... */ }
    };

    constexpr inline nullopt_t nullopt = impl::NulloptHelper::CreateInstance<nullopt_t>();

    namespace impl {

        template<typename T>
        struct OptionalPayloadBase {
            using StoredType = typename std::remove_const<T>::type;

            struct EmptyType{};

            template<typename U, bool = std::is_trivially_destructible<U>::value>
            union StorageType {
                EmptyType m_empty;
                U m_value;

                constexpr ALWAYS_INLINE StorageType() : m_empty() { /* ... */ }

                template<typename... Args>
                constexpr ALWAYS_INLINE StorageType(in_place_t, Args &&... args) : m_value(std::forward<Args>(args)...) { /* ... */ }

                template<typename V, typename... Args>
                constexpr ALWAYS_INLINE StorageType(std::initializer_list<V> il, Args &&... args) : m_value(il, std::forward<Args>(args)...) { /* ... */ }
            };

            template<typename U>
            union StorageType<U, false> {
                EmptyType m_empty;
                U m_value;

                constexpr ALWAYS_INLINE StorageType() : m_empty() { /* ... */ }

                template<typename... Args>
                constexpr ALWAYS_INLINE StorageType(in_place_t, Args &&... args) : m_value(std::forward<Args>(args)...) { /* ... */ }

                template<typename V, typename... Args>
                constexpr ALWAYS_INLINE StorageType(std::initializer_list<V> il, Args &&... args) : m_value(il, std::forward<Args>(args)...) { /* ... */ }

                constexpr ALWAYS_INLINE ~StorageType() { /* ... */ }
            };

            StorageType<StoredType> m_payload;
            bool m_engaged = false;

            constexpr OptionalPayloadBase() = default;
            constexpr ~OptionalPayloadBase() = default;

            template<typename... Args>
            constexpr OptionalPayloadBase(in_place_t tag, Args &&... args) : m_payload(tag, std::forward<Args>(args)...), m_engaged(true) { /* ... */ }

            template<typename U, typename... Args>
            constexpr OptionalPayloadBase(std::initializer_list<U> il, Args &&... args) : m_payload(il, std::forward<Args>(args)...), m_engaged(true) { /* ... */ }

            constexpr OptionalPayloadBase(bool engaged, const OptionalPayloadBase &rhs) { AMS_UNUSED(engaged); if (rhs.m_engaged) { this->Construct(rhs.Get()); } }
            constexpr OptionalPayloadBase(bool engaged, OptionalPayloadBase &&rhs) { AMS_UNUSED(engaged); if (rhs.m_engaged) { this->Construct(std::move(rhs.Get())); } }

            constexpr OptionalPayloadBase(const OptionalPayloadBase &) = default;
            constexpr OptionalPayloadBase(OptionalPayloadBase &&) = default;

            constexpr OptionalPayloadBase &operator=(const OptionalPayloadBase &) = default;
            constexpr OptionalPayloadBase &operator=(OptionalPayloadBase &&) = default;

            constexpr void CopyAssign(const OptionalPayloadBase &rhs) {
                if (m_engaged && rhs.m_engaged) {
                    this->Get() = rhs.Get();
                } else if (rhs.m_engaged) {
                    this->Construct(rhs.Get());
                } else {
                    this->Reset();
                }
            }

            constexpr void MoveAssign(OptionalPayloadBase &&rhs) {
                if (m_engaged && rhs.m_engaged) {
                    this->Get() = std::move(rhs.Get());
                } else if (rhs.m_engaged) {
                    this->Construct(std::move(rhs.Get()));
                } else {
                    this->Reset();
                }
            }

            template<typename... Args>
            constexpr void Construct(Args &&... args) {
                std::construct_at(std::addressof(m_payload.m_value), std::forward<Args>(args)...);
                m_engaged = true;
            }

            constexpr void Destroy() {
                m_engaged = false;
                std::destroy_at(std::addressof(m_payload.m_value));
            }

            constexpr ALWAYS_INLINE       T &Get()       { return m_payload.m_value; }
            constexpr ALWAYS_INLINE const T &Get() const { return m_payload.m_value; }

            constexpr void Reset() {
                if (m_engaged) {
                    this->Destroy();
                }
            }
        };

        template<typename T, bool = std::is_trivially_destructible<T>::value, bool = std::is_trivially_copy_assignable<T>::value && std::is_trivially_copy_constructible<T>::value, bool = std::is_trivially_move_assignable<T>::value && std::is_trivially_move_constructible<T>::value>
        struct OptionalPayload;

        template<typename T>
        struct OptionalPayload<T, true, true, true> : OptionalPayloadBase<T> {
            using OptionalPayloadBase<T>::OptionalPayloadBase;

            constexpr OptionalPayload() = default;
        };

        template<typename T>
        struct OptionalPayload<T, true, false, true> : OptionalPayloadBase<T> {
            using OptionalPayloadBase<T>::OptionalPayloadBase;

            constexpr OptionalPayload() = default;
            constexpr ~OptionalPayload() = default;
            constexpr OptionalPayload(const OptionalPayload &) = default;
            constexpr OptionalPayload(OptionalPayload &&) = default;
            constexpr OptionalPayload& operator=(OptionalPayload &&) = default;

            constexpr OptionalPayload &operator=(const OptionalPayload &rhs) {
                this->CopyAssign(rhs);
                return *this;
            }
        };

        template<typename T>
        struct OptionalPayload<T, true, true, false> : OptionalPayloadBase<T> {
            using OptionalPayloadBase<T>::OptionalPayloadBase;

            constexpr OptionalPayload() = default;
            constexpr ~OptionalPayload() = default;
            constexpr OptionalPayload(const OptionalPayload &) = default;
            constexpr OptionalPayload(OptionalPayload &&) = default;
            constexpr OptionalPayload& operator=(const OptionalPayload &) = default;

            constexpr OptionalPayload &operator=(OptionalPayload &&rhs) {
                this->MoveAssign(std::move(rhs));
                return *this;
            }
        };

        template<typename T>
        struct OptionalPayload<T, true, false, false> : OptionalPayloadBase<T> {
            using OptionalPayloadBase<T>::OptionalPayloadBase;

            constexpr OptionalPayload() = default;
            constexpr ~OptionalPayload() = default;
            constexpr OptionalPayload(const OptionalPayload &) = default;
            constexpr OptionalPayload(OptionalPayload &&) = default;

            constexpr OptionalPayload &operator=(const OptionalPayload &rhs) {
                this->CopyAssign(rhs);
                return *this;
            }

            constexpr OptionalPayload &operator=(OptionalPayload &&rhs) {
                this->MoveAssign(std::move(rhs));
                return *this;
            }
        };

        template<typename T, bool TrivialCopy, bool TrivialMove>
        struct OptionalPayload<T, false, TrivialCopy, TrivialMove> : OptionalPayload<T, true, TrivialCopy, TrivialMove> {
            using OptionalPayload<T, true, TrivialCopy, TrivialMove>::OptionalPayload;

            constexpr OptionalPayload() = default;
            constexpr OptionalPayload(const OptionalPayload &) = default;
            constexpr OptionalPayload(OptionalPayload &&) = default;
            constexpr OptionalPayload& operator=(const OptionalPayload &) = default;
            constexpr OptionalPayload& operator=(OptionalPayload &&) = default;

            constexpr ~OptionalPayload() { this->Reset(); }
        };

        template<typename T, typename Derived>
        class OptionalBaseImpl {
            protected:
                using StoredType = std::remove_const_t<T>;

                template<typename... Args>
                constexpr void ConstructImpl(Args &&... args) { static_cast<Derived *>(this)->m_payload.Construct(std::forward<Args>(args)...); }

                constexpr void DestructImpl() { static_cast<Derived *>(this)->m_payload.Destroy(); }

                constexpr void ResetImpl() { static_cast<Derived *>(this)->m_payload.Reset(); }

                constexpr ALWAYS_INLINE bool IsEngagedImpl() const { return static_cast<const Derived *>(this)->m_payload.m_engaged; }

                constexpr ALWAYS_INLINE       T &GetImpl()       { return static_cast<Derived *>(this)->m_payload.Get(); }
                constexpr ALWAYS_INLINE const T &GetImpl() const { return static_cast<const Derived *>(this)->m_payload.Get(); }
        };

        template<typename T, bool = std::is_trivially_copy_constructible<T>::value, bool = std::is_trivially_move_constructible<T>::value>
        struct OptionalBase : OptionalBaseImpl<T, OptionalBase<T>> {
            OptionalPayload<T> m_payload;

            constexpr OptionalBase() = default;

            template<typename... Args, std::enable_if_t<std::is_constructible<T, Args...>::value, bool> = false>
            constexpr explicit OptionalBase(in_place_t, Args &&... args) : m_payload(in_place, std::forward<Args>(args)...) { /* ... */ }

            template<typename U, typename... Args, std::enable_if_t<std::is_constructible<T, std::initializer_list<U> &, Args...>::value, bool> = false>
            constexpr explicit OptionalBase(in_place_t, std::initializer_list<U> il, Args &&... args) : m_payload(in_place, il, std::forward<Args>(args)...) { /* ... */ }

            constexpr OptionalBase(const OptionalBase &rhs) : m_payload(rhs.m_payload.m_engaged, rhs.m_payload) { /* ... */ }
            constexpr OptionalBase(OptionalBase &&rhs) : m_payload(rhs.m_payload.m_engaged, std::move(rhs.m_payload)) { /* ... */ }

            constexpr OptionalBase &operator=(const OptionalBase &) = default;
            constexpr OptionalBase &operator=(OptionalBase &&) = default;
        };

        template<typename T>
        struct OptionalBase<T, false, true> : OptionalBaseImpl<T, OptionalBase<T>> {
            OptionalPayload<T> m_payload;

            constexpr OptionalBase() = default;

            template<typename... Args, std::enable_if_t<std::is_constructible<T, Args...>::value, bool> = false>
            constexpr explicit OptionalBase(in_place_t, Args &&... args) : m_payload(in_place, std::forward<Args>(args)...) { /* ... */ }

            template<typename U, typename... Args, std::enable_if_t<std::is_constructible<T, std::initializer_list<U> &, Args...>::value, bool> = false>
            constexpr explicit OptionalBase(in_place_t, std::initializer_list<U> il, Args &&... args) : m_payload(in_place, il, std::forward<Args>(args)...) { /* ... */ }

            constexpr OptionalBase(const OptionalBase &rhs) : m_payload(rhs.m_payload.m_engaged, rhs.m_payload) { /* ... */ }
            constexpr OptionalBase(OptionalBase &&rhs) = default;

            constexpr OptionalBase &operator=(const OptionalBase &) = default;
            constexpr OptionalBase &operator=(OptionalBase &&) = default;
        };

        template<typename T>
        struct OptionalBase<T, true, false> : OptionalBaseImpl<T, OptionalBase<T>> {
            OptionalPayload<T> m_payload;

            constexpr OptionalBase() = default;

            template<typename... Args, std::enable_if_t<std::is_constructible<T, Args...>::value, bool> = false>
            constexpr explicit OptionalBase(in_place_t, Args &&... args) : m_payload(in_place, std::forward<Args>(args)...) { /* ... */ }

            template<typename U, typename... Args, std::enable_if_t<std::is_constructible<T, std::initializer_list<U> &, Args...>::value, bool> = false>
            constexpr explicit OptionalBase(in_place_t, std::initializer_list<U> il, Args &&... args) : m_payload(in_place, il, std::forward<Args>(args)...) { /* ... */ }

            constexpr OptionalBase(const OptionalBase &rhs) = default;
            constexpr OptionalBase(OptionalBase &&rhs) : m_payload(rhs.m_payload.m_engaged, std::move(rhs.m_payload)) { /* ... */ }

            constexpr OptionalBase &operator=(const OptionalBase &) = default;
            constexpr OptionalBase &operator=(OptionalBase &&) = default;
        };

        template<typename T>
        struct OptionalBase<T, true, true> : OptionalBaseImpl<T, OptionalBase<T>> {
            OptionalPayload<T> m_payload;

            constexpr OptionalBase() = default;

            template<typename... Args, std::enable_if_t<std::is_constructible<T, Args...>::value, bool> = false>
            constexpr explicit OptionalBase(in_place_t, Args &&... args) : m_payload(in_place, std::forward<Args>(args)...) { /* ... */ }

            template<typename U, typename... Args, std::enable_if_t<std::is_constructible<T, std::initializer_list<U> &, Args...>::value, bool> = false>
            constexpr explicit OptionalBase(in_place_t, std::initializer_list<U> il, Args &&... args) : m_payload(in_place, il, std::forward<Args>(args)...) { /* ... */ }

            constexpr OptionalBase(const OptionalBase &rhs) = default;
            constexpr OptionalBase(OptionalBase &&rhs) = default;

            constexpr OptionalBase &operator=(const OptionalBase &) = default;
            constexpr OptionalBase &operator=(OptionalBase &&) = default;
        };

    }

    template<typename T>
    class optional;

    namespace impl {

        template<typename T, typename U>
        constexpr inline bool ConvertsFromOptional = std::is_constructible<T, const optional<U> &>::value ||
                                                     std::is_constructible<T, optional<U> &>::value ||
                                                     std::is_constructible<T, const optional<U> &&>::value ||
                                                     std::is_constructible<T, optional<U> &&>::value ||
                                                     std::is_convertible<const optional<U> &, T>::value ||
                                                     std::is_convertible<optional<U> &, T>::value ||
                                                     std::is_convertible<const optional<U> &&, T>::value ||
                                                     std::is_convertible<optional<U> &&, T>::value;

        template<typename T, typename U>
        constexpr inline bool AssignsFromOptional = std::is_assignable<T &, const optional<U> &>::value ||
                                                    std::is_assignable<T &, optional<U> &>::value ||
                                                    std::is_assignable<T &, const optional<U> &&>::value ||
                                                    std::is_assignable<T &, optional<U> &&>::value;

    }

    template<typename T>
    class optional : private impl::OptionalBase<T>, private impl::EnableCopyMove<std::is_copy_constructible<T>::value, std::is_copy_constructible<T>::value && std::is_copy_assignable<T>::value, std::is_move_constructible<T>::value, std::is_move_constructible<T>::value && std::is_move_assignable<T>::value, optional<T>> {
        static_assert(!std::is_same<std::remove_cv_t<T>, ::ams::util::nullopt_t>::value);
        static_assert(!std::is_same<std::remove_cv_t<T>, ::ams::util::in_place_t>::value);
        static_assert(!std::is_reference<T>::value);
        private:
            using Base = impl::OptionalBase<T>;

            template<typename U> static constexpr inline bool IsNotSelf = !std::is_same<optional, std::remove_cvref_t<U>>::value;
            template<typename U> static constexpr inline bool IsNotTag  = !std::is_same<::ams::util::in_place_t, std::remove_cvref_t<U>>::value && !std::is_same<::std::in_place_t, std::remove_cvref_t<U>>::value;

            template<bool... Cond>
            using Requires = std::enable_if_t<(Cond && ...), bool>;
        public:
            using value_type = T;
        public:
            constexpr optional() { /* ... */ }
            constexpr optional(nullopt_t) { /* ... */ }

            template<typename U = T, Requires<IsNotSelf<U>, IsNotTag<U>, std::is_constructible<T, U>::value, std::is_convertible<U, T>::value> = true>
            constexpr optional(U &&u) : Base(::ams::util::in_place, std::forward<U>(u)) { /* ... */ }

            template<typename U = T, Requires<IsNotSelf<U>, IsNotTag<U>, std::is_constructible<T, U>::value, !std::is_convertible<U, T>::value> = false>
            constexpr explicit optional(U &&u) : Base(::ams::util::in_place, std::forward<U>(u)) { /* ... */ }

            template<typename U, Requires<!std::is_same<T, U>::value, std::is_constructible<T, const U &>::value, std::is_convertible<const U &, T>::value, !impl::ConvertsFromOptional<T, U>> = true>
            constexpr optional(const optional<U> &u) {
                if (u) {
                    this->emplace(*u);
                }
            }

            template<typename U, Requires<!std::is_same<T, U>::value, std::is_constructible<T, const U &>::value, !std::is_convertible<const U &, T>::value, !impl::ConvertsFromOptional<T, U>> = false>
            constexpr explicit optional(const optional<U> &u) {
                if (u) {
                    this->emplace(*u);
                }
            }

            template<typename U, Requires<!std::is_same<T, U>::value, std::is_constructible<T, const U &>::value, std::is_convertible<const U &, T>::value, !impl::ConvertsFromOptional<T, U>> = true>
            constexpr optional(optional<U> &&u) {
                if (u) {
                    this->emplace(std::move(*u));
                }
            }

            template<typename U, Requires<!std::is_same<T, U>::value, std::is_constructible<T, const U &>::value, !std::is_convertible<const U &, T>::value, !impl::ConvertsFromOptional<T, U>> = false>
            constexpr explicit optional(optional<U> &&u) {
                if (u) {
                    this->emplace(std::move(*u));
                }
            }

            template<typename... Args, Requires<std::is_constructible<T, Args...>::value> = false>
            constexpr explicit optional(in_place_t, Args &&... args) : Base(::ams::util::in_place, std::forward<Args>(args)...) { /* ... */ }

            template<typename U, typename... Args, Requires<std::is_constructible<T, std::initializer_list<U> &, Args...>::value> = false>
            constexpr explicit optional(in_place_t, std::initializer_list<U> il, Args &&... args) : Base(::ams::util::in_place, il, std::forward<Args>(args)...) { /* ... */ }

            constexpr optional &operator=(nullopt_t) { this->ResetImpl(); return *this; }

            template<typename U = T>
            constexpr std::enable_if_t<IsNotSelf<U> && !(std::is_scalar<T>::value && std::is_same<T, std::decay_t<U>>::value) && std::is_constructible<T, U>::value && std::is_assignable<T &, U>::value,
                                       optional &>
            operator =(U &&u) {
                if (this->IsEngagedImpl()) {
                    this->GetImpl() = std::forward<U>(u);
                } else {
                    this->ConstructImpl(std::forward<U>(u));
                }

                return *this;
            }

            template<typename U>
            constexpr std::enable_if_t<!std::is_same<T, U>::value && std::is_constructible<T, const U &>::value && std::is_assignable<T &, const U &>::value && !impl::ConvertsFromOptional<T, U> && !impl::AssignsFromOptional<T, U>,
                                       optional &>
            operator =(const optional<U> &u) {
                if (u) {
                    if (this->IsEngagedImpl()) {
                        this->GetImpl() = *u;
                    } else {
                        this->ConstructImpl(*u);
                    }
                } else {
                    this->ResetImpl();
                }

                return *this;
            }

            template<typename U>
            constexpr std::enable_if_t<!std::is_same<T, U>::value && std::is_constructible<T, U>::value && std::is_assignable<T &, U>::value && !impl::ConvertsFromOptional<T, U> && !impl::AssignsFromOptional<T, U>,
                                       optional &>
            operator =(optional<U> &&u) {
                if (u) {
                    if (this->IsEngagedImpl()) {
                        this->GetImpl() = std::move(*u);
                    } else {
                        this->ConstructImpl(std::move(*u));
                    }
                } else {
                    this->ResetImpl();
                }

                return *this;
            }

            template<typename... Args>
            constexpr std::enable_if_t<std::is_constructible<T, Args...>::value, T &> emplace(Args &&... args) {
                this->ResetImpl();
                this->ConstructImpl(std::forward<Args>(args)...);
                return this->GetImpl();
            }

            template<typename U, typename... Args>
            constexpr std::enable_if_t<std::is_constructible<T, std::initializer_list<U> &, Args...>::value, T &> emplace(std::initializer_list<U> il, Args &&... args) {
                this->ResetImpl();
                this->ConstructImpl(il, std::forward<Args>(args)...);
                return this->GetImpl();
            }

            constexpr void swap(optional &rhs) {
                if (this->IsEngagedImpl() && rhs.IsEngagedImpl()) {
                    std::swap(this->GetImpl(), rhs.GetImpl());
                } else if (this->IsEngagedImpl()) {
                    rhs.ConstructImpl(std::move(this->GetImpl()));
                    this->DestructImpl();
                } else if (rhs.IsEngagedImpl()) {
                    this->ConstructImpl(std::move(rhs.GetImpl()));
                    rhs.DestructImpl();
                }
            }

            constexpr ALWAYS_INLINE const T *operator ->() const { return std::addressof(this->GetImpl()); }
            constexpr ALWAYS_INLINE       T *operator ->()       { return std::addressof(this->GetImpl()); }

            constexpr ALWAYS_INLINE const T &operator *() const & { return this->GetImpl(); }
            constexpr ALWAYS_INLINE       T &operator *()       & { return this->GetImpl(); }

            constexpr ALWAYS_INLINE const T &&operator *() const && { return std::move(this->GetImpl()); }
            constexpr ALWAYS_INLINE       T &&operator *()       && { return std::move(this->GetImpl()); }

            constexpr ALWAYS_INLINE explicit operator bool() const { return this->IsEngagedImpl(); }
            constexpr ALWAYS_INLINE bool has_value() const { return this->IsEngagedImpl(); }

            constexpr ALWAYS_INLINE const T &value() const & { /* AMS_ASSERT(this->IsEngagedImpl()); */ return this->GetImpl(); }
            constexpr ALWAYS_INLINE       T &value()       & { /* AMS_ASSERT(this->IsEngagedImpl()); */ return this->GetImpl(); }

            constexpr ALWAYS_INLINE const T &&value() const && { /* AMS_ASSERT(this->IsEngagedImpl()); */ return std::move(this->GetImpl()); }
            constexpr ALWAYS_INLINE       T &&value()       && { /* AMS_ASSERT(this->IsEngagedImpl()); */ return std::move(this->GetImpl()); }

            template<typename U>
            constexpr T value_or(U &&u) const & {
                static_assert(std::is_copy_constructible<T>::value);
                static_assert(std::is_convertible<U &&, T>::value);

                return this->IsEngagedImpl() ? this->GetImpl() : static_cast<T>(std::forward<U>(u));
            }

            template<typename U>
            constexpr T value_or(U &&u) && {
                static_assert(std::is_move_constructible<T>::value);
                static_assert(std::is_convertible<U &&, T>::value);

                return this->IsEngagedImpl() ? std::move(this->GetImpl()) : static_cast<T>(std::forward<U>(u));
            }

            constexpr void reset() { this->ResetImpl(); }
    };

    namespace impl {

        template<typename T> using optional_relop_t = std::enable_if_t<std::is_convertible<T, bool>::value, bool>;

        template<typename T, typename U> using optional_eq_t = optional_relop_t<decltype(std::declval<const T &>() == std::declval<const U &>())>;
        template<typename T, typename U> using optional_ne_t = optional_relop_t<decltype(std::declval<const T &>() != std::declval<const U &>())>;
        template<typename T, typename U> using optional_le_t = optional_relop_t<decltype(std::declval<const T &>() <= std::declval<const U &>())>;
        template<typename T, typename U> using optional_ge_t = optional_relop_t<decltype(std::declval<const T &>() >= std::declval<const U &>())>;
        template<typename T, typename U> using optional_lt_t = optional_relop_t<decltype(std::declval<const T &>() <  std::declval<const U &>())>;
        template<typename T, typename U> using optional_gt_t = optional_relop_t<decltype(std::declval<const T &>() >  std::declval<const U &>())>;

    }

    template<typename T, typename U>
    constexpr inline impl::optional_eq_t<T, U> operator==(const optional<T> &lhs, const optional<U> &rhs) { return static_cast<bool>(lhs) == static_cast<bool>(rhs) && (!lhs || *lhs == *rhs); }

    template<typename T, typename U>
    constexpr inline impl::optional_ne_t<T, U> operator!=(const optional<T> &lhs, const optional<U> &rhs) { return static_cast<bool>(lhs) != static_cast<bool>(rhs) || (static_cast<bool>(lhs) && *lhs != *rhs); }

    template<typename T, typename U>
    constexpr inline impl::optional_lt_t<T, U> operator< (const optional<T> &lhs, const optional<U> &rhs) { return static_cast<bool>(rhs) && (!lhs || *lhs < *rhs); }

    template<typename T, typename U>
    constexpr inline impl::optional_gt_t<T, U> operator> (const optional<T> &lhs, const optional<U> &rhs) { return static_cast<bool>(lhs) && (!rhs || *lhs > *rhs); }

    template<typename T, typename U>
    constexpr inline impl::optional_le_t<T, U> operator<=(const optional<T> &lhs, const optional<U> &rhs) { return !lhs || (static_cast<bool>(rhs) && *lhs <= *rhs); }

    template<typename T, typename U>
    constexpr inline impl::optional_ge_t<T, U> operator>=(const optional<T> &lhs, const optional<U> &rhs) { return !rhs || (static_cast<bool>(lhs) && *lhs >= *rhs); }

    template<typename T, std::three_way_comparable_with<T> U>
    constexpr inline std::compare_three_way_result_t<T, U> operator <=>(const optional<T> &lhs, const optional<U> &rhs) {
        return (lhs && rhs) ? *lhs <=> *rhs : static_cast<bool>(lhs) <=> static_cast<bool>(rhs);
    }

    template<typename T> constexpr inline bool operator==(const optional<T> &lhs, nullopt_t) { return !lhs; }

    template<typename T> constexpr inline std::strong_ordering operator<=>(const optional<T> &lhs, nullopt_t) { return static_cast<bool>(lhs) <=> false; }

    template<typename T, typename U>
    constexpr inline impl::optional_eq_t<T, U> operator==(const optional<T> &lhs, const U &rhs) { return lhs && *lhs == rhs; }

    template<typename T, typename U>
    constexpr inline impl::optional_eq_t<U, T> operator==(const U &lhs, const optional<T> &rhs) { return rhs && lhs == *rhs; }

    template<typename T, typename U>
    constexpr inline impl::optional_ne_t<T, U> operator!=(const optional<T> &lhs, const U &rhs) { return !lhs || *lhs != rhs; }

    template<typename T, typename U>
    constexpr inline impl::optional_ne_t<U, T> operator!=(const U &lhs, const optional<T> &rhs) { return !rhs || lhs != *rhs; }

    template<typename T, typename U>
    constexpr inline impl::optional_lt_t<T, U> operator< (const optional<T> &lhs, const U &rhs) { return !lhs || *lhs < rhs; }

    template<typename T, typename U>
    constexpr inline impl::optional_lt_t<U, T> operator< (const U &lhs, const optional<T> &rhs) { return rhs && lhs < *rhs; }

    template<typename T, typename U>
    constexpr inline impl::optional_gt_t<T, U> operator> (const optional<T> &lhs, const U &rhs) { return lhs && *lhs > rhs; }

    template<typename T, typename U>
    constexpr inline impl::optional_gt_t<U, T> operator> (const U &lhs, const optional<T> &rhs) { return !rhs || lhs > *rhs; }

    template<typename T, typename U>
    constexpr inline impl::optional_le_t<T, U> operator<=(const optional<T> &lhs, const U &rhs) { return !lhs || *lhs <= rhs; }

    template<typename T, typename U>
    constexpr inline impl::optional_le_t<U, T> operator<=(const U &lhs, const optional<T> &rhs) { return rhs && lhs <= *rhs; }

    template<typename T, typename U>
    constexpr inline impl::optional_ge_t<T, U> operator>=(const optional<T> &lhs, const U &rhs) { return lhs && *lhs >= rhs; }

    template<typename T, typename U>
    constexpr inline impl::optional_ge_t<U, T> operator>=(const U &lhs, const optional<T> &rhs) { return !rhs || lhs >= *rhs; }

    namespace impl {

        template<typename T>
        constexpr inline bool IsOptional = false;

        template<typename T>
        constexpr inline bool IsOptional<optional<T>> = true;

    }

    template<typename T, typename U> requires (!impl::IsOptional<U>) && std::three_way_comparable_with<T, U>
    constexpr inline std::compare_three_way_result_t<T, U> operator<=>(const optional<T> &lhs, const U &rhs) {
        return static_cast<bool>(lhs) ? *lhs <=> rhs : std::strong_ordering::less;
    }

    template<typename T>
    constexpr inline std::enable_if_t<std::is_constructible<std::decay_t<T>, T>::value, optional<std::decay_t<T>>> make_optional(T && t) { return optional<std::decay_t<T>>{ std::forward<T>(t) }; }

    template<typename T, typename... Args>
    constexpr inline std::enable_if_t<std::is_constructible<T, Args...>::value, optional<T>> make_optional(Args &&... args) { return optional<T>{ ::ams::util::in_place, std::forward<Args>(args)... }; }

    template<typename T, typename U, typename... Args>
    constexpr inline std::enable_if_t<std::is_constructible<T, std::initializer_list<U> &, Args...>::value, optional<T>> make_optional(std::initializer_list<U> il, Args &&... args) { return optional<T>{ ::ams::util::in_place, il, std::forward<Args>(args)... }; }

    template<typename T> optional(T) -> optional<T>;

}

namespace std {

    template<typename T>
    constexpr inline enable_if_t<is_move_constructible_v<T> && is_swappable_v<T>> swap(::ams::util::optional<T> &lhs, ::ams::util::optional<T> &rhs) noexcept {
        lhs.swap(rhs);
    }

    template<typename T>
    constexpr inline enable_if_t<!(is_move_constructible_v<T> && is_swappable_v<T>)> swap(::ams::util::optional<T> &lhs, ::ams::util::optional<T> &rhs) = delete;

}
