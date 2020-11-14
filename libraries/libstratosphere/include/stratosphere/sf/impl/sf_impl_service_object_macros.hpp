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

namespace ams::sf::impl {

    template<typename T>
    concept ServiceCommandResult = std::same_as<::ams::Result, T> || std::same_as<void, T>;

    template<auto F, typename ...Arguments>
    concept Invokable = requires (Arguments &&... args) {
        { F(std::forward<Arguments>(args)...) };
    };

    struct FunctionTraits {
        public:
            template<typename R, typename... A>
            static std::tuple<A...> GetArgsImpl(R(A...));
    };

    template<auto F>
    using FunctionArgsType = decltype(FunctionTraits::GetArgsImpl(F));

    template<typename Class, typename Return, typename ArgsTuple>
    struct ClassFunctionPointerHelper;

    template<typename Class, typename Return, typename... Arguments>
    struct ClassFunctionPointerHelper<Class, Return, std::tuple<Arguments...>> {
        using Type = Return (*)(Class *, Arguments &&...);
    };

    template<typename Class, typename Return, typename ArgsTuple>
    using ClassFunctionPointer = typename ClassFunctionPointerHelper<Class, Return, ArgsTuple>::Type;

    template<typename T>
    struct TypeTag{};

    #define AMS_SF_IMPL_HELPER_FUNCTION_NAME_IMPL(CLASSNAME, FUNCNAME, SUFFIX) \
        __ams_sf_impl_helper_##CLASSNAME##_##FUNCNAME##_##SUFFIX

    #define AMS_SF_IMPL_HELPER_FUNCTION_NAME(CLASSNAME, FUNCNAME) \
        AMS_SF_IMPL_HELPER_FUNCTION_NAME_IMPL(CLASSNAME, FUNCNAME, intf)

    #define AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, FUNCNAME) \
        ::ams::sf::impl::FunctionArgsType<AMS_SF_IMPL_HELPER_FUNCTION_NAME(CLASSNAME, FUNCNAME)>

    #define AMS_SF_IMPL_CONCEPT_HELPER_FUNCTION_NAME(CLASSNAME, FUNCNAME) \
        AMS_SF_IMPL_HELPER_FUNCTION_NAME_IMPL(CLASSNAME, FUNCNAME, intf_for_concept)

    #define AMS_SF_IMPL_DECLARE_HELPER_FUNCTIONS(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX)       \
        static void AMS_SF_IMPL_HELPER_FUNCTION_NAME(CLASSNAME, NAME) ARGS { __builtin_unreachable(); }                 \
        template<typename T, typename... Arguments>                                                                     \
            requires std::same_as<std::tuple<Arguments...>, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>          \
        static auto AMS_SF_IMPL_CONCEPT_HELPER_FUNCTION_NAME(CLASSNAME, NAME) (T &t, std::tuple<Arguments...> a) {      \
            return [&]<size_t... Ix>(std::index_sequence<Ix...>) {                                                      \
                return t.NAME(std::forward<typename std::tuple_element<Ix, decltype(a)>::type>(std::get<Ix>(a))...);    \
            }(std::make_index_sequence<sizeof...(Arguments)>());                                                        \
        }

    #define AMS_SF_IMPL_DECLARE_HELPERS(CLASSNAME, CMD_MACRO) \
        CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DECLARE_HELPER_FUNCTIONS)

    #define AMS_SF_IMPL_DECLARE_CONCEPT_REQUIREMENT(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX) \
        { AMS_SF_IMPL_CONCEPT_HELPER_FUNCTION_NAME(CLASSNAME, NAME) (impl, std::declval<AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>()) } -> ::ams::sf::impl::ServiceCommandResult;

    #define AMS_SF_IMPL_DEFINE_CONCEPT(CLASSNAME, CMD_MACRO)                \
        template<typename Impl>                                             \
        concept Is##CLASSNAME = requires (Impl &impl) {                     \
            CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DECLARE_CONCEPT_REQUIREMENT)   \
        };

    #define AMS_SF_IMPL_FUNCTION_POINTER_TYPE(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX)  \
        ::ams::sf::impl::ClassFunctionPointer<CLASSNAME, RETURN, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>

    #define AMS_SF_IMPL_DECLARE_FUNCTION_POINTER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX) \
        AMS_SF_IMPL_FUNCTION_POINTER_TYPE(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX) NAME;

    #define AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX)                                                                                            \
        template<typename ...Arguments>                                                                                                                                                                        \
            requires std::same_as<std::tuple<Arguments...>, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>                                                                                                 \
        RETURN Invoke##NAME##ByCommandTable (Arguments &&... args) {                                                                                                                                           \
            return this->cmd_table->NAME(this, std::forward<Arguments>(args)...);                                                                                                                              \
        }                                                                                                                                                                                                      \
        template<typename ...Arguments>                                                                                                                                                                        \
            requires (::ams::sf::impl::Invokable<AMS_SF_IMPL_HELPER_FUNCTION_NAME(CLASSNAME, NAME), Arguments...> &&                                                                                           \
                      std::same_as<std::tuple<Arguments...>, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>)                                                                                               \
        ALWAYS_INLINE RETURN NAME (Arguments &&... args) {                                                                                                                                                     \
            return this->Invoke##NAME##ByCommandTable(std::forward<Arguments>(args)...);                                                                                                                       \
        }                                                                                                                                                                                                      \
        template<typename ...Arguments>                                                                                                                                                                        \
            requires (::ams::sf::impl::Invokable<AMS_SF_IMPL_HELPER_FUNCTION_NAME(CLASSNAME, NAME), Arguments...> &&                                                                                           \
                      !std::same_as<std::tuple<Arguments...>, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>)                                                                                              \
        ALWAYS_INLINE RETURN NAME (Arguments &&... args) {                                                                                                                                                     \
            return [this] <typename... CorrectArguments, typename... PassedArguments> ALWAYS_INLINE_LAMBDA (::ams::sf::impl::TypeTag<std::tuple<CorrectArguments...>>, PassedArguments &&...args_) -> RETURN { \
                return this->template NAME<CorrectArguments...>(std::forward<CorrectArguments>(args_)...);                                                                                                     \
            }(::ams::sf::impl::TypeTag<AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>{}, std::forward<Arguments>(args)...);                                                                                \
        }

    #define AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION_INVOKER_HOLDER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX) \
        template<typename ...Arguments>                                                                                            \
            requires std::same_as<std::tuple<Arguments...>, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>                     \
        static RETURN NAME##Invoker (CLASSNAME *_this, Arguments &&... args) {                                                     \
            return static_cast<ImplHolder *>(_this)->NAME(std::forward<Arguments>(args)...);                                       \
        }

    #define AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION_INVOKER_POINTER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX) \
        template<typename ...Arguments>                                                                                             \
            requires std::same_as<std::tuple<Arguments...>, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>                      \
        static RETURN NAME##Invoker (CLASSNAME *_this, Arguments &&... args) {                                                      \
            return static_cast<ImplPointer *>(_this)->NAME(std::forward<Arguments>(args)...);                                       \
        }

    #define AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION_INVOKER_SHARED_POINTER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX) \
        template<typename ...Arguments>                                                                                                    \
            requires std::same_as<std::tuple<Arguments...>, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>                             \
        static RETURN NAME##Invoker (CLASSNAME *_this, Arguments &&... args) {                                                             \
            return static_cast<ImplSharedPointer *>(_this)->NAME(std::forward<Arguments>(args)...);                                        \
        }

    #define AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION_IMPL(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX)    \
        template<typename ...Arguments>                                                                                     \
            requires (std::same_as<std::tuple<Arguments...>, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)> &&          \
                      std::same_as<RETURN, decltype(impl.NAME(std::declval<Arguments>()...))>)                              \
        RETURN NAME (Arguments &&... args) {                                                                                \
            return this->impl.NAME(std::forward<Arguments>(args)...);                                                       \
        }

    #define AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION_IMPL_PTR(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX)    \
        template<typename ...Arguments>                                                                                         \
            requires (std::same_as<std::tuple<Arguments...>, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)> &&              \
                      std::same_as<RETURN, decltype(impl->NAME(std::declval<Arguments>()...))>)                                 \
        RETURN NAME (Arguments &&... args) {                                                                                    \
            return this->impl->NAME(std::forward<Arguments>(args)...);                                                          \
        }

    #define AMS_SF_IMPL_DEFINE_INTERFACE_IMPL_FUNCTION_POINTER_HOLDER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX)                                                                  \
        template<typename A> struct NAME##FunctionPointerHolder;                                                                                                                                        \
                                                                                                                                                                                                        \
        template<typename ...Arguments>                                                                                                                                                                 \
            requires std::same_as<std::tuple<Arguments...>, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>                                                                                          \
        struct NAME##FunctionPointerHolder<std::tuple<Arguments...>> {                                                                                                                                  \
            static constexpr auto Value = static_cast<AMS_SF_IMPL_FUNCTION_POINTER_TYPE(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX)>(&NAME##Invoker<Arguments...>);                \
        };

    #define AMS_SF_IMPL_DEFINE_INTERFACE_SERVICE_COMMAND_META_HOLDER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX)                                                                                      \
        template<typename ServiceImpl, typename A> struct NAME##ServiceCommandMetaHolder;                                                                                                                                  \
                                                                                                                                                                                                                           \
        template<typename ServiceImpl, typename ...Arguments>                                                                                                                                                              \
            requires std::same_as<std::tuple<Arguments...>, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>                                                                                                             \
        struct NAME##ServiceCommandMetaHolder<ServiceImpl, std::tuple<Arguments...>> {                                                                                                                                     \
            static constexpr auto Value = ::ams::sf::impl::MakeServiceCommandMeta<VERSION_MIN, VERSION_MAX, CMD_ID, &ServiceImpl::template Invoke##NAME##ByCommandTable<Arguments...>, RETURN, CLASSNAME, Arguments...>(); \
        };

    #define AMS_SF_IMPL_DEFINE_INTERFACE_COMMAND_POINTER_TABLE_MEMBER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX) \
        .NAME = NAME##FunctionPointerHolder<AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>::Value,

    #define AMS_SF_IMPL_DEFINE_CMIF_SERVICE_COMMAND_META_TABLE_ENTRY(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX) \
        NAME##ServiceCommandMetaHolder<ServiceImpl, AMS_SF_IMPL_HELPER_FUNCTION_ARGS(CLASSNAME, NAME)>::Value,

    template<typename T>
    struct Print;

    #define AMS_SF_IMPL_DEFINE_CLASS(BASECLASS, CLASSNAME, CMD_MACRO)                                                                       \
        class CLASSNAME : public BASECLASS {                                                                                                \
            NON_COPYABLE(CLASSNAME);                                                                                                        \
            NON_MOVEABLE(CLASSNAME);                                                                                                        \
            private:                                                                                                                        \
                struct CommandPointerTable {                                                                                                \
                    CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DECLARE_FUNCTION_POINTER)                                                              \
                };                                                                                                                          \
            private:                                                                                                                        \
                const CommandPointerTable * const cmd_table;                                                                                \
            private:                                                                                                                        \
                CLASSNAME() = delete;                                                                                                       \
            protected:                                                                                                                      \
                constexpr CLASSNAME(const CommandPointerTable *ct)                                                                          \
                    : cmd_table(ct) { /* ... */ }                                                                                           \
                virtual ~CLASSNAME() { /* ... */ }                                                                                          \
            public:                                                                                                                         \
                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION)                                                                \
            private:                                                                                                                        \
                template<typename S, typename T>                                                                                            \
                    requires ((std::same_as<CLASSNAME, S> && !std::same_as<CLASSNAME, T>&& Is##CLASSNAME<T>) &&                             \
                              (::ams::sf::IsMitmServiceObject<S> == ::ams::sf::IsMitmServiceImpl<T>))                                       \
                struct ImplGenerator {                                                                                                      \
                    public:                                                                                                                 \
                        class ImplHolder : public S {                                                                                       \
                            private:                                                                                                        \
                                T impl;                                                                                                     \
                            public:                                                                                                         \
                                template<typename... Args> requires std::constructible_from<T, Args...>                                     \
                                constexpr ImplHolder(Args &&... args)                                                                       \
                                    : S(std::addressof(CommandPointerTableImpl)), impl(std::forward<Args>(args)...)                         \
                                {                                                                                                           \
                                    /* ... */                                                                                               \
                                }                                                                                                           \
                                ALWAYS_INLINE T &GetImpl() { return this->impl; }                                                           \
                                ALWAYS_INLINE const T &GetImpl() const { return this->impl; }                                               \
                                                                                                                                            \
                                template<typename U = S> requires ::ams::sf::IsMitmServiceObject<S> && std::same_as<U, S>                   \
                                static ALWAYS_INLINE bool ShouldMitm(os::ProcessId p, ncm::ProgramId r) { return T::ShouldMitm(p, r); }     \
                            private:                                                                                                        \
                                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION_INVOKER_HOLDER)                                 \
                            public:                                                                                                         \
                                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION_IMPL)                                           \
                            private:                                                                                                        \
                                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_INTERFACE_IMPL_FUNCTION_POINTER_HOLDER)                             \
                            public:                                                                                                         \
                                static constexpr CommandPointerTable CommandPointerTableImpl = {                                            \
                                    CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_INTERFACE_COMMAND_POINTER_TABLE_MEMBER)                         \
                                };                                                                                                          \
                        };                                                                                                                  \
                        static_assert(Is##CLASSNAME<ImplHolder>);                                                                           \
                                                                                                                                            \
                        class ImplPointer : public S {                                                                                      \
                            private:                                                                                                        \
                                T *impl;                                                                                                    \
                            public:                                                                                                         \
                                constexpr ImplPointer(T *t)                                                                                 \
                                    : S(std::addressof(CommandPointerTableImpl)), impl(t)                                                   \
                                {                                                                                                           \
                                    /* ... */                                                                                               \
                                }                                                                                                           \
                                ALWAYS_INLINE T &GetImpl() { return *this->impl; }                                                          \
                                ALWAYS_INLINE const T &GetImpl() const { return *this->impl; }                                              \
                            private:                                                                                                        \
                                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION_INVOKER_POINTER)                                \
                            public:                                                                                                         \
                                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION_IMPL_PTR)                                       \
                            private:                                                                                                        \
                                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_INTERFACE_IMPL_FUNCTION_POINTER_HOLDER)                             \
                            public:                                                                                                         \
                                static constexpr CommandPointerTable CommandPointerTableImpl = {                                            \
                                    CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_INTERFACE_COMMAND_POINTER_TABLE_MEMBER)                         \
                                };                                                                                                          \
                        };                                                                                                                  \
                                                                                                                                            \
                        class ImplSharedPointer : public S {                                                                                \
                            private:                                                                                                        \
                                std::shared_ptr<T> impl;                                                                                    \
                            public:                                                                                                         \
                                constexpr ImplSharedPointer(std::shared_ptr<T> &&t)                                                         \
                                    : S(std::addressof(CommandPointerTableImpl)), impl(std::move(t))                                        \
                                {                                                                                                           \
                                    /* ... */                                                                                               \
                                }                                                                                                           \
                                ALWAYS_INLINE T &GetImpl() { return *this->impl; }                                                          \
                                ALWAYS_INLINE const T &GetImpl() const { return *this->impl; }                                              \
                            private:                                                                                                        \
                                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION_INVOKER_SHARED_POINTER)                         \
                            public:                                                                                                         \
                                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DECLARE_INTERFACE_FUNCTION_IMPL_PTR)                                       \
                            private:                                                                                                        \
                                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_INTERFACE_IMPL_FUNCTION_POINTER_HOLDER)                             \
                            public:                                                                                                         \
                                static constexpr CommandPointerTable CommandPointerTableImpl = {                                            \
                                    CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_INTERFACE_COMMAND_POINTER_TABLE_MEMBER)                         \
                                };                                                                                                          \
                        };                                                                                                                  \
                        static_assert(Is##CLASSNAME<ImplHolder>);                                                                           \
                };                                                                                                                          \
            private:                                                                                                                        \
                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_INTERFACE_SERVICE_COMMAND_META_HOLDER)                                              \
            public:                                                                                                                         \
                template<typename T> requires (!std::same_as<CLASSNAME, T>&& Is##CLASSNAME<T>)                                              \
                using ImplHolder = typename ImplGenerator<CLASSNAME, T>::ImplHolder;                                                        \
                                                                                                                                            \
                template<typename T> requires (!std::same_as<CLASSNAME, T>&& Is##CLASSNAME<T>)                                              \
                using ImplPointer = typename ImplGenerator<CLASSNAME, T>::ImplPointer;                                                      \
                                                                                                                                            \
                template<typename T> requires (!std::same_as<CLASSNAME, T>&& Is##CLASSNAME<T> &&                                            \
                                               std::derived_from<T, std::enable_shared_from_this<T>>)                                       \
                using ImplSharedPointer = typename ImplGenerator<CLASSNAME, T>::ImplSharedPointer;                                          \
                                                                                                                                            \
                AMS_SF_CMIF_IMPL_DEFINE_SERVICE_DISPATCH_TABLE {                                                                            \
                    CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_CMIF_SERVICE_COMMAND_META_TABLE_ENTRY)                                          \
                };                                                                                                                          \
        };

    #define AMS_SF_METHOD_INFO_6(CLASSNAME, HANDLER, CMD_ID, RETURN, NAME, ARGS) \
        HANDLER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, hos::Version_Min, hos::Version_Max)

    #define AMS_SF_METHOD_INFO_7(CLASSNAME, HANDLER, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN) \
        HANDLER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, hos::Version_Max)

    #define AMS_SF_METHOD_INFO_8(CLASSNAME, HANDLER, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX) \
        HANDLER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX)

    #define AMS_SF_METHOD_INFO_X(_, _0, _1, _2, _3, _4, _5, _6, _7, FUNC, ...) FUNC

    #define AMS_SF_METHOD_INFO(...) \
        AMS_SF_METHOD_INFO_X(, ## __VA_ARGS__, AMS_SF_METHOD_INFO_8(__VA_ARGS__), AMS_SF_METHOD_INFO_7(__VA_ARGS__), AMS_SF_METHOD_INFO_6(__VA_ARGS__))

    #define AMS_SF_DEFINE_INTERFACE(CLASSNAME, CMD_MACRO)                           \
        AMS_SF_IMPL_DECLARE_HELPERS(CLASSNAME,CMD_MACRO)                            \
        AMS_SF_IMPL_DEFINE_CONCEPT(CLASSNAME, CMD_MACRO)                            \
        AMS_SF_IMPL_DEFINE_CLASS( ::ams::sf::IServiceObject, CLASSNAME, CMD_MACRO)  \
        static_assert(Is##CLASSNAME<CLASSNAME>);

    #define AMS_SF_DEFINE_MITM_INTERFACE(CLASSNAME, CMD_MACRO)                          \
        AMS_SF_IMPL_DECLARE_HELPERS(CLASSNAME,CMD_MACRO)                                \
        AMS_SF_IMPL_DEFINE_CONCEPT(CLASSNAME, CMD_MACRO)                                \
        AMS_SF_IMPL_DEFINE_CLASS(::ams::sf::IMitmServiceObject, CLASSNAME, CMD_MACRO)   \
        static_assert(Is##CLASSNAME<CLASSNAME>);

    #define AMS_SF_IMPL_DECLARE_INTERFACE_METHODS(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, VERSION_MIN, VERSION_MAX) \
        RETURN NAME ARGS;

    #define AMS_SF_DECLARE_INTERFACE_METHODS(CMD_MACRO) \
        CMD_MACRO(_, AMS_SF_IMPL_DECLARE_INTERFACE_METHODS)

}