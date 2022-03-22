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
#include <stratosphere/tipc/tipc_service_object_base.hpp>
#include <stratosphere/tipc/impl/tipc_impl_command_serialization.hpp>

namespace ams::tipc::impl {

    template<typename T>
    concept HasDefaultServiceCommandProcessor = requires (T &t, const svc::ipc::MessageBuffer &message_buffer) {
        { t.ProcessDefaultServiceCommand(message_buffer) } -> std::same_as<Result>;
    };

    struct SyncFunctionTraits {
        public:
            template<typename R, typename C, typename... A>
            static std::tuple<A...> GetArgsImpl(R(C::*)(A...));
    };

    template<auto F>
    using SyncFunctionArgsType = decltype(SyncFunctionTraits::GetArgsImpl(F));

    #define AMS_TIPC_IMPL_DEFINE_SYNC_METHOD_HOLDER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        struct NAME##ArgumentsFunctionHolder { RETURN f ARGS; };

    #define AMS_TIPC_IMPL_EXTRACT_SYNC_METHOD_ARGUMENTS(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        using NAME##ArgumentsType = ::ams::tipc::impl::SyncFunctionArgsType<&NAME##ArgumentsFunctionHolder::f>;

    #define AMS_TIPC_IMPL_GET_MAXIMUM_REQUEST_SIZE(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        , ::ams::tipc::impl::CommandMetaInfo<CMD_ID + 0x10, NAME##ArgumentsType>::InMessageTotalSize

    #define AMS_TIPC_IMPL_GET_MAXIMUM_RESPONSE_SIZE(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        , ::ams::tipc::impl::CommandMetaInfo<CMD_ID + 0x10, NAME##ArgumentsType>::OutMessageTotalSize

    #define AMS_TIPC_IMPL_DEFINE_INTERFACE(BASECLASS, CLASSNAME, CMD_MACRO)       \
        class CLASSNAME : public BASECLASS {                                      \
            private:                                                              \
                CMD_MACRO(CLASSNAME, AMS_TIPC_IMPL_DEFINE_SYNC_METHOD_HOLDER)     \
            public:                                                               \
                CMD_MACRO(CLASSNAME, AMS_TIPC_IMPL_EXTRACT_SYNC_METHOD_ARGUMENTS) \
            public:                                                               \
                static constexpr size_t MaximumRequestSize = std::max<size_t>({   \
                    0                                                             \
                    CMD_MACRO(CLASSNAME, AMS_TIPC_IMPL_GET_MAXIMUM_REQUEST_SIZE)  \
                });                                                               \
                                                                                  \
                static constexpr size_t MaximumResponseSize = std::max<size_t>({  \
                    0                                                             \
                    CMD_MACRO(CLASSNAME, AMS_TIPC_IMPL_GET_MAXIMUM_RESPONSE_SIZE) \
                });                                                               \
        };

    #define AMS_TIPC_IMPL_DEFINE_CONCEPT_HELPERS(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX)               \
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

    #define AMS_TIPC_IMPL_CHECK_CONCEPT_HELPER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        Is##CLASSNAME##__##NAME<T> &&

    #define AMS_TIPC_IMPL_DEFINE_CONCEPT(CLASSNAME, CMD_MACRO)                               \
        CMD_MACRO(CLASSNAME, AMS_TIPC_IMPL_DEFINE_CONCEPT_HELPERS)                           \
                                                                                             \
        template<typename T>                                                                 \
        concept Is##CLASSNAME = CMD_MACRO(CLASSNAME, AMS_TIPC_IMPL_CHECK_CONCEPT_HELPER) true;

    #define AMS_TIPC_IMPL_PROCESS_METHOD_REQUEST(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        else if (constexpr u16 TipcCommandId = CMD_ID + 0x10; tag == TipcCommandId) {                                       \
            return this->ProcessMethodById<TipcCommandId, ImplType>(impl, message_buffer, fw_ver);                          \
        }

    #define AMS_TIPC_IMPL_PROCESS_METHOD_REQUEST_BY_ID(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX)   \
        if constexpr (constexpr u16 TipcCommandId = CMD_ID + 0x10; CommandId == TipcCommandId) {                                    \
            constexpr bool MinValid = VERSION_MIN == hos::Version_Min;                                                              \
            constexpr bool MaxValid = VERSION_MAX == hos::Version_Max;                                                              \
            if ((MinValid || VERSION_MIN <= fw_ver) && (MaxValid || fw_ver <= VERSION_MAX)) {                                       \
                return ::ams::tipc::impl::InvokeServiceCommandImpl<TipcCommandId, &ImplType::NAME, ImplType>(impl, message_buffer); \
            }                                                                                                                       \
        }

    #define AMS_TIPC_IMPL_IS_FIRMWARE_VERSION_ALWAYS_VALID(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
            {                                                                                                                         \
                constexpr bool MinValid = VERSION_MIN == hos::Version_Min;                                                            \
                constexpr bool MaxValid = VERSION_MAX == hos::Version_Max;                                                            \
                if (!MinValid || !MaxValid) {                                                                                         \
                    return false;                                                                                                     \
                }                                                                                                                     \
            }


    #define AMS_TIPC_DEFINE_INTERFACE_WITH_DEFAULT_BASE(NAMESPACE, INTERFACE, BASE, CMD_MACRO)                                                         \
        namespace NAMESPACE {                                                                                                                          \
                                                                                                                                                       \
            AMS_TIPC_IMPL_DEFINE_INTERFACE(BASE, INTERFACE, CMD_MACRO)                                                                                 \
            AMS_TIPC_IMPL_DEFINE_CONCEPT(INTERFACE, CMD_MACRO)                                                                                         \
                                                                                                                                                       \
        }                                                                                                                                              \
                                                                                                                                                       \
        namespace ams::tipc::impl {                                                                                                                    \
                                                                                                                                                       \
            template<typename Base, typename ImplHolder, typename ImplGetter, typename Root>                                                           \
            class ImplTemplateBaseT<::NAMESPACE::INTERFACE, Base, ImplHolder, ImplGetter, Root> : public Base, public ImplHolder {                     \
                public:                                                                                                                                \
                    template<typename... Args>                                                                                                         \
                    constexpr explicit ImplTemplateBaseT(Args &&...args) : ImplHolder(std::forward<Args>(args)...) { /* ... */ }                       \
                private:                                                                                                                               \
                    template<typename ImplType>                                                                                                        \
                    ALWAYS_INLINE Result ProcessDefaultMethod(ImplType *impl, const svc::ipc::MessageBuffer &message_buffer) const {                   \
                        /* Handle a default command. */                                                                                                \
                        if constexpr (HasDefaultServiceCommandProcessor<ImplType>) {                                                                   \
                            return impl->ProcessDefaultServiceCommand(message_buffer);                                                                 \
                        } else {                                                                                                                       \
                            return tipc::ResultInvalidMethod();                                                                                        \
                        }                                                                                                                              \
                    }                                                                                                                                  \
                                                                                                                                                       \
                    template<u16 CommandId, typename ImplType>                                                                                         \
                    ALWAYS_INLINE Result ProcessMethodById(ImplType *impl, const svc::ipc::MessageBuffer &message_buffer, hos::Version fw_ver) const { \
                        CMD_MACRO(ImplType, AMS_TIPC_IMPL_PROCESS_METHOD_REQUEST_BY_ID)                                                                \
                                                                                                                                                       \
                        return this->ProcessDefaultMethod<ImplType>(impl, message_buffer);                                                             \
                    }                                                                                                                                  \
                                                                                                                                                       \
                    static consteval bool IsFirmwareVersionAlwaysValid() {                                                                             \
                        CMD_MACRO(ImplType, AMS_TIPC_IMPL_IS_FIRMWARE_VERSION_ALWAYS_VALID);                                                           \
                        return true;                                                                                                                   \
                    }                                                                                                                                  \
                public:                                                                                                                                \
                    virtual Result ProcessRequest() override {                                                                                         \
                        /* Get the implementation object. */                                                                                           \
                        auto * const impl = ImplGetter::GetImplPointer(static_cast<ImplHolder *>(this));                                               \
                                                                                                                                                       \
                        /* Get the implementation type. */                                                                                             \
                        using ImplType = typename std::remove_reference<decltype(*impl)>::type;                                                        \
                        static_assert(::NAMESPACE::Is##INTERFACE<ImplType>);                                                                           \
                                                                                                                                                       \
                        /* Get accessor to the message buffer. */                                                                                      \
                        const svc::ipc::MessageBuffer message_buffer(svc::ipc::GetMessageBuffer());                                                    \
                                                                                                                                                       \
                        /* Get decision variables. */                                                                                                  \
                        const auto tag    = svc::ipc::MessageBuffer::MessageHeader(message_buffer).GetTag();                                           \
                        const auto fw_ver = IsFirmwareVersionAlwaysValid() ? hos::Version_Current : hos::GetVersion();                                 \
                                                                                                                                                       \
                        /* Process against the command ids. */                                                                                         \
                        if (false) { }                                                                                                                 \
                        CMD_MACRO(ImplType, AMS_TIPC_IMPL_PROCESS_METHOD_REQUEST)                                                                      \
                        else {                                                                                                                         \
                            return this->ProcessDefaultMethod<ImplType>(impl, message_buffer);                                                         \
                        }                                                                                                                              \
                    }                                                                                                                                  \
            };                                                                                                                                         \
                                                                                                                                                       \
        }


    #define AMS_TIPC_DEFINE_INTERFACE(NAMESPACE, INTERFACE, CMD_MACRO) \
        AMS_TIPC_DEFINE_INTERFACE_WITH_DEFAULT_BASE(NAMESPACE, INTERFACE, ::ams::tipc::ServiceObjectBase, CMD_MACRO)

    #define AMS_TIPC_METHOD_INFO_7(CLASSNAME, HANDLER, CMD_ID, RETURN, NAME, ARGS, ARGNAMES) \
        HANDLER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, hos::Version_Min, hos::Version_Max)

    #define AMS_TIPC_METHOD_INFO_8(CLASSNAME, HANDLER, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN) \
        HANDLER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, hos::Version_Max)

    #define AMS_TIPC_METHOD_INFO_9(CLASSNAME, HANDLER, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
        HANDLER(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX)

    #define AMS_TIPC_METHOD_INFO_X(_, _0, _1, _2, _3, _4, _5, _6, _7, _8, FUNC, ...) FUNC

    #define AMS_TIPC_METHOD_INFO(...) \
        AMS_TIPC_METHOD_INFO_X(, ## __VA_ARGS__, AMS_TIPC_METHOD_INFO_9(__VA_ARGS__), AMS_TIPC_METHOD_INFO_8(__VA_ARGS__), AMS_TIPC_METHOD_INFO_7(__VA_ARGS__))

}
