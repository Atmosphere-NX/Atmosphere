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
#include <stratosphere/tipc/tipc_common.hpp>
#include <stratosphere/tipc/tipc_out.hpp>
#include <stratosphere/tipc/tipc_buffers.hpp>
#include <stratosphere/tipc/tipc_handles.hpp>

namespace ams::tipc {

    struct ClientProcessId {
        os::ProcessId value;
    };
    static_assert(std::is_trivial<ClientProcessId>::value && sizeof(ClientProcessId) == sizeof(os::ProcessId), "ClientProcessId");

}

namespace ams::tipc::impl {

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
        ProcessId,
    };

    template<typename T>
    constexpr inline ArgumentType GetArgumentType = [] {
        if constexpr (tipc::IsBuffer<T>) {
            return ArgumentType::Buffer;
        } else if constexpr (std::same_as<T, tipc::CopyHandle> || std::same_as<T, tipc::MoveHandle>) {
            return ArgumentType::InHandle;
        } else if constexpr (std::same_as<T, tipc::OutCopyHandle> || std::same_as<T, tipc::OutMoveHandle>) {
            return ArgumentType::OutHandle;
        } else if constexpr (std::is_base_of<tipc::impl::OutBaseTag, T>::value) {
            return ArgumentType::OutData;
        } else if constexpr (std::same_as<T, tipc::ClientProcessId>) {
            return ArgumentType::ProcessId;
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
    using ProcessIdFilter = ArgumentTypeFilter<T, ArgumentType::ProcessId>;

    /* Handle kind filters. */
    template<typename T>
    using InMoveHandleFilter = TypeEqualityFilter<T, tipc::MoveHandle>;

    template<typename T>
    using InCopyHandleFilter = TypeEqualityFilter<T, tipc::CopyHandle>;

    template<typename T>
    using OutMoveHandleFilter = TypeEqualityFilter<T, tipc::OutMoveHandle>;

    template<typename T>
    using OutCopyHandleFilter = TypeEqualityFilter<T, tipc::OutCopyHandle>;

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

    static constexpr size_t InBufferPredicate(const u32 attribute) {
        return (attribute & SfBufferAttr_In) && !(attribute & SfBufferAttr_Out);
    }

    static constexpr size_t OutBufferPredicate(const u32 attribute) {
        return !(attribute & SfBufferAttr_In) && (attribute & SfBufferAttr_Out);
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

                    /* TODO: Is sorting done on parameters? */
                    /* Sort?(map, aligns); */

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
        bool is_send_buffer;
        size_t send_map_alias_index;
        size_t recv_map_alias_index;

        /* Handle indexing. */
        size_t in_move_handle_index;
        size_t in_copy_handle_index;
        size_t out_move_handle_index;
        size_t out_copy_handle_index;
    };

    template<u16 _CommandId, typename ArgumentsTuple>
    struct CommandMetaInfo;

    template<u16 _CommandId, typename... Arguments>
    struct CommandMetaInfo<_CommandId, std::tuple<Arguments...>> {
        public:
            static constexpr u16 CommandId = _CommandId;

            using ArgsType  = std::tuple<typename std::decay<Arguments>::type...>;

            using InDatas    = TupleFilter<InDataFilter>::FilteredType<ArgsType>;
            using OutDatas   = TupleFilter<OutDataFilter>::FilteredType<ArgsType>;
            using Buffers    = TupleFilter<BufferFilter>::FilteredType<ArgsType>;
            using InHandles  = TupleFilter<InHandleFilter>::FilteredType<ArgsType>;
            using OutHandles = TupleFilter<OutHandleFilter>::FilteredType<ArgsType>;
            using ProcessIds = TupleFilter<ProcessIdFilter>::FilteredType<ArgsType>;

            static constexpr size_t NumInDatas    = std::tuple_size<InDatas>::value;
            static constexpr size_t NumOutDatas   = std::tuple_size<OutDatas>::value;
            static constexpr size_t NumBuffers    = std::tuple_size<Buffers>::value;
            static constexpr size_t NumInHandles  = std::tuple_size<InHandles>::value;
            static constexpr size_t NumOutHandles = std::tuple_size<OutHandles>::value;

            static constexpr size_t NumProcessIds = std::tuple_size<ProcessIds>::value;
            static constexpr bool   HasProcessId  = NumProcessIds >= 1;

            static_assert(NumBuffers <= 8, "Methods must take in <= 8 Buffers");
            static_assert(NumInHandles <= 8, "Methods must take in <= 8 Handles");
            static_assert(NumOutHandles <= 8, "Methods must output <= 8 Handles");

            static_assert(NumInHandles == 0, "In Handles not yet implemented!");

            /* Buffer marshalling. */
            static constexpr std::array<u32, NumBuffers> BufferAttributes = BufferAttributeArrayGetter<Buffers>::value;
            static constexpr size_t NumInBuffers  = BufferAttributeCounter<InBufferPredicate>::GetCount(BufferAttributes);
            static constexpr size_t NumOutBuffers = BufferAttributeCounter<OutBufferPredicate>::GetCount(BufferAttributes);

            /* In/Out data marshalling. */
            static constexpr std::array<size_t, NumInDatas+1> InDataOffsets = RawDataOffsetCalculator<InDatas>::Offsets;
            static constexpr size_t InDataSize  = util::AlignUp(InDataOffsets[NumInDatas], alignof(u32));

            static constexpr std::array<size_t, NumOutDatas+1> OutDataOffsets = RawDataOffsetCalculator<OutDatas>::Offsets;
            static constexpr size_t OutDataSize = util::AlignUp(OutDataOffsets[NumOutDatas], alignof(u32));

            /* Useful because reasons. */
            static constexpr size_t OutDataAlign = []() -> size_t {
                if constexpr (std::tuple_size<OutDatas>::value) {
                    return alignof(typename std::tuple_element<0, OutDatas>::type);
                }
                return 0;
            }();

            /* Handle marshalling. */
            static constexpr size_t NumInMoveHandles = std::tuple_size<TupleFilter<InMoveHandleFilter>::FilteredType<InHandles>>::value;
            static constexpr size_t NumInCopyHandles = std::tuple_size<TupleFilter<InCopyHandleFilter>::FilteredType<InHandles>>::value;

            static constexpr size_t NumOutMoveHandles = std::tuple_size<TupleFilter<OutMoveHandleFilter>::FilteredType<OutHandles>>::value;
            static constexpr size_t NumOutCopyHandles = std::tuple_size<TupleFilter<OutCopyHandleFilter>::FilteredType<OutHandles>>::value;

            static_assert(NumInMoveHandles + NumInCopyHandles   == NumInHandles,  "NumInMoveHandles + NumInCopyHandles   == NumInHandles");
            static_assert(NumOutMoveHandles + NumOutCopyHandles == NumOutHandles, "NumOutMoveHandles + NumOutCopyHandles == NumOutHandles");

            /* tipc-specific accessors. */
            static constexpr bool HasInSpecialHeader = HasProcessId || NumInHandles > 0;

            static constexpr svc::ipc::MessageBuffer::MessageHeader InMessageHeader{CommandId, HasInSpecialHeader, 0, NumInBuffers, NumOutBuffers, 0, InDataSize / sizeof(u32), 0};
            static constexpr svc::ipc::MessageBuffer::SpecialHeader InSpecialHeader{HasProcessId, NumInCopyHandles, NumInMoveHandles, HasInSpecialHeader};

            static constexpr auto InMessageProcessIdIndex = svc::ipc::MessageBuffer::GetSpecialDataIndex(InMessageHeader, InSpecialHeader);
            static constexpr auto InMessageHandleIndex    = svc::ipc::MessageBuffer::GetSpecialDataIndex(InMessageHeader, InSpecialHeader) + (HasProcessId ? sizeof(u64) / sizeof(u32) : 0);
            static constexpr auto InMessageBufferIndex    = svc::ipc::MessageBuffer::GetMapAliasDescriptorIndex(InMessageHeader, InSpecialHeader);
            static constexpr auto InMessageRawDataIndex   = svc::ipc::MessageBuffer::GetRawDataIndex(InMessageHeader, InSpecialHeader);

            static constexpr bool HasOutSpecialHeader = NumOutHandles > 0;

            static constexpr svc::ipc::MessageBuffer::MessageHeader OutMessageHeader{CommandId, HasOutSpecialHeader, 0, 0, 0, 0, (OutDataSize / sizeof(u32)) + 1, 0};
            static constexpr svc::ipc::MessageBuffer::SpecialHeader OutSpecialHeader{false, NumOutCopyHandles, NumOutMoveHandles, HasOutSpecialHeader};

            static constexpr auto OutMessageHandleIndex  = svc::ipc::MessageBuffer::GetSpecialDataIndex(OutMessageHeader, OutSpecialHeader);
            static constexpr auto OutMessageResultIndex  = svc::ipc::MessageBuffer::GetRawDataIndex(OutMessageHeader, OutSpecialHeader);
            static constexpr auto OutMessageRawDataIndex = OutMessageResultIndex + 1;

            static constexpr size_t InMessageTotalSize  = (InMessageRawDataIndex  * sizeof(u32)) + InDataSize;
            static constexpr size_t OutMessageTotalSize = (OutMessageRawDataIndex * sizeof(u32)) + OutDataSize;

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
                        if constexpr (std::same_as<T, tipc::MoveHandle>) {
                            current_info.in_move_handle_index++;
                        } else if constexpr (std::same_as<T, tipc::CopyHandle>) {
                            current_info.in_copy_handle_index++;
                        } else {
                            static_assert(!std::is_same<T, T>::value, "Invalid InHandle kind");
                        }
                    } else if constexpr (arg_type == ArgumentType::OutHandle) {
                        /* New OutHandle, increment the appropriate index. */
                        if constexpr (std::same_as<T, tipc::OutMoveHandle>) {
                            current_info.out_move_handle_index++;
                        } else if constexpr (std::same_as<T, tipc::OutCopyHandle>) {
                            current_info.out_copy_handle_index++;
                        } else {
                            static_assert(!std::is_same<T, T>::value, "Invalid OutHandle kind");
                        }
                    } else if constexpr (arg_type == ArgumentType::Buffer) {
                        /* New Buffer, increment the appropriate index. */
                        const auto attributes = BufferAttributes[current_info.buffer_index++];

                        if (attributes & SfBufferAttr_In) {
                            returned_info.is_send_buffer = true;
                            current_info.send_map_alias_index++;
                        } else if (attributes & SfBufferAttr_Out) {
                            returned_info.is_send_buffer = false;
                            current_info.recv_map_alias_index++;
                        }
                    } else if constexpr (arg_type == ArgumentType::ProcessId) {
                        /* Nothing needs to be done to track process ids. */
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

    template<size_t _Size, size_t _Align, size_t OutIndex>
    class OutRawHolder {
        public:
            static constexpr size_t Size = _Size;
            static constexpr size_t Align = _Align ? _Align : alignof(u8);
        private:
            alignas(Align) u8 data[Size];
        public:
            constexpr ALWAYS_INLINE OutRawHolder() : data() { /* ... */ }

            template<size_t Offset, size_t TypeSize>
            constexpr ALWAYS_INLINE uintptr_t GetAddress() const {
                static_assert(Offset <= Size, "Offset <= Size");
                static_assert(TypeSize <= Size, "TypeSize <= Size");
                static_assert(Offset + TypeSize <= Size, "Offset + TypeSize <= Size");
                return reinterpret_cast<uintptr_t>(data + Offset);
            }

            constexpr ALWAYS_INLINE void CopyTo(const svc::ipc::MessageBuffer &buffer) const {
                if constexpr (Size > 0) {
                    buffer.SetRawArray(OutIndex, data, Size);
                }
            }
    };

    template<size_t _NumMove, size_t _NumCopy, size_t OutIndex>
    class OutHandleHolder {
        public:
            static constexpr size_t NumMove = _NumMove;
            static constexpr size_t NumCopy = _NumCopy;
        private:
            os::NativeHandle move_handles[NumMove];
            os::NativeHandle copy_handles[NumCopy];
        public:
            ALWAYS_INLINE OutHandleHolder() { /* ... */ }

            template<size_t Index>
            constexpr ALWAYS_INLINE os::NativeHandle *GetMoveHandlePointer() {
                static_assert(Index < NumMove, "Index < NumMove");
                return move_handles + Index;
            }

            template<size_t Index>
            constexpr ALWAYS_INLINE os::NativeHandle *GetCopyHandlePointer() {
                static_assert(Index < NumCopy, "Index < NumCopy");
                return copy_handles + Index;
            }

            ALWAYS_INLINE void CopyTo(const svc::ipc::MessageBuffer &buffer) const {
                #define _TIPC_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(n) do { if constexpr (NumCopy > n) { buffer.SetHandle(OutIndex + n, copy_handles[n]); } } while (0)
                _TIPC_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(0);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(1);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(2);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(3);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(4);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(5);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(6);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE(7);
                #undef _TIPC_OUT_HANDLE_HOLDER_WRITE_COPY_HANDLE
                #define _TIPC_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(n) do { if constexpr (NumMove > n) { buffer.SetHandle(OutIndex + NumCopy + n, move_handles[n]); } } while (0)
                _TIPC_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(0);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(1);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(2);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(3);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(4);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(5);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(6);
                _TIPC_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE(7);
                #undef _TIPC_OUT_HANDLE_HOLDER_WRITE_MOVE_HANDLE
            }
    };

    template<size_t... Ix>
    struct PrintIndex;

    template<typename CommandMeta>
    class CommandProcessor {
        public:
            /* Useful defines. */
            using ArgsType              = typename CommandMeta::ArgsType;
            using OutRawHolderType      = OutRawHolder<CommandMeta::OutDataSize, CommandMeta::OutDataAlign, CommandMeta::OutMessageRawDataIndex>;
            using OutHandleHolderType   = OutHandleHolder<CommandMeta::NumOutMoveHandles, CommandMeta::NumOutCopyHandles, CommandMeta::OutMessageHandleIndex>;
        private:
            static constexpr u64 GetMessageHeaderForCheck(const svc::ipc::MessageBuffer::MessageHeader &header) {
                using Value = util::BitPack32::Field<0, BITSIZEOF(util::BitPack32)>;

                const util::BitPack32 *data = header.GetData();
                const u32 lower = data[0].Get<Value>();
                const u32 upper = data[1].Get<Value>();

                return static_cast<u64>(lower) | (static_cast<u64>(upper) << BITSIZEOF(u32));
            }

            static constexpr u32 GetSpecialHeaderForCheck(const svc::ipc::MessageBuffer::SpecialHeader &header) {
                using Value = util::BitPack32::Field<0, BITSIZEOF(util::BitPack32)>;

                return header.GetHeader()->Get<Value>();
            }

            /* Argument deserialization. */
            template<size_t Index, typename T = typename std::tuple_element<Index, ArgsType>::type>
            static ALWAYS_INLINE typename std::tuple_element<Index, ArgsType>::type DeserializeArgumentImpl(const svc::ipc::MessageBuffer &message_buffer, const OutRawHolderType &out_raw_holder, OutHandleHolderType &out_handles_holder) {
                constexpr auto Info = CommandMeta::ArgumentSerializationInfos[Index];
                if constexpr (Info.arg_type == ArgumentType::InData) {
                    /* New in rawdata. */
                    constexpr size_t Offset   = CommandMeta::InDataOffsets[Info.in_raw_data_index];
                    constexpr size_t RawIndex = Offset / sizeof(u32);
                    static_assert(Offset == RawIndex * sizeof(u32)); /* TODO: Do unaligned data exist? */

                    if constexpr (!std::same_as<T, bool>) {
                        return message_buffer.GetRaw<T>(CommandMeta::InMessageRawDataIndex + RawIndex);
                    } else {
                        return message_buffer.GetRaw<u8>(CommandMeta::InMessageRawDataIndex + RawIndex) & 1;
                    }
                } else if constexpr (Info.arg_type == ArgumentType::OutData) {
                    /* New out rawdata. */
                    constexpr size_t Offset = CommandMeta::OutDataOffsets[Info.out_raw_data_index];
                    return T(out_raw_holder.template GetAddress<Offset, T::TypeSize>());
                } else if constexpr (Info.arg_type == ArgumentType::InHandle) {
                    /* New InHandle. */
                    if constexpr (std::same_as<T, tipc::MoveHandle>) {
                        constexpr auto HandleIndex = CommandMeta::InMessageHandleIndex + CommandMeta::NumInCopyHandles + Info.in_move_handle_index;
                        return T(message_buffer.GetHandle(HandleIndex));
                    } else if constexpr (std::same_as<T, tipc::CopyHandle>) {
                        constexpr auto HandleIndex = CommandMeta::InMessageHandleIndex + Info.in_copy_handle_index;
                        return T(message_buffer.GetHandle(HandleIndex));
                    } else {
                        static_assert(!std::is_same<T, T>::value, "Invalid InHandle kind");
                    }
                } else if constexpr (Info.arg_type == ArgumentType::OutHandle) {
                    /* New OutHandle. */
                    if constexpr (std::same_as<T, tipc::OutMoveHandle>) {
                        os::NativeHandle * const ptr = out_handles_holder.template GetMoveHandlePointer<Info.out_move_handle_index>();
                        *ptr = os::InvalidNativeHandle;
                        return T(ptr);
                    } else if constexpr (std::same_as<T, tipc::OutCopyHandle>) {
                        os::NativeHandle * const ptr = out_handles_holder.template GetCopyHandlePointer<Info.out_copy_handle_index>();
                        *ptr = os::InvalidNativeHandle;
                        return T(ptr);
                    } else {
                        static_assert(!std::is_same<T, T>::value, "Invalid OutHandle kind");
                    }
                } else if constexpr (Info.arg_type == ArgumentType::Buffer) {
                    /* NOTE: There are currently no tipc commands which use buffers-with-attributes */
                    /*       If these are added (e.g., NonSecure buffers), implement checking here? */
                    constexpr size_t MapAliasDescriptorSize = svc::ipc::MessageBuffer::MapAliasDescriptor::GetDataSize();

                    if constexpr (Info.is_send_buffer) {
                        /* Input send buffer. */
                        constexpr auto BufferIndex = CommandMeta::InMessageBufferIndex + (Info.send_map_alias_index * MapAliasDescriptorSize / sizeof(util::BitPack32));

                        const svc::ipc::MessageBuffer::MapAliasDescriptor descriptor(message_buffer, BufferIndex);
                        return T(descriptor.GetAddress(), descriptor.GetSize());
                    } else {
                        /* Input receive buffer. */
                        constexpr auto BufferIndex = CommandMeta::InMessageBufferIndex + ((CommandMeta::NumInBuffers + Info.recv_map_alias_index) * MapAliasDescriptorSize / sizeof(util::BitPack32));

                        const svc::ipc::MessageBuffer::MapAliasDescriptor descriptor(message_buffer, BufferIndex);
                        return T(descriptor.GetAddress(), descriptor.GetSize());
                    }
                } else if constexpr (Info.arg_type == ArgumentType::ProcessId) {
                    return T{ os::ProcessId{ message_buffer.GetProcessId(CommandMeta::InMessageProcessIdIndex) } };
                } else {
                    static_assert(!std::is_same<T, T>::value, "Invalid ArgumentType<T>");
                }
            }
        public:
            static ALWAYS_INLINE Result ValidateCommandFormat(const svc::ipc::MessageBuffer &message_buffer) {
                /* Validate the message header. */
                constexpr auto ExpectedMessageHeader = GetMessageHeaderForCheck(CommandMeta::InMessageHeader);
                R_UNLESS(message_buffer.Get64(0) == ExpectedMessageHeader, tipc::ResultInvalidMessageFormat());

                /* Validate the special header. */
                if constexpr (CommandMeta::HasInSpecialHeader) {
                    constexpr auto ExpectedSpecialHeader = GetSpecialHeaderForCheck(CommandMeta::InSpecialHeader);
                    constexpr auto SpecialHeaderIndex    = svc::ipc::MessageBuffer::MessageHeader::GetDataSize() / sizeof(util::BitPack32);
                    R_UNLESS(message_buffer.Get32(SpecialHeaderIndex) == ExpectedSpecialHeader, tipc::ResultInvalidMessageFormat());
                }

                return ResultSuccess();
            }

            template<size_t Ix>
            static ALWAYS_INLINE auto DeserializeArgument(const svc::ipc::MessageBuffer &message_buffer, const OutRawHolderType &out_raw_holder, OutHandleHolderType &out_handles_holder) {
                return DeserializeArgumentImpl<Ix>(message_buffer, out_raw_holder, out_handles_holder);
            }

            static ALWAYS_INLINE void SerializeResults(const svc::ipc::MessageBuffer &message_buffer, const Result &result, const OutRawHolderType &out_raw_holder, const OutHandleHolderType &out_handles_holder) {
                /* Set output headers. */
                message_buffer.Set(CommandMeta::OutMessageHeader);
                if constexpr (CommandMeta::HasOutSpecialHeader) {
                    message_buffer.Set(CommandMeta::OutSpecialHeader);
                }

                /* Set output handles. */
                out_handles_holder.CopyTo(message_buffer);

                /* Set output result. */
                message_buffer.Set(CommandMeta::OutMessageResultIndex, result.GetValue());

                /* Set output data. */
                out_raw_holder.CopyTo(message_buffer);
            }
    };

    struct FunctionTraits {
        public:
            template<typename R, typename C, typename... A>
            static std::tuple<A...> GetArgumentsImpl(R(C::*)(A...));

            template<typename R, typename C, typename... A>
            static R GetReturnImpl(R(C::*)(A...));
    };

    template<u16 _CommmandId, auto ServiceCommandImpl, typename ClassType>
    constexpr ALWAYS_INLINE Result InvokeServiceCommandImpl(ClassType *object, const svc::ipc::MessageBuffer &message_buffer) {
        using Return             = decltype(FunctionTraits::GetReturnImpl(ServiceCommandImpl));
        using TrueArgumentsTuple = decltype(FunctionTraits::GetArgumentsImpl(ServiceCommandImpl));

        using CommandMeta        = CommandMetaInfo<_CommmandId, TrueArgumentsTuple>;
        using Processor          = CommandProcessor<CommandMeta>;
        /* TODO: ValidateClassType is valid? */

        constexpr bool ReturnsResult = std::is_same<Return, Result>::value;
        constexpr bool ReturnsVoid   = std::is_same<Return, void>::value;
        static_assert(ReturnsResult || ReturnsVoid, "Service Commands must return Result or void.");

        /* Validate that the command is valid. */
        R_TRY(Processor::ValidateCommandFormat(message_buffer));

        /* Deserialize arguments. */
        typename Processor::OutRawHolderType out_raw_holder;
        typename Processor::OutHandleHolderType out_handles_holder;
        const Result command_result = [&]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
            if constexpr (ReturnsResult) {
                return (object->*ServiceCommandImpl)(Processor::template DeserializeArgument<Ix>(message_buffer, out_raw_holder, out_handles_holder)...);
            } else {
                (object->*ServiceCommandImpl)(Processor::template DeserializeArgument<Ix>(message_buffer, out_raw_holder, out_handles_holder)...);
                return ResultSuccess();
            }
        }(std::make_index_sequence<std::tuple_size<typename CommandMeta::ArgsType>::value>());

        /* Serialize output. */
        Processor::SerializeResults(message_buffer, command_result, out_raw_holder, out_handles_holder);

        return ResultSuccess();
    }

}
