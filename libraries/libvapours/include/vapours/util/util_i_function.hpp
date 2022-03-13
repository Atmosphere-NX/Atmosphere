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

    template<typename T>
    class IFunction;

    namespace impl {

        template<typename>
        struct GetIFunctionTypeForObject;

        template<typename F, typename R, typename... Args>
        struct GetIFunctionTypeForObject<R (F::*)(Args...)> { using Type = R(Args...); };

        template<typename F, typename R, typename... Args>
        struct GetIFunctionTypeForObject<R (F::*)(Args...) const> { using Type = R(Args...); };

        template<typename>
        struct GetIFunctionType;

        template<typename R, typename... Args>
        struct GetIFunctionType<R(Args...)> { using Type = R(Args...); };

        template<typename R, typename... Args>
        struct GetIFunctionType<R(*)(Args...)> : public GetIFunctionType<R(Args...)>{};

        template<typename F>
        struct GetIFunctionType<std::reference_wrapper<F>> : public GetIFunctionType<F>{};

        template<typename F>
        struct GetIFunctionType : public GetIFunctionTypeForObject<decltype(&F::operator())>{};

        template<typename T, typename F, typename Enabled = void>
        class Function;

        template<typename R, typename... Args, typename F>
        class Function<R(Args...), F, typename std::enable_if<!(std::is_class<F>::value && !std::is_final<F>::value)>::type> final : public IFunction<R(Args...)> {
            private:
                F m_f;
            public:
                constexpr explicit Function(F f) : m_f(std::move(f)) { /* ... */}
                constexpr virtual ~Function() override { /* ... */ }

                constexpr virtual R operator()(Args... args) const override final {
                    return m_f(std::forward<Args>(args)...);
                }
        };

        template<typename R, typename... Args, typename F>
        class Function<R(Args...), F, typename std::enable_if<std::is_class<F>::value && !std::is_final<F>::value>::type> final : public IFunction<R(Args...)>, private F {
            public:
                constexpr explicit Function(F f) : F(std::move(f)) { /* ... */}
                constexpr virtual ~Function() override { /* ... */ }

                constexpr virtual R operator()(Args... args) const override final {
                    return static_cast<const F &>(*this).operator()(std::forward<Args>(args)...);
                }
        };

        template<typename I, typename F>
        constexpr ALWAYS_INLINE auto MakeIFunctionExplicitly(F f) {
            using FunctionType = ::ams::util::impl::Function<I, typename std::decay<F>::type>;
            return FunctionType{ std::move(f) };
        }

        template<typename I, typename T, typename R>
        constexpr ALWAYS_INLINE auto MakeIFunctionExplicitly(R T::*f) {
            return MakeIFunctionExplicitly<I>(std::mem_fn(f));
        }

    }

    template<typename R, typename... Args>
    class IFunction<R(Args...)> {
        protected:
            constexpr virtual ~IFunction() { /* ... */ };
        public:
            constexpr virtual R operator()(Args... args) const = 0;

            template<typename F>
            static constexpr ALWAYS_INLINE auto Make(F f) {
                return ::ams::util::impl::MakeIFunctionExplicitly<R(Args...)>(std::move(f));
            }
    };

    template<typename F, typename = typename std::enable_if<!std::is_member_pointer<F>::value>::type>
    constexpr ALWAYS_INLINE auto MakeIFunction(F f) {
        static_assert(!std::is_member_pointer<F>::value);

        return IFunction<typename ::ams::util::impl::GetIFunctionType<typename std::decay<F>::type>::Type>::Make(std::move(f));
    }

}