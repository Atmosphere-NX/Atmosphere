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
#include "../sf_common.hpp"
#include "../sf_service_object.hpp"
#include "../sf_out.hpp"
#include "../sf_buffers.hpp"
#include "../sf_handles.hpp"
#include "../cmif/sf_cmif_pointer_and_size.hpp"
#include "../cmif/sf_cmif_service_dispatch.hpp"
#include "../cmif/sf_cmif_service_object_holder.hpp"
#include "../cmif/sf_cmif_domain_api.hpp"
#include "../hipc/sf_hipc_api.hpp"
#include "../hipc/sf_hipc_server_session_manager.hpp"

/* Serialization classes. */
namespace ams::sf {

    namespace impl {

        struct ProcessIdHolder {
            os::ProcessId process_id;

            constexpr explicit operator os::ProcessId() const { return this->process_id; }
            constexpr os::ProcessId GetValue() const { return this->process_id; }
            constexpr void SetValue(const os::ProcessId &p) { this->process_id = p; }
        };

    }

    struct ClientProcessId : public impl::ProcessIdHolder {};
    static_assert(std::is_trivial<ClientProcessId>::value && sizeof(ClientProcessId) == sizeof(os::ProcessId), "ClientProcessId");

    struct ClientAppletResourceUserId : public impl::ProcessIdHolder {};
    static_assert(std::is_trivial<ClientAppletResourceUserId >::value && sizeof(ClientAppletResourceUserId ) == sizeof(os::ProcessId), "ClientAppletResourceUserId");

    namespace impl {

        constexpr inline Result MarshalProcessId(ClientProcessId &client, const os::ProcessId &client_process_id) {
            client.SetValue(client_process_id);
            return ResultSuccess();
        }

        constexpr inline Result MarshalProcessId(ClientAppletResourceUserId &client, const os::ProcessId &client_process_id) {
            if (client.GetValue() != client_process_id && client.GetValue() != os::ProcessId{}) {
                return sf::ResultPreconditionViolation();
            }
            return ResultSuccess();
        }

    }

    namespace impl {

        struct OutObjectTag{};

    }

    template<typename T>
    class IsOutForceEnabled<std::shared_ptr<T>> : public std::true_type{};

    template<typename ServiceImpl>
    class Out<std::shared_ptr<ServiceImpl>> : public impl::OutObjectTag {
        static_assert(std::is_base_of<sf::IServiceObject, ServiceImpl>::value, "Out<std::shared_ptr<ServiceImpl>> requires ServiceObject base.");

        template<typename>
        friend class Out;

        public:
            using ServiceImplType = ServiceImpl;
        private:
            cmif::ServiceObjectHolder *srv;
            cmif::DomainObjectId *object_id;
        public:
            Out(cmif::ServiceObjectHolder *s) : srv(s), object_id(nullptr) { /* ... */ }
            Out(cmif::ServiceObjectHolder *s, cmif::DomainObjectId *o) : srv(s), object_id(o) { /* ... */ }

            void SetValue(std::shared_ptr<ServiceImpl> &&s, cmif::DomainObjectId new_object_id = cmif::InvalidDomainObjectId) {
                *this->srv = cmif::ServiceObjectHolder(std::move(s));
                if (new_object_id != cmif::InvalidDomainObjectId) {
                    AMS_ABORT_UNLESS(object_id != nullptr);
                    *this->object_id = new_object_id;
                }
            }
    };

}


namespace ams::sf::impl {

    /* Machinery for filtering type lists. */
    template<class, class>
    struct TupleCat;

    template<class... First, class... Second>
    struct TupleCat<std::tuple<First...>, std::tuple<Second...>> {
        using type = std::tuple<First..., Second...>;
    };

    template<template <typename> typename Cond>
    struct TupleFilter {
        private:
            template<typename, typename>
            struct ImplType;

            template<typename Res>
            struct ImplType<Res, std::tuple<>> {
                using type = Res;
            };

            template<typename Res, typename T, typename... Ts>
            struct ImplType<Res, std::tuple<T, Ts...>> {
                using type = typename std::conditional<Cond<T>::value,
                                                       typename ImplType<typename TupleCat<Res, std::tuple<T>>::type, std::tuple<Ts...>>::type,
                                                       typename ImplType<Res, std::tuple<Ts...>>::type
                                                      >::type;
            };
        public:
            template<typename T>
            using FilteredType = typename ImplType<std::tuple<>, T>::type;
    };

    enum class ArgumentType {
        InData,
        OutData,
        Buffer,
        InHandle,
        OutHandle,
        InObject,
        OutObject,
    };

    template<typename T>
    struct IsInObject : public std::false_type{};

    template<typename T>
    struct IsInObject<std::shared_ptr<T>> : public std::true_type {
        static_assert(std::is_base_of<sf::IServiceObject, T>::value, "Invalid IsInObject<std::shared_ptr<T>>");
    };

    template<typename T>
    constexpr inline ArgumentType GetArgumentType = [] {
        if constexpr (sf::IsBuffer<T>) {
            return ArgumentType::Buffer;
        } else if constexpr (IsInObject<T>::value) {
            return ArgumentType::InObject;
        } else if constexpr (std::is_base_of<sf::impl::OutObjectTag, T>::value) {
            return ArgumentType::OutObject;
        } else if constexpr (std::is_base_of<sf::impl::InHandleTag, T>::value) {
            return ArgumentType::InHandle;
        } else if constexpr (std::is_base_of<sf::impl::OutHandleTag, T>::value) {
            return ArgumentType::OutHandle;
        } else if constexpr (std::is_base_of<sf::impl::OutBaseTag, T>::value) {
            return ArgumentType::OutData;
        } else if constexpr (std::is_trivial<T>::value && !std::is_pointer<T>::value) {
            return ArgumentType::InData;
        } else if constexpr (std::is_same<T, ::ams::Result>::value) {
            return ArgumentType::InData;
        } else {
            static_assert(!std::is_same<T, T>::value, "Invalid ArgumentType<T>");
        }
    }();

    template<typename T, ArgumentType ArgT>
    struct ArgumentTypeFilter : public std::bool_constant<GetArgumentType<T> == ArgT>{};

    template<typename T, typename U>
    struct TypeEqualityFilter : public std::bool_constant<std::is_same<T, U>::value>{};

    /* Argument type filters. */
    template<typename T>
    using InDataFilter    = ArgumentTypeFilter<T, ArgumentType::InData>;

    template<typename T>
    using OutDataFilter   = ArgumentTypeFilter<T, ArgumentType::OutData>;

    template<typename T>
    using BufferFilter    = ArgumentTypeFilter<T, ArgumentType::Buffer>;

    template<typename T>
    using InHandleFilter  = ArgumentTypeFilter<T, ArgumentType::InHandle>;

    template<typename T>
    using OutHandleFilter = ArgumentTypeFilter<T, ArgumentType::OutHandle>;

    template<typename T>
    using InObjectFilter  = ArgumentTypeFilter<T, ArgumentType::InObject>;

    template<typename T>
    using OutObjectFilter = ArgumentTypeFilter<T, ArgumentType::OutObject>;

    template<typename T>
    struct ProcessIdHolderFilter : public std::bool_constant<std::is_base_of<sf::impl::ProcessIdHolder, T>::value>{};

    /* Handle kind filters. */
    template<typename T>
    using InMoveHandleFilter = TypeEqualityFilter<T, sf::MoveHandle>;

    template<typename T>
    using InCopyHandleFilter = TypeEqualityFilter<T, sf::CopyHandle>;

    template<typename T>
    using OutMoveHandleFilter = TypeEqualityFilter<T, sf::Out<sf::MoveHandle>>;

    template<typename T>
    using OutCopyHandleFilter = TypeEqualityFilter<T, sf::Out<sf::CopyHandle>>;

    template<typename>
    struct BufferAttributeArrayGetter;

    template<typename ...Ts>
    struct BufferAttributeArrayGetter<std::tuple<Ts...>> {
        static constexpr std::array<u32, sizeof...(Ts)> value = { BufferAttributes<Ts>..., };
    };

    template<>
    struct BufferAttributeArrayGetter<std::tuple<>> {
        static constexpr std::array<u32, 0> value{};
    };

    template<auto Predicate>
    struct BufferAttributeCounter {

        template<size_t Size>
        static constexpr size_t GetCount(const std::array<u32, Size> &attributes_array) {
            size_t count = 0;
            for (size_t i = 0; i < Size; i++) {
                if (Predicate(attributes_array[i])) {
                    count++;
                }
            }
            return count;
        }

    };

    NX_CONSTEXPR size_t InHipcMapAliasBufferPredicate(const u32 attribute) {
        if ((attribute & SfBufferAttr_In) && !(attribute & SfBufferAttr_Out)) {
            return (attribute & SfBufferAttr_HipcMapAlias) || (attribute & SfBufferAttr_HipcAutoSelect);
        } else {
            return false;
        }
    }

    NX_CONSTEXPR size_t OutHipcMapAliasBufferPredicate(const u32 attribute) {
        if (!(attribute & SfBufferAttr_In) && (attribute & SfBufferAttr_Out)) {
            return (attribute & SfBufferAttr_HipcMapAlias) || (attribute & SfBufferAttr_HipcAutoSelect);
        } else {
            return false;
        }
    }

    NX_CONSTEXPR size_t InHipcPointerBufferPredicate(const u32 attribute) {
        if ((attribute & SfBufferAttr_In) && !(attribute & SfBufferAttr_Out)) {
            return (attribute & SfBufferAttr_HipcPointer) || (attribute & SfBufferAttr_HipcAutoSelect);
        } else {
            return false;
        }
    }

    NX_CONSTEXPR size_t OutHipcPointerBufferPredicate(const u32 attribute) {
        if (!(attribute & SfBufferAttr_In) && (attribute & SfBufferAttr_Out)) {
            return (attribute & SfBufferAttr_HipcPointer) || (attribute & SfBufferAttr_HipcAutoSelect);
        } else {
            return false;
        }
    }

    NX_CONSTEXPR size_t FixedSizeOutHipcPointerBufferPredicate(const u32 attribute) {
        return OutHipcPointerBufferPredicate(attribute) && (attribute & SfBufferAttr_FixedSize);
    }

    template<typename>
    struct RawDataOffsetCalculator;

    template<typename... Ts>
    struct RawDataOffsetCalculator<std::tuple<Ts...>> {
        private:
            template<typename T>
            struct LayoutHelper {
                static_assert(GetArgumentType<T> == ArgumentType::InData, "LayoutHelper InData");
                static constexpr size_t Alignment = alignof(T);
                static constexpr size_t Size      = sizeof(T);
            };
            template<typename T>
            struct LayoutHelper<Out<T>> {
                static_assert(GetArgumentType<T> == ArgumentType::InData, "LayoutHelper OutData");
                static constexpr size_t Alignment = alignof(T);
                static constexpr size_t Size      = sizeof(T);
            };

            static constexpr void StableSort(std::array<size_t, sizeof...(Ts)> &map, const std::array<size_t, sizeof...(Ts)> &values) {
                /* Use insertion sort, which is stable and optimal for small numbers of parameters. */
                for (size_t i = 1; i < sizeof...(Ts); i++) {
                    for (size_t j = i; j > 0 && values[map[j-1]] > values[map[j]]; j--) {
                        std::swap(map[j], map[j-1]);
                    }
                }
            }
        public:
            static constexpr std::array<size_t, sizeof...(Ts)+1> Offsets = [] {
                std::array<size_t, sizeof...(Ts)+1> offsets{};
                offsets[0] = 0;
                if constexpr (sizeof...(Ts) > 0) {
                    /* Get size, alignment for each type. */
                    const std::array<size_t, sizeof...(Ts)> sizes = { LayoutHelper<Ts>::Size... };
                    const std::array<size_t, sizeof...(Ts)> aligns = { LayoutHelper<Ts>::Alignment... };

                    /* We want to sort...by alignment. */
                    std::array<size_t, sizeof...(Ts)> map = {};
                    for (size_t i = 0; i < sizeof...(Ts); i++) { map[i] = i; }
                    StableSort(map, aligns);

                    /* Iterate over sorted sizes. */
                    size_t cur_offset = 0;
                    for (size_t i : map) {
                        cur_offset = util::AlignUp(cur_offset, aligns[i]);
                        offsets[i] = cur_offset;
                        cur_offset += sizes[i];
                    }
                    offsets[sizeof...(Ts)] = cur_offset;
                }
                return offsets;
            }();

    };

    struct ArgumentSerializationInfo {
        /* Type used to select from below fields. */
        ArgumentType arg_type;

        /* Raw data indexing. */
        size_t in_raw_data_index;
        size_t out_raw_data_index;

        /* Buffer indexing. */
        size_t buffer_index;
        size_t send_map_alias_index;
        size_t recv_map_alias_index;
        size_t send_pointer_index;
        size_t recv_pointer_index;
        size_t unfixed_recv_pointer_index;
        size_t fixed_size;

        /* Handle indexing. */
        size_t in_move_handle_index;
        size_t in_copy_handle_index;
        size_t out_move_handle_index;
        size_t out_copy_handle_index;

        /* Object indexing. */
        size_t in_object_index;
        size_t out_object_index;
    };

    template<auto MemberFunction, typename Return, typename Class, typename... Arguments>
    struct CommandMetaInfo {
        public:
            using ReturnType        = Return;
            using ClassType         = Class;
            using ClassTypePointer  = ClassType *;
            using ArgsType          = std::tuple<typename std::decay<Arguments>::type...>;

            static constexpr bool ReturnsResult = std::is_same<ReturnType, Result>::value;
            static constexpr bool ReturnsVoid   = std::is_same<ReturnType, void>::value;
            static_assert(ReturnsResult || ReturnsVoid, "Service Commands must return Result or void.");

            using InDatas    = TupleFilter<InDataFilter>::FilteredType<ArgsType>;
            using OutDatas   = TupleFilter<OutDataFilter>::FilteredType<ArgsType>;
            using Buffers    = TupleFilter<BufferFilter>::FilteredType<ArgsType>;
            using InHandles  = TupleFilter<InHandleFilter>::FilteredType<ArgsType>;
            using OutHandles = TupleFilter<OutHandleFilter>::FilteredType<ArgsType>;
            using InObjects  = TupleFilter<InObjectFilter>::FilteredType<ArgsType>;
            using OutObjects = TupleFilter<OutObjectFilter>::FilteredType<ArgsType>;

            /* Not kept separate from InDatas when processing, for reasons. */
            using InProcessIdHolders = TupleFilter<ProcessIdHolderFilter>::FilteredType<InDatas>;
            /* TODO: Support OutProcessIdHolders? */

            static constexpr size_t NumInDatas    = std::tuple_size<InDatas>::value;
            static constexpr size_t NumOutDatas   = std::tuple_size<OutDatas>::value;
            static constexpr size_t NumBuffers    = std::tuple_size<Buffers>::value;
            static constexpr size_t NumInHandles  = std::tuple_size<InHandles>::value;
            static constexpr size_t NumOutHandles = std::tuple_size<OutHandles>::value;
            static constexpr size_t NumInObjects  = std::tuple_size<InObjects>::value;
            static constexpr size_t NumOutObjects = std::tuple_size<OutObjects>::value;

            static constexpr size_t NumInProcessIdHolders  = std::tuple_size<InProcessIdHolders>::value;
            static constexpr bool   HasInProcessIdHolder   = NumInProcessIdHolders >= 1;

            template<typename T>
            static constexpr bool IsInProcessIdHolder = GetArgumentType<T> == ArgumentType::InData && std::is_base_of<sf::impl::ProcessIdHolder, T>::value;

            template<size_t I>
            static constexpr bool IsInProcessIdHolderIndex = I < std::tuple_size<ArgsType>::value && IsInProcessIdHolder<typename std::tuple_element<I, ArgsType>::type>;

            static_assert(NumBuffers <= 8, "Methods must take in <= 8 Buffers");
            static_assert(NumInHandles <= 8, "Methods must take in <= 8 Handles");
            static_assert(NumOutHandles + NumOutObjects <= 8, "Methods must output <= 8 Handles");

            /* Buffer marshalling. */
            static constexpr std::array<u32, NumBuffers> BufferAttributes = BufferAttributeArrayGetter<Buffers>::value;
            static constexpr size_t NumInHipcMapAliasBuffers              = BufferAttributeCounter<InHipcMapAliasBufferPredicate>::GetCount(BufferAttributes);
            static constexpr size_t NumOutHipcMapAliasBuffers             = BufferAttributeCounter<OutHipcMapAliasBufferPredicate>::GetCount(BufferAttributes);
            static constexpr size_t NumInHipcPointerBuffers               = BufferAttributeCounter<InHipcPointerBufferPredicate>::GetCount(BufferAttributes);
            static constexpr size_t NumOutHipcPointerBuffers              = BufferAttributeCounter<OutHipcPointerBufferPredicate>::GetCount(BufferAttributes);
            static constexpr size_t NumFixedSizeOutHipcPointerBuffers     = BufferAttributeCounter<FixedSizeOutHipcPointerBufferPredicate>::GetCount(BufferAttributes);
            static constexpr size_t NumUnfixedSizeOutHipcPointerBuffers   = NumOutHipcPointerBuffers - NumFixedSizeOutHipcPointerBuffers;

            /* In/Out data marshalling. */
            static constexpr std::array<size_t, NumInDatas+1> InDataOffsets = RawDataOffsetCalculator<InDatas>::Offsets;
            static constexpr size_t InDataSize  = util::AlignUp(InDataOffsets[NumInDatas], alignof(u16));

            static constexpr std::array<size_t, NumOutDatas+1> OutDataOffsets = RawDataOffsetCalculator<OutDatas>::Offsets;
            static constexpr size_t OutDataSize = util::AlignUp(OutDataOffsets[NumOutDatas], alignof(u32));
            static constexpr size_t OutDataAlign = [] {
                if constexpr (std::tuple_size<OutDatas>::value) {
                    return alignof(typename std::tuple_element<0, OutDatas>::type);
                }
                return size_t();
            }();

            /* Handle marshalling. */
            static constexpr size_t NumInMoveHandles = std::tuple_size<TupleFilter<InMoveHandleFilter>::FilteredType<InHandles>>::value;
            static constexpr size_t NumInCopyHandles = std::tuple_size<TupleFilter<InCopyHandleFilter>::FilteredType<InHandles>>::value;

            static constexpr size_t NumOutMoveHandles = std::tuple_size<TupleFilter<OutMoveHandleFilter>::FilteredType<OutHandles>>::value;
            static constexpr size_t NumOutCopyHandles = std::tuple_size<TupleFilter<OutCopyHandleFilter>::FilteredType<OutHandles>>::value;

            static_assert(NumInMoveHandles + NumInCopyHandles   == NumInHandles,  "NumInMoveHandles + NumInCopyHandles   == NumInHandles");
            static_assert(NumOutMoveHandles + NumOutCopyHandles == NumOutHandles, "NumOutMoveHandles + NumOutCopyHandles == NumOutHandles");

            /* Used by server message processor at runtime. */
            static constexpr inline const cmif::ServerMessageRuntimeMetadata RuntimeMetadata = cmif::ServerMessageRuntimeMetadata{
                .in_data_size      = InDataSize,
                .out_data_size     = OutDataSize,
                .in_headers_size   = sizeof(CmifInHeader),
                .out_headers_size  = sizeof(CmifOutHeader),
                .in_object_count   = NumInObjects,
                .out_object_count  = NumOutObjects,
            };

        /* Construction of argument serialization structs. */
        private:
            template<typename>
            struct ArgumentSerializationInfoConstructor;

            template<typename ...Ts>
            struct ArgumentSerializationInfoConstructor<std::tuple<Ts...>> {

                template<typename T>
                static constexpr ArgumentSerializationInfo ProcessUpdate(ArgumentSerializationInfo &current_info) {
                    /* Save a copy of the current state to return. */
                    ArgumentSerializationInfo returned_info = current_info;

                    /* Clear previous iteration's fixed size. */
                    returned_info.fixed_size = 0;
                    current_info.fixed_size = 0;

                    constexpr auto arg_type = GetArgumentType<T>;
                    returned_info.arg_type = arg_type;
                    if constexpr (arg_type == ArgumentType::InData) {
                        /* New rawdata, so increment index. */
                        current_info.in_raw_data_index++;
                    } else if constexpr (arg_type == ArgumentType::OutData) {
                        /* New rawdata, so increment index. */
                        current_info.out_raw_data_index++;
                    } else if constexpr (arg_type == ArgumentType::InHandle) {
                        /* New InHandle, increment the appropriate index. */
                        if constexpr (std::is_same<T, sf::MoveHandle>::value) {
                            current_info.in_move_handle_index++;
                        } else if constexpr (std::is_same<T, sf::CopyHandle>::value) {
                            current_info.in_copy_handle_index++;
                        } else {
                            static_assert(!std::is_same<T, T>::value, "Invalid InHandle kind");
                        }
                    } else if constexpr (arg_type == ArgumentType::OutHandle) {
                        /* New OutHandle, increment the appropriate index. */
                        if constexpr (std::is_same<T, sf::Out<sf::MoveHandle>>::value) {
                            current_info.out_move_handle_index++;
                        } else if constexpr (std::is_same<T, sf::Out<sf::CopyHandle>>::value) {
                            current_info.out_copy_handle_index++;
                        } else {
                            static_assert(!std::is_same<T, T>::value, "Invalid OutHandle kind");
                        }
                    } else if constexpr (arg_type == ArgumentType::InObject) {
                        /* New InObject, increment the appropriate index. */
                        current_info.in_object_index++;
                    } else if constexpr (arg_type == ArgumentType::OutObject) {
                        /* New OutObject, increment the appropriate index. */
                        current_info.out_object_index++;
                    } else if constexpr (arg_type == ArgumentType::Buffer) {
                        /* New Buffer, increment the appropriate index. */
                        const auto attributes = BufferAttributes[current_info.buffer_index];
                        current_info.buffer_index++;

                        if (attributes & SfBufferAttr_HipcMapAlias) {
                            if (attributes & SfBufferAttr_In) {
                                current_info.send_map_alias_index++;
                            } else if (attributes & SfBufferAttr_Out) {
                                current_info.recv_map_alias_index++;
                            }
                        } else if (attributes & SfBufferAttr_HipcPointer) {
                            if (attributes & SfBufferAttr_In) {
                                current_info.send_pointer_index++;
                            } else if (attributes & SfBufferAttr_Out) {
                                current_info.recv_pointer_index++;
                                if (!(attributes & SfBufferAttr_FixedSize)) {
                                    current_info.unfixed_recv_pointer_index++;
                                } else {
                                    returned_info.fixed_size = LargeDataSize<T>;
                                }
                            }
                        } else if (attributes & SfBufferAttr_HipcAutoSelect) {
                            if (attributes & SfBufferAttr_In) {
                                current_info.send_map_alias_index++;
                                current_info.send_pointer_index++;
                            } else if (attributes & SfBufferAttr_Out) {
                                current_info.recv_map_alias_index++;
                                current_info.recv_pointer_index++;
                                if (!(attributes & SfBufferAttr_FixedSize)) {
                                    current_info.unfixed_recv_pointer_index++;
                                } else {
                                    returned_info.fixed_size = LargeDataSize<T>;
                                }
                            }
                        }
                    } else {
                        static_assert(!std::is_same<T, T>::value, "Invalid ArgumentType<T>");
                    }

                    return returned_info;
                }

                static constexpr std::array<ArgumentSerializationInfo, sizeof...(Ts)> ArgumentSerializationInfos = [] {
                    ArgumentSerializationInfo current_info = {};

                    return std::array<ArgumentSerializationInfo, sizeof...(Ts)>{ ProcessUpdate<Ts>(current_info)... };
                }();

            };
        public:
            static constexpr std::array<ArgumentSerializationInfo, std::tuple_size<ArgsType>::value> ArgumentSerializationInfos = ArgumentSerializationInfoConstructor<ArgsType>::ArgumentSerializationInfos;
    };

    template<size_t _Size, size_t _Align>
    class OutRawHolder {
        public:
            static constexpr size_t Size = _Size;
            static constexpr size_t Align = _Align ? _Align : alignof(u8);
        private:
            alignas(Align) u8 data[Size];
        public:
            constexpr OutRawHolder() : data() { /* ... */ }

            template<size_t Offset, size_t TypeSize>
            constexpr inline uintptr_t GetAddress() const {
                static_assert(Offset <= Size, "Offset <= Size");
                static_assert(TypeSize <= Size, "TypeSize <= Size");
                static_assert(Offset + TypeSize <= Size, "Offset + TypeSize <= Size");
                return reinterpret_cast<uintptr_t>(&data[Offset]);
            }

            constexpr inline void CopyTo(void *dst) const {
                if constexpr (Size > 0) {
                    std::memcpy(dst, data, Size);
                }
            }
    };

    template<size_t _NumMove, size_t _NumCopy>
    class OutHandleHolder {
        public:
            static constexpr size_t NumMove = _NumMove;
            static constexpr size_t NumCopy = _NumCopy;
        private:
            MoveHandle move_handles[NumMove];
            CopyHandle copy_handles[NumCopy];
        public:
            constexpr OutHandleHolder() : move_handles(), copy_handles() { /* ... */ }

            template<size_t Index>
            constexpr inline MoveHandle *GetMoveHandlePointer() {
                static_assert(Index < NumMove, "Index < NumMove");
                return &move_handles[Index];
            }

            template<size_t Index>
            constexpr inline CopyHandle *GetCopyHandlePointer() {
                static_assert(Index < NumCopy, "Index < NumCopy");
                return &copy_handles[Index];
            }

            constexpr inline void CopyTo(const HipcRequest &response, const size_t num_out_object_handles) {
                #define _SF_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(n) do { if constexpr (NumCopy > n) { response.copy_handles[n] = copy_handles[n].GetValue(); } } while (0)
                _SF_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(0);
                _SF_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(1);
                _SF_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(2);
                _SF_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(3);
                _SF_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(4);
                _SF_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(5);
                _SF_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(6);
                _SF_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(7);
                #undef _SF_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE
                #define _SF_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(n) do { if constexpr (NumMove > n) { response.move_handles[n + num_out_object_handles] = move_handles[n].GetValue(); } } while (0)
                _SF_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(0);
                _SF_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(1);
                _SF_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(2);
                _SF_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(3);
                _SF_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(4);
                _SF_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(5);
                _SF_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(6);
                _SF_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(7);
                #undef _SF_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE
            }
    };

    template<size_t NumInObjects, size_t NumOutObjects>
    class InOutObjectHolder {
        private:
            std::array<cmif::ServiceObjectHolder, NumInObjects> in_object_holders;
            std::array<cmif::ServiceObjectHolder, NumOutObjects> out_object_holders;
            std::array<cmif::DomainObjectId, NumOutObjects> out_object_ids;
        public:
            constexpr InOutObjectHolder() : in_object_holders(), out_object_holders() {
                #define _SF_IN_OUT_HOLDER_INITIALIZE_OBJECT_ID(n) if constexpr (NumOutObjects > n) { this->out_object_ids[n] = cmif::InvalidDomainObjectId; }
                _SF_IN_OUT_HOLDER_INITIALIZE_OBJECT_ID(0)
                _SF_IN_OUT_HOLDER_INITIALIZE_OBJECT_ID(1)
                _SF_IN_OUT_HOLDER_INITIALIZE_OBJECT_ID(2)
                _SF_IN_OUT_HOLDER_INITIALIZE_OBJECT_ID(3)
                _SF_IN_OUT_HOLDER_INITIALIZE_OBJECT_ID(4)
                _SF_IN_OUT_HOLDER_INITIALIZE_OBJECT_ID(5)
                _SF_IN_OUT_HOLDER_INITIALIZE_OBJECT_ID(6)
                _SF_IN_OUT_HOLDER_INITIALIZE_OBJECT_ID(7)
                #undef _SF_IN_OUT_HOLDER_INITIALIZE_OBJECT_ID
            }

            Result GetInObjects(const sf::cmif::ServerMessageProcessor *processor) {
                if constexpr (NumInObjects > 0) {
                    R_TRY(processor->GetInObjects(this->in_object_holders.data()));
                }
                return ResultSuccess();
            }

            template<typename ServiceImplTuple>
            constexpr inline Result ValidateInObjects() const {
                static_assert(std::tuple_size<ServiceImplTuple>::value == NumInObjects);
                #define _SF_IN_OUT_HOLDER_VALIDATE_IN_OBJECT(n) do { \
                    if constexpr (NumInObjects > n) { \
                        using SharedPtrToServiceImplType = typename std::tuple_element<n, ServiceImplTuple>::type; \
                        using ServiceImplType = typename SharedPtrToServiceImplType::element_type; \
                        R_UNLESS((this->in_object_holders[n].template IsServiceObjectValid<ServiceImplType>()), sf::cmif::ResultInvalidInObject()); \
                    } \
                } while (0)
                _SF_IN_OUT_HOLDER_VALIDATE_IN_OBJECT(0);
                _SF_IN_OUT_HOLDER_VALIDATE_IN_OBJECT(1);
                _SF_IN_OUT_HOLDER_VALIDATE_IN_OBJECT(2);
                _SF_IN_OUT_HOLDER_VALIDATE_IN_OBJECT(3);
                _SF_IN_OUT_HOLDER_VALIDATE_IN_OBJECT(4);
                _SF_IN_OUT_HOLDER_VALIDATE_IN_OBJECT(5);
                _SF_IN_OUT_HOLDER_VALIDATE_IN_OBJECT(6);
                _SF_IN_OUT_HOLDER_VALIDATE_IN_OBJECT(7);
                #undef _SF_IN_OUT_HOLDER_VALIDATE_IN_OBJECT
                return ResultSuccess();
            }

            template<size_t Index, typename ServiceImpl>
            std::shared_ptr<ServiceImpl> GetInObject() const {
                /* We know from ValidateInObjects() that this will succeed always. */
                return this->in_object_holders[Index].template GetServiceObject<ServiceImpl>();
            }

            template<size_t Index, typename ServiceImpl>
            Out<std::shared_ptr<ServiceImpl>> GetOutObject() {
                return Out<std::shared_ptr<ServiceImpl>>(&this->out_object_holders[Index], &this->out_object_ids[Index]);
            }

            constexpr void SetOutObjects(const cmif::ServiceDispatchContext &ctx, const HipcRequest &response) {
                if constexpr (NumOutObjects > 0) {
                    ctx.processor->SetOutObjects(ctx, response, this->out_object_holders.data(), this->out_object_ids.data());
                }
            }
    };

    class HipcCommandProcessorCommon : public sf::cmif::ServerMessageProcessor {
        public:
            virtual void SetImplementationProcessor(sf::cmif::ServerMessageProcessor *) override final { /* ... */ }

            virtual void PrepareForErrorReply(const cmif::ServiceDispatchContext &ctx, cmif::PointerAndSize &out_raw_data, const cmif::ServerMessageRuntimeMetadata runtime_metadata) override final {
                const size_t raw_size = runtime_metadata.GetOutHeadersSize();
                const auto response = hipcMakeRequestInline(ctx.out_message_buffer.GetPointer(),
                    .type = CmifCommandType_Invalid, /* Really response */
                    .num_data_words = static_cast<u32>((util::AlignUp(raw_size, 0x4) + 0x10 /* padding */) / sizeof(u32)),
                );
                out_raw_data = cmif::PointerAndSize(util::AlignUp(reinterpret_cast<uintptr_t>(response.data_words), 0x10), raw_size);
            }

            virtual Result GetInObjects(cmif::ServiceObjectHolder *in_objects) const override final {
                /* By default, InObjects aren't supported. */
                return sf::ResultNotSupported();
            }
    };

    template<typename CommandMeta>
    struct HipcCommandProcessor : public HipcCommandProcessorCommon {
        public:
            virtual const cmif::ServerMessageRuntimeMetadata GetRuntimeMetadata() const override final {
                return CommandMeta::RuntimeMetadata;
            }

            virtual Result PrepareForProcess(const cmif::ServiceDispatchContext &ctx, const cmif::ServerMessageRuntimeMetadata runtime_metadata) const override final {
                const auto &meta = ctx.request.meta;
                bool is_request_valid = true;
                is_request_valid &= meta.send_pid         == CommandMeta::HasInProcessIdHolder;
                is_request_valid &= meta.num_send_statics == CommandMeta::NumInHipcPointerBuffers;
             /* is_request_valid &= meta.num_recv_statics == CommandMeta::NumOutHipcPointerBuffers; */
                is_request_valid &= meta.num_send_buffers == CommandMeta::NumInHipcMapAliasBuffers;
                is_request_valid &= meta.num_recv_buffers == CommandMeta::NumOutHipcMapAliasBuffers;
                is_request_valid &= meta.num_exch_buffers == 0; /* Exchange buffers aren't supported. */
                is_request_valid &= meta.num_copy_handles == CommandMeta::NumInCopyHandles;
                is_request_valid &= meta.num_move_handles == CommandMeta::NumInMoveHandles;

                const size_t meta_raw_size = meta.num_data_words * sizeof(u32);
                const size_t command_raw_size = util::AlignUp(runtime_metadata.GetUnfixedOutPointerSizeOffset() + (CommandMeta::NumUnfixedSizeOutHipcPointerBuffers * sizeof(u16)), alignof(u32));
                is_request_valid &= meta_raw_size >= command_raw_size;

                R_UNLESS(is_request_valid, sf::hipc::ResultInvalidCmifRequest());
                return ResultSuccess();
            }

            virtual HipcRequest PrepareForReply(const cmif::ServiceDispatchContext &ctx, cmif::PointerAndSize &out_raw_data, const cmif::ServerMessageRuntimeMetadata runtime_metadata) override final {
                const size_t raw_size = runtime_metadata.GetOutDataSize() + runtime_metadata.GetOutHeadersSize();
                const auto response = hipcMakeRequestInline(ctx.out_message_buffer.GetPointer(),
                    .type = CmifCommandType_Invalid, /* Really response */
                    .num_send_statics = CommandMeta::NumOutHipcPointerBuffers,
                    .num_data_words = static_cast<u32>((util::AlignUp(raw_size, 0x4) + 0x10 /* padding */) / sizeof(u32)),
                    .num_copy_handles = CommandMeta::NumOutCopyHandles,
                    .num_move_handles = static_cast<u32>(CommandMeta::NumOutMoveHandles + runtime_metadata.GetOutObjectCount()),
                );
                out_raw_data = cmif::PointerAndSize(util::AlignUp(reinterpret_cast<uintptr_t>(response.data_words), 0x10), raw_size);
                return response;
            }

            virtual void SetOutObjects(const cmif::ServiceDispatchContext &ctx, const HipcRequest &response, cmif::ServiceObjectHolder *out_objects, cmif::DomainObjectId *ids) override final {
                #define _SF_IMPL_PROCESSOR_SET_OUT_OBJECT_IMPL(n) do { if constexpr (CommandMeta::NumOutObjects > n) { SetOutObjectImpl<n>(response, ctx.manager, std::move(out_objects[n])); } } while (0)
                _SF_IMPL_PROCESSOR_SET_OUT_OBJECT_IMPL(0);
                _SF_IMPL_PROCESSOR_SET_OUT_OBJECT_IMPL(1);
                _SF_IMPL_PROCESSOR_SET_OUT_OBJECT_IMPL(2);
                _SF_IMPL_PROCESSOR_SET_OUT_OBJECT_IMPL(3);
                _SF_IMPL_PROCESSOR_SET_OUT_OBJECT_IMPL(4);
                _SF_IMPL_PROCESSOR_SET_OUT_OBJECT_IMPL(5);
                _SF_IMPL_PROCESSOR_SET_OUT_OBJECT_IMPL(6);
                _SF_IMPL_PROCESSOR_SET_OUT_OBJECT_IMPL(7);
                #undef _SF_IMPL_PROCESSOR_SET_OUT_OBJECT_IMPL
            }

        /* Useful defines. */
        using ArgsType = typename CommandMeta::ArgsType;
        using BufferArrayType = std::array<cmif::PointerAndSize, CommandMeta::NumBuffers>;
        using OutRawHolderType = OutRawHolder<CommandMeta::OutDataSize, CommandMeta::OutDataAlign>;
        using OutHandleHolderType = OutHandleHolder<CommandMeta::NumOutMoveHandles, CommandMeta::NumOutCopyHandles>;
        using InOutObjectHolderType = InOutObjectHolder<CommandMeta::NumInObjects, CommandMeta::NumOutObjects>;

        /* Buffer processing. */
        private:
            template<size_t Index>
            NX_CONSTEXPR void SetOutObjectImpl(const HipcRequest &response, hipc::ServerSessionManager *manager, cmif::ServiceObjectHolder &&object) {
                /* If no object, write INVALID_HANDLE. This is what official software does. */
                if (!object) {
                    response.move_handles[Index] = INVALID_HANDLE;
                    return;
                }
                Handle server_handle, client_handle;
                R_ABORT_UNLESS(sf::hipc::CreateSession(&server_handle, &client_handle));
                R_ABORT_UNLESS(manager->RegisterSession(server_handle, std::move(object)));
                response.move_handles[Index] = client_handle;
            }

            template<u32 Attributes>
            NX_CONSTEXPR bool IsMapTransferModeValid(u32 mode) {
                static_assert(!((Attributes & SfBufferAttr_HipcMapTransferAllowsNonSecure) && (Attributes & SfBufferAttr_HipcMapTransferAllowsNonDevice)), "Invalid Attributes");
                if constexpr (Attributes & SfBufferAttr_HipcMapTransferAllowsNonSecure) {
                    return mode == HipcBufferMode_NonSecure;
                } else if constexpr (Attributes & SfBufferAttr_HipcMapTransferAllowsNonDevice) {
                    return mode == HipcBufferMode_NonDevice;
                } else {
                    return mode == HipcBufferMode_Normal;
                }
            }

            template<size_t BufferIndex>
            static constexpr inline size_t GetIndexFromBufferIndex = [] {
                for (size_t i = 0; i < CommandMeta::ArgumentSerializationInfos.size(); i++) {
                    const auto Info = CommandMeta::ArgumentSerializationInfos[i];
                    if (Info.arg_type == ArgumentType::Buffer && Info.buffer_index == BufferIndex) {
                        return i;
                    }
                }
                return std::numeric_limits<size_t>::max();
            }();

            template<size_t BufferIndex, size_t Index = GetIndexFromBufferIndex<BufferIndex>>
            NX_CONSTEXPR void ProcessBufferImpl(const cmif::ServiceDispatchContext &ctx, cmif::PointerAndSize &buffer, bool &is_buffer_map_alias, bool &map_alias_buffers_valid, size_t &pointer_buffer_head, size_t &pointer_buffer_tail, const cmif::ServerMessageRuntimeMetadata runtime_metadata) {
                static_assert(Index != std::numeric_limits<size_t>::max(), "Invalid Index From Buffer Index");
                constexpr auto Info = CommandMeta::ArgumentSerializationInfos[Index];
                constexpr auto Attributes = CommandMeta::BufferAttributes[BufferIndex];
                static_assert(BufferIndex == Info.buffer_index && Info.arg_type == ArgumentType::Buffer, "BufferIndex == Info.buffer_index && Info.arg_type == ArgumentType::Buffer");

                if constexpr (Attributes & SfBufferAttr_HipcMapAlias) {
                    is_buffer_map_alias = true;
                    if constexpr (Attributes & SfBufferAttr_In) {
                        const HipcBufferDescriptor *desc = &ctx.request.data.send_buffers[Info.send_map_alias_index];
                        buffer = cmif::PointerAndSize(hipcGetBufferAddress(desc), hipcGetBufferSize(desc));
                        if (!IsMapTransferModeValid<Attributes>(static_cast<u32>(desc->mode))) { map_alias_buffers_valid = false; }
                    } else if constexpr (Attributes & SfBufferAttr_Out) {
                        const HipcBufferDescriptor *desc = &ctx.request.data.recv_buffers[Info.recv_map_alias_index];
                        buffer = cmif::PointerAndSize(hipcGetBufferAddress(desc), hipcGetBufferSize(desc));
                        if (!IsMapTransferModeValid<Attributes>(static_cast<u32>(desc->mode))) { map_alias_buffers_valid = false; }
                    } else {
                        static_assert(Attributes != Attributes, "Invalid Buffer Attributes");
                    }
                } else if constexpr (Attributes & SfBufferAttr_HipcPointer) {
                    is_buffer_map_alias = false;
                    if constexpr (Attributes & SfBufferAttr_In) {
                        const HipcStaticDescriptor *desc = &ctx.request.data.send_statics[Info.send_pointer_index];
                        buffer = cmif::PointerAndSize(hipcGetStaticAddress(desc), hipcGetStaticSize(desc));
                        const size_t size = buffer.GetSize();
                        if (size) {
                            pointer_buffer_tail = std::max(pointer_buffer_tail, buffer.GetAddress() + size);
                        }
                    } else if constexpr (Attributes & SfBufferAttr_Out) {
                        if constexpr (Attributes & SfBufferAttr_FixedSize) {
                            constexpr size_t size = Info.fixed_size;
                            static_assert(size > 0, "FixedSize object must have non-zero size!");
                            pointer_buffer_head = util::AlignDown(pointer_buffer_head - size, 0x10);
                            buffer = cmif::PointerAndSize(pointer_buffer_head, size);
                        } else {
                            const u16 *recv_pointer_sizes = reinterpret_cast<const u16 *>(reinterpret_cast<uintptr_t>(ctx.request.data.data_words) + runtime_metadata.GetUnfixedOutPointerSizeOffset());
                            const size_t size = size_t(recv_pointer_sizes[Info.unfixed_recv_pointer_index]);
                            pointer_buffer_head = util::AlignDown(pointer_buffer_head - size, 0x10);
                            buffer = cmif::PointerAndSize(pointer_buffer_head, size);
                        }
                    } else {
                        static_assert(Attributes != Attributes, "Invalid Buffer Attributes");
                    }
                } else if constexpr (Attributes & SfBufferAttr_HipcAutoSelect) {
                    if constexpr (Attributes & SfBufferAttr_In) {
                        const HipcBufferDescriptor *map_desc = &ctx.request.data.send_buffers[Info.send_map_alias_index];
                        const HipcStaticDescriptor *ptr_desc = &ctx.request.data.send_statics[Info.send_pointer_index];
                        is_buffer_map_alias = hipcGetBufferAddress(map_desc) != 0;
                        if (is_buffer_map_alias) {
                            buffer = cmif::PointerAndSize(hipcGetBufferAddress(map_desc), hipcGetBufferSize(map_desc));
                            if (!IsMapTransferModeValid<Attributes>(static_cast<u32>(map_desc->mode))) { map_alias_buffers_valid = false; }
                        } else {
                            buffer = cmif::PointerAndSize(hipcGetStaticAddress(ptr_desc), hipcGetStaticSize(ptr_desc));
                            const size_t size = buffer.GetSize();
                            if (size) {
                                pointer_buffer_tail = std::max(pointer_buffer_tail, buffer.GetAddress() + size);
                            }
                        }
                    } else if constexpr (Attributes & SfBufferAttr_Out) {
                        const HipcBufferDescriptor *map_desc = &ctx.request.data.recv_buffers[Info.recv_map_alias_index];
                        is_buffer_map_alias = hipcGetBufferAddress(map_desc) != 0;
                        if (is_buffer_map_alias) {
                            buffer = cmif::PointerAndSize(hipcGetBufferAddress(map_desc), hipcGetBufferSize(map_desc));
                        if (!IsMapTransferModeValid<Attributes>(static_cast<u32>(map_desc->mode))) { map_alias_buffers_valid = false; }
                        } else {
                            if constexpr (Attributes & SfBufferAttr_FixedSize) {
                                constexpr size_t size = Info.fixed_size;
                                static_assert(size > 0, "FixedSize object must have non-zero size!");
                                pointer_buffer_head = util::AlignDown(pointer_buffer_head - size, 0x10);
                                buffer = cmif::PointerAndSize(pointer_buffer_head, size);
                            } else {
                                const u16 *recv_pointer_sizes = reinterpret_cast<const u16 *>(reinterpret_cast<uintptr_t>(ctx.request.data.data_words) + runtime_metadata.GetUnfixedOutPointerSizeOffset());
                                const size_t size = size_t(recv_pointer_sizes[Info.unfixed_recv_pointer_index]);
                                pointer_buffer_head = util::AlignDown(pointer_buffer_head - size, 0x10);
                                buffer = cmif::PointerAndSize(pointer_buffer_head, size);
                            }
                        }
                    } else {
                        static_assert(Attributes != Attributes, "Invalid Buffer Attributes");
                    }
                } else {
                    static_assert(Attributes != Attributes, "Invalid Buffer Attributes");
                }
            }

            template<size_t BufferIndex, size_t Index = GetIndexFromBufferIndex<BufferIndex>>
            NX_CONSTEXPR void SetOutBufferImpl(const HipcRequest &response, const cmif::PointerAndSize &buffer, const bool is_buffer_map_alias) {
                static_assert(Index != std::numeric_limits<size_t>::max(), "Invalid Index From Buffer Index");
                constexpr auto Info = CommandMeta::ArgumentSerializationInfos[Index];
                constexpr auto Attributes = CommandMeta::BufferAttributes[BufferIndex];
                static_assert(BufferIndex == Info.buffer_index && Info.arg_type == ArgumentType::Buffer, "BufferIndex == Info.buffer_index && Info.arg_type == ArgumentType::Buffer");

                if constexpr (Attributes & SfBufferAttr_Out) {
                    if constexpr (Attributes & SfBufferAttr_HipcPointer) {
                        response.send_statics[Info.recv_pointer_index] = hipcMakeSendStatic(buffer.GetPointer(), buffer.GetSize(), Info.recv_pointer_index);
                    } else if constexpr (Attributes & SfBufferAttr_HipcAutoSelect) {
                        if (!is_buffer_map_alias) {
                            response.send_statics[Info.recv_pointer_index] = hipcMakeSendStatic(buffer.GetPointer(), buffer.GetSize(), Info.recv_pointer_index);
                        } else {
                            response.send_statics[Info.recv_pointer_index] = hipcMakeSendStatic(nullptr, 0, Info.recv_pointer_index);
                        }
                    }
                }
            }
        public:
            NX_CONSTEXPR Result ProcessBuffers(const cmif::ServiceDispatchContext &ctx, BufferArrayType &buffers, std::array<bool, CommandMeta::NumBuffers> &is_buffer_map_alias, const cmif::ServerMessageRuntimeMetadata runtime_metadata) {
                bool map_alias_buffers_valid = true;
                size_t pointer_buffer_tail = ctx.pointer_buffer.GetAddress();
                size_t pointer_buffer_head = pointer_buffer_tail + ctx.pointer_buffer.GetSize();
                #define _SF_IMPL_PROCESSOR_PROCESS_BUFFER_IMPL(n) do { if constexpr (CommandMeta::NumBuffers > n) { ProcessBufferImpl<n>(ctx, buffers[n], is_buffer_map_alias[n], map_alias_buffers_valid, pointer_buffer_head, pointer_buffer_tail, runtime_metadata); } } while (0)
                _SF_IMPL_PROCESSOR_PROCESS_BUFFER_IMPL(0);
                _SF_IMPL_PROCESSOR_PROCESS_BUFFER_IMPL(1);
                _SF_IMPL_PROCESSOR_PROCESS_BUFFER_IMPL(2);
                _SF_IMPL_PROCESSOR_PROCESS_BUFFER_IMPL(3);
                _SF_IMPL_PROCESSOR_PROCESS_BUFFER_IMPL(4);
                _SF_IMPL_PROCESSOR_PROCESS_BUFFER_IMPL(5);
                _SF_IMPL_PROCESSOR_PROCESS_BUFFER_IMPL(6);
                _SF_IMPL_PROCESSOR_PROCESS_BUFFER_IMPL(7);
                #undef _SF_IMPL_PROCESSOR_PROCESS_BUFFER_IMPL
                R_UNLESS(map_alias_buffers_valid, sf::hipc::ResultInvalidCmifRequest());
                if constexpr (CommandMeta::NumOutHipcPointerBuffers > 0) {
                    R_UNLESS(pointer_buffer_tail <= pointer_buffer_head, sf::hipc::ResultPointerBufferTooSmall());
                }
                return ResultSuccess();
            }

            NX_CONSTEXPR void SetOutBuffers(const HipcRequest &response, const BufferArrayType &buffers, const std::array<bool, CommandMeta::NumBuffers> &is_buffer_map_alias) {
                #define _SF_IMPL_PROCESSOR_SET_OUT_BUFFER_IMPL(n) do { if constexpr (CommandMeta::NumBuffers > n) { SetOutBufferImpl<n>(response, buffers[n], is_buffer_map_alias[n]); } } while (0)
                _SF_IMPL_PROCESSOR_SET_OUT_BUFFER_IMPL(0);
                _SF_IMPL_PROCESSOR_SET_OUT_BUFFER_IMPL(1);
                _SF_IMPL_PROCESSOR_SET_OUT_BUFFER_IMPL(2);
                _SF_IMPL_PROCESSOR_SET_OUT_BUFFER_IMPL(3);
                _SF_IMPL_PROCESSOR_SET_OUT_BUFFER_IMPL(4);
                _SF_IMPL_PROCESSOR_SET_OUT_BUFFER_IMPL(5);
                _SF_IMPL_PROCESSOR_SET_OUT_BUFFER_IMPL(6);
                _SF_IMPL_PROCESSOR_SET_OUT_BUFFER_IMPL(7);
                #undef _SF_IMPL_PROCESSOR_SET_OUT_BUFFER_IMPL
            }

        /* Argument deserialization. */
        private:
            template<size_t Index, typename T = typename std::tuple_element<Index, ArgsType>::type>
            NX_CONSTEXPR T DeserializeArgumentImpl(const cmif::ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data, const OutRawHolderType &out_raw_holder, const BufferArrayType &buffers, OutHandleHolderType &out_handles_holder, InOutObjectHolderType &in_out_objects_holder) {
                constexpr auto Info = CommandMeta::ArgumentSerializationInfos[Index];
                if constexpr (Info.arg_type == ArgumentType::InData) {
                    /* New in rawdata. */
                    constexpr size_t Offset = CommandMeta::InDataOffsets[Info.in_raw_data_index];
                    if constexpr (!std::is_same<T, bool>::value) {
                        return *reinterpret_cast<const T *>(in_raw_data.GetAddress() + Offset);
                    } else {
                        /* Special case bools. */
                        return *reinterpret_cast<const u8 *>(in_raw_data.GetAddress() + Offset) & 1;
                    }
                } else if constexpr (Info.arg_type == ArgumentType::OutData) {
                    /* New out rawdata. */
                    constexpr size_t Offset = CommandMeta::OutDataOffsets[Info.out_raw_data_index];
                    return T(out_raw_holder.template GetAddress<Offset, T::TypeSize>());
                } else if constexpr (Info.arg_type == ArgumentType::InHandle) {
                    /* New InHandle. */
                    if constexpr (std::is_same<T, sf::MoveHandle>::value) {
                        return T(ctx.request.data.move_handles[Info.in_move_handle_index]);
                    } else if constexpr (std::is_same<T, sf::CopyHandle>::value) {
                        return T(ctx.request.data.copy_handles[Info.in_copy_handle_index]);
                    } else {
                        static_assert(!std::is_same<T, T>::value, "Invalid InHandle kind");
                    }
                } else if constexpr (Info.arg_type == ArgumentType::OutHandle) {
                    /* New OutHandle. */
                    if constexpr (std::is_same<T, sf::Out<sf::MoveHandle>>::value) {
                        return T(out_handles_holder.template GetMoveHandlePointer<Info.out_move_handle_index>());
                    } else if constexpr (std::is_same<T, sf::Out<sf::CopyHandle>>::value) {
                        return T(out_handles_holder.template GetCopyHandlePointer<Info.out_copy_handle_index>());
                    } else {
                        static_assert(!std::is_same<T, T>::value, "Invalid OutHandle kind");
                    }
                } else if constexpr (Info.arg_type == ArgumentType::InObject) {
                    /* New InObject. */
                    return in_out_objects_holder.template GetInObject<Info.in_object_index, typename T::element_type>();
                } else if constexpr (Info.arg_type == ArgumentType::OutObject) {
                    /* New OutObject. */
                    return in_out_objects_holder.template GetOutObject<Info.out_object_index, typename T::ServiceImplType>();
                } else if constexpr (Info.arg_type == ArgumentType::Buffer) {
                    /* Buffers were already processed earlier. */
                    if constexpr (sf::IsLargeData<T>) {
                        /* Fake buffer. This is either InData or OutData, but serializing over buffers. */
                        constexpr auto Attributes = CommandMeta::BufferAttributes[Info.buffer_index];
                        if constexpr (Attributes & SfBufferAttr_In) {
                            /* TODO: AMS_ABORT_UNLESS()? N does not bother. */
                            return *reinterpret_cast<const T *>(buffers[Info.buffer_index].GetAddress());
                        } else if constexpr (Attributes & SfBufferAttr_Out) {
                            return T(buffers[Info.buffer_index]);
                        } else {
                            static_assert(!std::is_same<T, T>::value, "Invalid BufferAttributes for LargeData type.");
                        }
                    } else {
                        /* Actual buffer! */
                        return T(buffers[Info.buffer_index]);
                    }
                } else {
                    static_assert(!std::is_same<T, T>::value, "Invalid ArgumentType<T>");
                }
            }

            template<size_t... Is>
            NX_CONSTEXPR ArgsType DeserializeArgumentsImpl(const cmif::ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data, const OutRawHolderType &out_raw_holder, const BufferArrayType &buffers, OutHandleHolderType &out_handles_holder, InOutObjectHolderType &in_out_objects_holder, std::index_sequence<Is...>) {
                return ArgsType { DeserializeArgumentImpl<Is>(ctx, in_raw_data, out_raw_holder, buffers, out_handles_holder, in_out_objects_holder)..., };
            }
        public:
            NX_CONSTEXPR ArgsType DeserializeArguments(const cmif::ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data, const OutRawHolderType &out_raw_holder, const BufferArrayType &buffers, OutHandleHolderType &out_handles_holder, InOutObjectHolderType &in_out_objects_holder) {
                return DeserializeArgumentsImpl(ctx, in_raw_data, out_raw_holder, buffers, out_handles_holder, in_out_objects_holder, std::make_index_sequence<std::tuple_size<ArgsType>::value>{});
            }
    };

    constexpr Result GetCmifOutHeaderPointer(CmifOutHeader **out_header_ptr, cmif::PointerAndSize &out_raw_data) {
        CmifOutHeader *header = static_cast<CmifOutHeader *>(out_raw_data.GetPointer());
        R_UNLESS(out_raw_data.GetSize() >= sizeof(*header), sf::cmif::ResultInvalidHeaderSize());
        out_raw_data = cmif::PointerAndSize(out_raw_data.GetAddress() + sizeof(*header), out_raw_data.GetSize() - sizeof(*header));
        *out_header_ptr = header;
        return ResultSuccess();
    }

    template<auto ServiceCommandImpl, typename Return, typename ClassType, typename... Arguments>
    constexpr Result InvokeServiceCommandImpl(CmifOutHeader **out_header_ptr, cmif::ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) {
        using CommandMeta = CommandMetaInfo<ServiceCommandImpl, Return, ClassType, Arguments...>;
        using ImplProcessorType = HipcCommandProcessor<CommandMeta>;
        using BufferArrayType = std::array<cmif::PointerAndSize, CommandMeta::NumBuffers>;
        using OutHandleHolderType = OutHandleHolder<CommandMeta::NumOutMoveHandles, CommandMeta::NumOutCopyHandles>;
        using OutRawHolderType = OutRawHolder<CommandMeta::OutDataSize, CommandMeta::OutDataAlign>;
        using InOutObjectHolderType = InOutObjectHolder<CommandMeta::NumInObjects, CommandMeta::NumOutObjects>;
        static_assert(std::is_base_of<sf::IServiceObject, typename CommandMeta::ClassType>::value, "InvokeServiceCommandImpl: Service Commands must be ServiceObject member functions");

        /* Create a processor for us to work with. */
        ImplProcessorType impl_processor;
        if (ctx.processor == nullptr) {
            /* In the non-domain case, this is our only processor. */
            ctx.processor = &impl_processor;
        } else {
            /* In the domain case, we already have a processor, so we should give it a pointer to our template implementation. */
            ctx.processor->SetImplementationProcessor(&impl_processor);
        }

        /* Validate the metadata has the expected counts. */
        const auto runtime_metadata = ctx.processor->GetRuntimeMetadata();
        R_TRY(ctx.processor->PrepareForProcess(ctx, runtime_metadata));

        /* Storage for output. */
        BufferArrayType buffers;
        std::array<bool, CommandMeta::NumBuffers> is_buffer_map_alias = {};
        OutRawHolderType out_raw_holder;
        OutHandleHolderType out_handles_holder;
        InOutObjectHolderType in_out_objects_holder;

        /* Process buffers. */
        R_TRY(ImplProcessorType::ProcessBuffers(ctx, buffers, is_buffer_map_alias, runtime_metadata));

        /* Process input/output objects. */
        R_TRY(in_out_objects_holder.GetInObjects(ctx.processor));
        R_TRY((in_out_objects_holder.template ValidateInObjects<typename CommandMeta::InObjects>()));

        /* Decoding/Invocation. */
        {
            typename CommandMeta::ClassTypePointer this_ptr = static_cast<typename CommandMeta::ClassTypePointer>(ctx.srv_obj);
            typename CommandMeta::ArgsType args_tuple = ImplProcessorType::DeserializeArguments(ctx, in_raw_data, out_raw_holder, buffers, out_handles_holder, in_out_objects_holder);

            /* Handle in process ID holder if relevant. */
            if constexpr (CommandMeta::HasInProcessIdHolder) {
                /* TODO: More precise value than 32? */
                static_assert(std::tuple_size<typename CommandMeta::ArgsType>::value <= 32, "Commands must have <= 32 arguments");
                os::ProcessId process_id{ctx.request.pid};
                #define _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(n) do {                                         \
                    using ArgsType = typename CommandMeta::ArgsType;                                          \
                    if constexpr (n < std::tuple_size<ArgsType>::value) {                                     \
                        if constexpr (CommandMeta::template IsInProcessIdHolderIndex<n>) {                    \
                            R_TRY(MarshalProcessId(std::get<n>(args_tuple), process_id));                     \
                        }                                                                                     \
                    }                                                                                         \
                } while (0)
                _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x00); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x01); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x02); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x03);
                _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x04); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x05); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x06); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x07);
                _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x08); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x09); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x0a); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x0b);
                _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x0c); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x0d); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x0e); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x0f);
                _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x10); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x11); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x12); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x13);
                _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x14); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x15); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x16); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x17);
                _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x18); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x19); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x1a); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x1b);
                _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x1c); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x1d); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x1e); _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID(0x1f);
                #undef _SF_IMPL_PROCESSOR_MARSHAL_PROCESS_ID
            }

            if constexpr (CommandMeta::ReturnsResult) {
                const auto command_result = std::apply([=](auto&&... args) { return (this_ptr->*ServiceCommandImpl)(std::forward<Arguments>(args)...); }, args_tuple);
                if (R_FAILED(command_result)) {
                    cmif::PointerAndSize out_raw_data;
                    ctx.processor->PrepareForErrorReply(ctx, out_raw_data, runtime_metadata);
                    R_TRY(GetCmifOutHeaderPointer(out_header_ptr, out_raw_data));
                    return command_result;
                }
            } else {
                std::apply([=](auto&&... args) { (this_ptr->*ServiceCommandImpl)(std::forward<Arguments>(args)...); }, args_tuple);
            }
        }

        /* Encode. */
        cmif::PointerAndSize out_raw_data;
        const auto response = ctx.processor->PrepareForReply(ctx, out_raw_data, runtime_metadata);
        R_TRY(GetCmifOutHeaderPointer(out_header_ptr, out_raw_data));

        /* Copy raw data output struct. */
        R_UNLESS(out_raw_data.GetSize() >= OutRawHolderType::Size, sf::cmif::ResultInvalidOutRawSize());
        out_raw_holder.CopyTo(out_raw_data.GetPointer());

        /* Set output recvlist buffers. */
        ImplProcessorType::SetOutBuffers(response, buffers, is_buffer_map_alias);

        /* Set out handles. */
        out_handles_holder.CopyTo(response, runtime_metadata.GetOutObjectCount());

        /* Set output objects. */
        in_out_objects_holder.SetOutObjects(ctx, response);

        return ResultSuccess();
    }

}

namespace ams::sf::impl {

    template<hos::Version Low, hos::Version High, u32 CommandId, auto CommandImpl, typename Return, typename ClassType, typename... Arguments>
    consteval inline cmif::ServiceCommandMeta MakeServiceCommandMeta() {
        return {
            .hosver_low  = Low,
            .hosver_high = High,
            .cmd_id      = static_cast<u32>(CommandId),
            .handler     = ::ams::sf::impl::InvokeServiceCommandImpl<CommandImpl, Return, ClassType, Arguments...>,
        };
    }

}
