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

namespace ams::sf::impl {

    struct SyncFunctionTraits {
        public:
            template<typename R, typename C, typename... A>
            static std::tuple<A...> GetArgsImpl(R(C::*)(A...));
    };

    template<auto F>
    using SyncFunctionArgsType = decltype(SyncFunctionTraits::GetArgsImpl(F));

    template<typename T>
    struct TypeTag{};

    template<size_t First, size_t... Ix>
    static constexpr inline size_t ParameterCount = sizeof...(Ix);

    #define AMS_SF_IMPL_SYNC_FUNCTION_NAME(FUNCNAME) \
        _ams_sf_sync_##FUNCNAME

    #define AMS_SF_IMPL_DEFINE_INTERFACE_SYNC_METHOD(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        virtual RETURN AMS_SF_IMPL_SYNC_FUNCTION_NAME(NAME) ARGS = 0;

    #define AMS_SF_IMPL_EXTRACT_INTERFACE_SYNC_METHOD_ARGUMENTS(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        using NAME##ArgumentsType = ::ams::sf::impl::SyncFunctionArgsType<&CLASSNAME::AMS_SF_IMPL_SYNC_FUNCTION_NAME(NAME)>;

    #define AMS_SF_IMPL_DEFINE_INTERFACE_METHOD(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        ALWAYS_INLINE RETURN NAME ARGS {                                                                                   \
            return this->AMS_SF_IMPL_SYNC_FUNCTION_NAME(NAME) ARGNAMES;                                                    \
        }

    #define AMS_SF_IMPL_DEFINE_INTERFACE_SERVICE_COMMAND_META_HOLDER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX)                                                           \
        template<typename Interface, typename A> struct NAME##ServiceCommandMetaHolder;                                                                                                                   \
                                                                                                                                                                                                          \
        template<typename Interface, typename ...Arguments>                                                                                                                                               \
            requires std::same_as<std::tuple<Arguments...>, NAME##ArgumentsType>                                                                                                                          \
        struct NAME##ServiceCommandMetaHolder<Interface, std::tuple<Arguments...>> {                                                                                                                      \
            static constexpr auto Value = ::ams::sf::impl::MakeServiceCommandMeta<VERSION_MIN, VERSION_MAX, CMD_ID, &Interface::AMS_SF_IMPL_SYNC_FUNCTION_NAME(NAME), RETURN, Interface, Arguments...>(); \
        };

    #define AMS_SF_IMPL_GET_NULL_FOR_PARAMETER_COUNT(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        , 0

    #define AMS_SF_IMPL_DEFINE_CMIF_SERVICE_COMMAND_META_TABLE_ENTRY(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        NAME##ServiceCommandMetaHolder<Interface, NAME##ArgumentsType>::Value,

    template<typename...>
    struct Print;

    #define AMS_SF_IMPL_DEFINE_INTERFACE(BASECLASS, CLASSNAME, CMD_MACRO)                                                                                                        \
        class CLASSNAME : public BASECLASS {                                                                                                                                     \
            private:                                                                                                                                                             \
                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_INTERFACE_SYNC_METHOD)                                                                                                   \
            public:                                                                                                                                                              \
                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_EXTRACT_INTERFACE_SYNC_METHOD_ARGUMENTS)                                                                                        \
            public:                                                                                                                                                              \
                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_INTERFACE_METHOD)                                                                                                        \
            private:                                                                                                                                                             \
                CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_INTERFACE_SERVICE_COMMAND_META_HOLDER)                                                                                   \
            public:                                                                                                                                                              \
                template<typename Interface>                                                                                                                                     \
                static constexpr inline ::ams::sf::cmif::ServiceDispatchTable s_CmifServiceDispatchTable {                                                                       \
                    [] {                                                                                                                                                         \
                        constexpr size_t CurSize = ::ams::sf::impl::ParameterCount<0 CMD_MACRO(CLASSNAME, AMS_SF_IMPL_GET_NULL_FOR_PARAMETER_COUNT) >;                           \
                        std::array<::ams::sf::cmif::ServiceCommandMeta, CurSize> cur_entries { CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_CMIF_SERVICE_COMMAND_META_TABLE_ENTRY) }; \
                                                                                                                                                                                 \
                        constexpr const auto &BaseEntries = ::ams::sf::cmif::ServiceDispatchTraits<BASECLASS>::DispatchTable.GetEntries();                                       \
                        constexpr size_t BaseSize         = BaseEntries.size();                                                                                                  \
                                                                                                                                                                                 \
                        constexpr size_t CombinedSize = BaseSize + CurSize;                                                                                                      \
                                                                                                                                                                                 \
                        std::array<size_t, CombinedSize> map{};                                                                                                                  \
                        for (size_t i = 0; i < CombinedSize; ++i) { map[i] = i; }                                                                                                \
                                                                                                                                                                                 \
                        for (size_t i = 1; i < CombinedSize; ++i) {                                                                                                              \
                            size_t j = i;                                                                                                                                        \
                            while (j > 0) {                                                                                                                                      \
                                const auto li = map[j];                                                                                                                          \
                                const auto ri = map[j - 1];                                                                                                                      \
                                                                                                                                                                                 \
                                const auto &lhs = (li < BaseSize) ? BaseEntries[li] : cur_entries[li - BaseSize];                                                                \
                                const auto &rhs = (ri < BaseSize) ? BaseEntries[ri] : cur_entries[ri - BaseSize];                                                                \
                                                                                                                                                                                 \
                                if (!(rhs > lhs)) {                                                                                                                              \
                                    break;                                                                                                                                       \
                                }                                                                                                                                                \
                                                                                                                                                                                 \
                                std::swap(map[j], map[j - 1]);                                                                                                                   \
                                                                                                                                                                                 \
                                --j;                                                                                                                                             \
                            }                                                                                                                                                    \
                        }                                                                                                                                                        \
                                                                                                                                                                                 \
                        std::array<::ams::sf::cmif::ServiceCommandMeta, CombinedSize> combined_entries{};                                                                        \
                        for (size_t i = 0; i < CombinedSize; ++i) {                                                                                                              \
                            if (map[i] < BaseSize) {                                                                                                                             \
                                combined_entries[i] = BaseEntries[map[i]];                                                                                                       \
                            } else {                                                                                                                                             \
                                combined_entries[i] = cur_entries[map[i] - BaseSize];                                                                                            \
                            }                                                                                                                                                    \
                        }                                                                                                                                                        \
                                                                                                                                                                                 \
                        return ::ams::sf::cmif::ServiceDispatchTable { combined_entries };                                                                                       \
                    }()                                                                                                                                                          \
                };                                                                                                                                                               \
        };

    #define AMS_SF_IMPL_DEFINE_CONCEPT_HELPERS(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX)                 \
        template<typename T, typename... Args>                                                                                            \
        concept Is##CLASSNAME##__##NAME##Impl = requires (T &t, Args &&... args) {                                                        \
            { t.NAME(std::forward<Args>(args)...) } -> std::same_as<RETURN>;                                                              \
        };                                                                                                                                \
                                                                                                                                          \
        template<typename T, typename A>                                                                                                  \
        struct Is##CLASSNAME##__##NAME##Holder : std::false_type{};                                                                       \
                                                                                                                                          \
        template<typename T, typename... Args> requires std::same_as<std::tuple<Args...>, CLASSNAME::NAME##ArgumentsType>                 \
        struct Is##CLASSNAME##__##NAME##Holder<T, std::tuple<Args...>> : std::bool_constant<Is##CLASSNAME##__##NAME##Impl<T, Args...>>{}; \
                                                                                                                                          \
        template<typename T>                                                                                                              \
        static constexpr inline bool Is##CLASSNAME##__##NAME = Is##CLASSNAME##__##NAME##Holder<T, CLASSNAME::NAME##ArgumentsType>::value;

    #define AMS_SF_IMPL_CHECK_CONCEPT_HELPER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        Is##CLASSNAME##__##NAME<T> &&

    #define AMS_SF_IMPL_DEFINE_CONCEPT(CLASSNAME, CMD_MACRO)                                 \
        CMD_MACRO(CLASSNAME, AMS_SF_IMPL_DEFINE_CONCEPT_HELPERS)                             \
                                                                                             \
        template<typename T>                                                                 \
        concept Is##CLASSNAME = CMD_MACRO(CLASSNAME, AMS_SF_IMPL_CHECK_CONCEPT_HELPER) true;

    #define AMS_SF_DEFINE_INTERFACE_IMPL(BASECLASS, CLASSNAME, CMD_MACRO) \
        AMS_SF_IMPL_DEFINE_INTERFACE(BASECLASS, CLASSNAME, CMD_MACRO)     \
        AMS_SF_IMPL_DEFINE_CONCEPT(CLASSNAME, CMD_MACRO)                  \
        static_assert(Is##CLASSNAME<CLASSNAME>);

    #define AMS_SF_METHOD_INFO_7(CLASSNAME, HANDLER, CMD_ID, RETURN, NAME, ARGS, ARGNAMES) \
        HANDLER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, hos::Version_Min, hos::Version_Max)

    #define AMS_SF_METHOD_INFO_8(CLASSNAME, HANDLER, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN) \
        HANDLER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, hos::Version_Max)

    #define AMS_SF_METHOD_INFO_9(CLASSNAME, HANDLER, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        HANDLER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX)

    #define AMS_SF_METHOD_INFO_X(_, _0, _1, _2, _3, _4, _5, _6, _7, _8, FUNC, ...) FUNC

    #define AMS_SF_METHOD_INFO(...) \
        AMS_SF_METHOD_INFO_X(, ## __VA_ARGS__, AMS_SF_METHOD_INFO_9(__VA_ARGS__), AMS_SF_METHOD_INFO_8(__VA_ARGS__), AMS_SF_METHOD_INFO_7(__VA_ARGS__))

}