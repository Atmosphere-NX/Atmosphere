/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include <cstdlib>
#include <cstring>
#include <tuple>
#include <type_traits>
#include <memory>

#include "../results.hpp"

#include "ipc_out.hpp"
#include "ipc_buffers.hpp"
#include "ipc_special.hpp"

#include "ipc_domain_object.hpp"

#include "ipc_response_context.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

template <typename...>
struct TypeList{};

template <typename... T1s, typename... T2s>
constexpr auto Concatenate(TypeList<T1s...>, TypeList<T2s...>) {
    return TypeList<T1s..., T2s...>{};
}

template <template <typename> typename Condition, typename R>
constexpr auto FilterTypes(R result, TypeList<>) {
    return result;
}

template <template <typename> typename Condition, typename R, typename T, typename... Ts>
constexpr auto FilterTypes(R result, TypeList<T, Ts...>) {
    if constexpr (Condition<T>{})
        return FilterTypes<Condition>(Concatenate(result, TypeList<T>{}), TypeList<Ts...>{});
    else
        return FilterTypes<Condition>(result, TypeList<Ts...>{});
}

template<typename Types> struct TypeListToTuple;

template<typename... Types>
struct TypeListToTuple<TypeList<Types...>> {
    using type = std::tuple<Types...>;
};

template <template <typename> typename Condition, typename... Types>
using FilteredTypes = typename TypeListToTuple<std::decay_t<decltype(FilterTypes<Condition>(TypeList<>{}, TypeList<Types...>{}))>>::type;

enum class ArgType {
    InData,
    OutData,
    InHandle,
    OutHandle,
    InSession,
    OutSession,
    PidDesc,
    InBuffer,
    OutBuffer,
    InPointer,
    OutPointerClientSize,
    OutPointerServerSize,
};

template<typename X>
constexpr ArgType GetArgType() {
    if constexpr (std::is_base_of_v<OutDataTag, X>) {
        return ArgType::OutData;
    } else if constexpr (std::is_base_of_v<OutSessionTag, X>) {
        return ArgType::OutSession;
    } else if constexpr (std::is_base_of_v<OutHandleTag, X>) {
        return ArgType::OutHandle;
    } else if constexpr (std::is_base_of_v<InBufferBase, X>) {
        return ArgType::InBuffer;
    } else if constexpr (std::is_base_of_v<OutBufferBase, X>) {
        return ArgType::OutBuffer;
    } else if constexpr (std::is_base_of_v<InPointerBase, X>) {
        return ArgType::InPointer;
    } else if constexpr (std::is_base_of_v<OutPointerWithClientSizeBase, X>) {
        return ArgType::OutPointerClientSize;
    } else if constexpr (std::is_base_of_v<OutPointerWithServerSizeBase, X>) {
        return ArgType::OutPointerServerSize;
    } else if constexpr (std::is_base_of_v<PidDescriptorTag, X>) {
        return ArgType::PidDesc;
    } else if constexpr (std::is_base_of_v<IpcHandleTag, X>) {
        return ArgType::InHandle;
    } else if constexpr (std::is_trivial_v<X> && !std::is_pointer_v<X>) {
        return ArgType::InData;
    } else {
        static_assert(std::is_pod_v<X> && !std::is_pod_v<X>, "Unhandled InSession!");
        return ArgType::InSession;
    }
}

template<ArgType ArgT>
struct ArgTypeFilter {
    template<typename X>
    using type = std::conditional_t<GetArgType<X>() == ArgT, std::true_type, std::false_type>;
};

template<ArgType ArgT>
struct IsArgTypeBuffer {
    static constexpr bool value = ArgT == ArgType::InBuffer || ArgT == ArgType::OutBuffer || ArgT == ArgType::InPointer || ArgT == ArgType::OutPointerClientSize || ArgT == ArgType::OutPointerServerSize;
};

struct ArgTypeBufferFilter {
    template<typename X>
    using type = std::conditional_t<IsArgTypeBuffer<GetArgType<X>()>::value, std::true_type, std::false_type>;
};

template<ArgType ArgT>
struct IsArgTypeInData {
    static constexpr bool value = ArgT == ArgType::InData || ArgT == ArgType::PidDesc;
};

struct ArgTypeInDataFilter {
    template<typename X>
    using type = std::conditional_t<IsArgTypeInData<GetArgType<X>()>::value, std::true_type, std::false_type>;
};

template<typename T>
struct RawDataHelper {
    static_assert(GetArgType<T>() == ArgType::InData || GetArgType<T>() == ArgType::PidDesc);
    static constexpr size_t align = (GetArgType<T>() == ArgType::InData) ? __alignof__(T) : __alignof__(u64);
    static constexpr size_t size = (GetArgType<T>() == ArgType::InData) ? sizeof(T) : sizeof(u64);
};

template<typename T>
struct RawDataHelper<Out<T>> {
    static_assert(GetArgType<T>() == ArgType::InData);
    static constexpr size_t align = __alignof(T);
    static constexpr size_t size = sizeof(T);
};

template<typename Ts>
struct RawDataComputer;

template<typename... Ts>
struct RawDataComputer<std::tuple<Ts...>> {
    /* https://referencesource.microsoft.com/#System.Core/System/Linq/Enumerable.cs,2604 */
    static constexpr void QuickSort(std::array<size_t, sizeof...(Ts)> &map, std::array<size_t, sizeof...(Ts)> &values, int left, int right) {
        do {
            int i = left;
            int j = right;
            int x = map[i + ((j - i) >> 1)];
            do {
                while (i < static_cast<int>(sizeof...(Ts)) && values[x] > values[map[i]]) i++;
                while (j >= 0 && values[x] < values[map[j]]) j--;
                if (i > j) break;
                if (i < j) {
                    const size_t temp = map[i];
                    map[i] = map[j];
                    map[j] = temp;
                }
                i++;
                j--;
            } while (i <= j);
            if (j - left <= right - i) {
                if (left < j) QuickSort(map, values, left, j);
                left = i;
            } else {
                if (i < right) QuickSort(map, values, i, right);
                right = j;
            }
        } while (left < right);
    }

    static constexpr void StableSort(std::array<size_t, sizeof...(Ts)> &map, std::array<size_t, sizeof...(Ts)> &values) {
        /* First, quicksort a copy of the map. */
        std::array<size_t, sizeof...(Ts)> map_unstable(map);
        QuickSort(map_unstable, values, 0, sizeof...(Ts)-1);

        /* Now, create stable sorted map from unstably quicksorted indices (via repeated insertion sort on element runs). */
        for (size_t i = 0; i < sizeof...(Ts); i++) {
            map[i] = map_unstable[i];
            for (ssize_t j = i-1; j >= 0 && values[map[j]] == values[map[j+1]] && map[j] > map[j+1]; j--) {
                const size_t temp = map[j];
                map[j] = map[j+1];
                map[j+1] = temp;
            }
        }
    }

    static constexpr std::array<size_t, sizeof...(Ts)+1> GetOffsets() {
        std::array<size_t, sizeof...(Ts)+1> offsets = {};
        offsets[0] = 0;
        if constexpr (sizeof...(Ts) > 0) {
            /* Get size, alignment for each type. */
            std::array<size_t, sizeof...(Ts)> sizes = { RawDataHelper<Ts>::size... };
            std::array<size_t, sizeof...(Ts)> aligns = { RawDataHelper<Ts>::align... };

            /* We want to sort...by alignment. */
            std::array<size_t, sizeof...(Ts)> map = {};
            for (size_t i = 0; i < sizeof...(Ts); i++) { map[i] = i; }
            StableSort(map, aligns);

            /* Iterate over sorted types. */
            size_t cur_offset = 0;
            for (size_t i = 0; i < sizeof...(Ts); i++) {
                const size_t align = aligns[map[i]];
                if (cur_offset % align != 0) {
                    cur_offset += align - (cur_offset % align);
                }
                offsets[map[i]] = cur_offset;
                cur_offset += sizes[map[i]];
            }
            offsets[sizeof...(Ts)] = cur_offset;
        }
        return offsets;
    }

    static constexpr std::array<size_t, sizeof...(Ts)+1> offsets = GetOffsets();
};

template <typename _Args, typename _ReturnType>
struct CommandMetaInfo;

template<typename... _Args, typename _ReturnType>
struct CommandMetaInfo<std::tuple<_Args...>, _ReturnType> {
    using Args = std::tuple<_Args...>;
    using ReturnType = _ReturnType;

    static constexpr bool ReturnsResult = std::is_same_v<ReturnType, Result>;
    static constexpr bool ReturnsVoid = std::is_same_v<ReturnType, void>;

    using InDatas = FilteredTypes<ArgTypeInDataFilter::type, _Args...>;
    using OutDatas = FilteredTypes<ArgTypeFilter<ArgType::OutData>::type, _Args...>;
    using InHandles = FilteredTypes<ArgTypeFilter<ArgType::InHandle>::type, _Args...>;
    using OutHandles = FilteredTypes<ArgTypeFilter<ArgType::OutHandle>::type, _Args...>;
    using InSessions = FilteredTypes<ArgTypeFilter<ArgType::InSession>::type, _Args...>;
    using OutSessions = FilteredTypes<ArgTypeFilter<ArgType::OutSession>::type, _Args...>;
    using PidDescs = FilteredTypes<ArgTypeFilter<ArgType::PidDesc>::type, _Args...>;

    using InBuffers = FilteredTypes<ArgTypeFilter<ArgType::InBuffer>::type, _Args...>;
    using OutBuffers = FilteredTypes<ArgTypeFilter<ArgType::OutBuffer>::type, _Args...>;
    using InPointers = FilteredTypes<ArgTypeFilter<ArgType::InPointer>::type, _Args...>;
    using ClientSizeOutPointers = FilteredTypes<ArgTypeFilter<ArgType::OutPointerClientSize>::type, _Args...>;
    using ServerSizeOutPointers = FilteredTypes<ArgTypeFilter<ArgType::OutPointerServerSize>::type, _Args...>;
    using Buffers = FilteredTypes<ArgTypeBufferFilter::type, _Args...>;

    static constexpr size_t NumInDatas = std::tuple_size_v<InDatas>;
    static constexpr size_t NumOutDatas = std::tuple_size_v<OutDatas>;
    static constexpr size_t NumInHandles = std::tuple_size_v<InHandles>;
    static constexpr size_t NumOutHandles = std::tuple_size_v<OutHandles>;
    static constexpr size_t NumInSessions = std::tuple_size_v<InSessions>;
    static constexpr size_t NumOutSessions = std::tuple_size_v<OutSessions>;
    static constexpr size_t NumPidDescs = std::tuple_size_v<PidDescs>;

    static constexpr size_t NumInBuffers = std::tuple_size_v<InBuffers>;
    static constexpr size_t NumOutBuffers = std::tuple_size_v<OutBuffers>;
    static constexpr size_t NumInPointers = std::tuple_size_v<InPointers>;
    static constexpr size_t NumClientSizeOutPointers = std::tuple_size_v<ClientSizeOutPointers>;
    static constexpr size_t NumServerSizeOutPointers = std::tuple_size_v<ServerSizeOutPointers>;
    static constexpr size_t NumBuffers = std::tuple_size_v<Buffers>;

    static_assert(NumInSessions == 0, "InSessions not yet supported!");
    static_assert(NumPidDescs == 0 || NumPidDescs == 1, "Methods can only take in 0 or 1 PIDs!");
    static_assert(NumBuffers <= 8, "Methods can only take in <= 8 Buffers!");
    static_assert(NumInHandles <= 8, "Methods can take in <= 8 Handles!");
    static_assert(NumOutHandles + NumOutSessions <= 8, "Methods can only return <= 8 Handles+Sessions!");

    static constexpr std::array<size_t, NumInDatas+1> InDataOffsets = RawDataComputer<InDatas>::offsets;
    static constexpr size_t InRawArgSize = InDataOffsets[NumInDatas];
    static constexpr size_t InRawArgSizeWithOutPointers = ((InRawArgSize + NumClientSizeOutPointers * sizeof(u16)) + 3) & ~3;

    static constexpr std::array<size_t, NumOutDatas+1> OutDataOffsets = RawDataComputer<OutDatas>::offsets;
    static constexpr size_t OutRawArgSize = OutDataOffsets[NumOutDatas];
};


/* ================================================================================= */
/* Actual wrapping implementation goes here. */

/* Validator. */
struct Validator {

    template <typename T>
    static constexpr bool ValidateCommandArgument(IpcResponseContext *ctx, size_t& a_index, size_t& b_index, size_t& x_index, size_t& h_index, size_t& cur_c_size_offset, size_t& total_c_size) {
        constexpr ArgType argT = GetArgType<T>();
        if constexpr (argT == ArgType::InBuffer) {
            return (ctx->request.Buffers[a_index] != nullptr || ctx->request.BufferSizes[a_index] == 0) && ctx->request.BufferDirections[a_index] == BufferDirection_Send && ctx->request.BufferTypes[a_index++] == T::expected_type;
        } else if constexpr (argT == ArgType::OutBuffer) {
            return (ctx->request.Buffers[b_index] != nullptr || ctx->request.BufferSizes[b_index] == 0) && ctx->request.BufferDirections[b_index] == BufferDirection_Recv && ctx->request.BufferTypes[b_index++] == T::expected_type;
        } else if constexpr (argT == ArgType::InPointer) {
            return ctx->request.Statics[x_index++] != nullptr;
        } else if constexpr (argT == ArgType::InHandle) {
            if constexpr (std::is_same_v<T, MovedHandle>) {
                return !ctx->request.WasHandleCopied[h_index++];
            } else if constexpr (std::is_same_v<T, CopiedHandle>) {
                return ctx->request.WasHandleCopied[h_index++];
            }
        } else {
            if constexpr (argT == ArgType::OutPointerServerSize) {
                total_c_size += T::num_elements * sizeof(T);
            } else if constexpr (argT == ArgType::OutPointerClientSize) {
                total_c_size += *((u16 *)((uintptr_t)(ctx->request.Raw) + 0x10 + cur_c_size_offset));
                cur_c_size_offset += sizeof(u16);
            }
            return true;
        }
    }

    template <typename Ts>
    struct ValidateCommandTuple;

    template <typename ...Ts>
    struct ValidateCommandTuple<std::tuple<Ts...>> {
        static constexpr bool IsValid(IpcResponseContext *ctx, size_t& a_index, size_t& b_index, size_t& x_index, size_t& h_index, size_t& cur_c_size_offset, size_t& total_c_size) {
            return (ValidateCommandArgument<Ts>(ctx, a_index, b_index, x_index, h_index, cur_c_size_offset, total_c_size) && ...);
        }
    };

    template<typename MetaInfo>
	static constexpr Result Validate(IpcResponseContext *ctx) {
        if (ctx->request.RawSize < MetaInfo::InRawArgSizeWithOutPointers) {
            return ResultKernelConnectionClosed;
        }

        if (ctx->request.NumBuffers != MetaInfo::NumInBuffers + MetaInfo::NumOutBuffers) {
            return ResultKernelConnectionClosed;
        }

        if (ctx->request.NumStatics != MetaInfo::NumInPointers) {
            return ResultKernelConnectionClosed;
        }

        if (ctx->request.NumStaticsOut != MetaInfo::NumClientSizeOutPointers + MetaInfo::NumServerSizeOutPointers) {
            return ResultKernelConnectionClosed;
        }

        if (ctx->request.NumHandles != MetaInfo::NumInHandles) {
            return ResultKernelConnectionClosed;
        }


        if ((ctx->request.HasPid && MetaInfo::NumPidDescs == 0) || (!ctx->request.HasPid && MetaInfo::NumPidDescs != 0)) {
            return ResultKernelConnectionClosed;
        }

        if (((u32 *)ctx->request.Raw)[0] != SFCI_MAGIC) {
            return ResultKernelConnectionClosed;
        }

        size_t a_index = 0, b_index = MetaInfo::NumInBuffers, x_index = 0, h_index = 0;
        size_t cur_c_size_offset = MetaInfo::InRawArgSize + (0x10 - ((uintptr_t)ctx->request.Raw - (ctx->request.IsDomainRequest ? sizeof(DomainMessageHeader) : 0) - (uintptr_t)ctx->request.RawWithoutPadding));
        size_t total_c_size = 0;

        if (!ValidateCommandTuple<typename MetaInfo::Args>::IsValid(ctx, a_index, b_index, x_index, h_index, cur_c_size_offset, total_c_size)) {
            return ResultKernelConnectionClosed;
        }

        if (total_c_size > ctx->pb_size) {
            return ResultKernelConnectionClosed;
        }

        return ResultSuccess;
	}
};

/* ================================================================================= */

/* Decoder. */
template<typename MetaInfo>
struct Decoder {

    template<typename T>
    static constexpr T DecodeCommandArgument(IpcResponseContext *ctx, size_t& a_index, size_t& b_index, size_t& x_index, size_t& c_index, size_t& in_h_index, size_t& out_h_index, size_t& out_obj_index, size_t& in_data_index, size_t& out_data_index, size_t& pb_offset, size_t& c_sz_offset) {
        constexpr ArgType argT = GetArgType<T>();
        if constexpr (argT == ArgType::InBuffer) {
            const T& value = T(ctx->request.Buffers[a_index], ctx->request.BufferSizes[a_index], ctx->request.BufferTypes[a_index]);
            ++a_index;
            return value;
        } else if constexpr (argT == ArgType::OutBuffer) {
            const T& value = T(ctx->request.Buffers[b_index], ctx->request.BufferSizes[b_index], ctx->request.BufferTypes[b_index]);
            ++b_index;
            return value;
        } else if constexpr (argT == ArgType::InPointer) {
            const T& value = T(ctx->request.Statics[x_index], ctx->request.StaticSizes[x_index]);
            ++x_index;
            return value;
        } else if constexpr (argT == ArgType::InHandle) {
            return T(ctx->request.Handles[in_h_index++]);
        } else if constexpr (argT == ArgType::OutHandle) {
            return T(&ctx->out_handles[out_h_index++]);
        } else if constexpr (argT == ArgType::PidDesc) {
            uintptr_t ptr = ((uintptr_t)ctx->request.Raw + 0x10 + MetaInfo::InDataOffsets[in_data_index++]);
            *(u64 *)ptr = ctx->request.Pid;
            return T(ctx->request.Pid);
        } else if constexpr (argT == ArgType::InData) {
            uintptr_t ptr = ((uintptr_t)ctx->request.Raw + 0x10 + MetaInfo::InDataOffsets[in_data_index++]);
            if constexpr (std::is_same_v<bool, T>) {
                return *((u8 *)ptr) & 1;
            } else {
                return *((T *)ptr);
            }
        } else if constexpr (argT == ArgType::OutData) {
            uintptr_t ptr = ((uintptr_t)ctx->out_data + MetaInfo::OutDataOffsets[out_data_index++]);
            return T(reinterpret_cast<typename OutHelper<T>::type *>(ptr));
        } else if constexpr (argT == ArgType::OutPointerClientSize || argT == ArgType::OutPointerServerSize) {
            u16 sz;
            if constexpr(argT == ArgType::OutPointerServerSize) {
                sz = T::element_size;
            } else {
                sz = *(const u16 *)((uintptr_t)ctx->request.Raw + 0x10 + c_sz_offset);
            }
            u8* buf = ctx->pb + pb_offset;
            c_sz_offset += sizeof(u16);
            pb_offset += sz;
            ipcAddSendStatic(&ctx->reply, buf, sz, c_index++);
            return T(buf, sz);
        } else if constexpr (argT == ArgType::OutSession) {
            if (IsDomainObject(ctx->obj_holder)) {
               const T& value = T(ctx->out_objs[out_obj_index], ctx->obj_holder->GetServiceObject<IDomainObject>(), &ctx->out_object_ids[out_obj_index]);
               out_obj_index++;
               return value;
            } else {
               const T& value = T(ctx->out_objs[out_obj_index], nullptr, 0);
               out_obj_index++;
               return value;
            }
        }
    }

    template <typename Ts>
    struct DecodeTuple;

    template <typename ...Ts>
    struct DecodeTuple<std::tuple<Ts...>> {
        static constexpr std::tuple<Ts...> GetArgs(IpcResponseContext *ctx, size_t& a_index, size_t& b_index, size_t& x_index, size_t& c_index, size_t& in_h_index, size_t& out_h_index, size_t& out_obj_index, size_t& in_data_index, size_t& out_data_index, size_t& pb_offset, size_t& c_sz_offset) {
            return std::tuple<Ts... > {
                    DecodeCommandArgument<Ts>(ctx, a_index, b_index, x_index, c_index, in_h_index, out_h_index, out_obj_index, in_data_index, out_data_index, pb_offset, c_sz_offset)
                    ...
            };
        }
    };


	static constexpr typename MetaInfo::Args Decode(IpcResponseContext *ctx) {
        size_t a_index = 0, b_index = MetaInfo::NumInBuffers, x_index = 0, c_index = 0, in_h_index = 0, out_h_index = 0, out_obj_index = 0;
        size_t in_data_index = 0x0, out_data_index = 0, pb_offset = 0;
        size_t c_sz_offset = MetaInfo::InRawArgSize + (0x10 - ((uintptr_t)ctx->request.Raw  - (ctx->request.IsDomainRequest ? sizeof(DomainMessageHeader) : 0) - (uintptr_t)ctx->request.RawWithoutPadding));
        return DecodeTuple<typename MetaInfo::Args>::GetArgs(ctx, a_index, b_index, x_index, c_index,  in_h_index, out_h_index, out_obj_index, in_data_index, out_data_index, pb_offset, c_sz_offset);
    }
};

/* ================================================================================= */

template<typename MetaInfo, typename T>
static constexpr void EncodeArgument(IpcResponseContext *ctx, size_t&out_obj_index, T& arg) {
    constexpr ArgType argT = GetArgType<T>();
    if constexpr (argT == ArgType::OutHandle) {
        if constexpr (std::is_same_v<MovedHandle, typename OutHelper<T>::type>) {
            ipcSendHandleMove(&ctx->reply, arg.GetValue().handle);
        } else {
            ipcSendHandleCopy(&ctx->reply, arg.GetValue().handle);
        }
    } else if constexpr (argT == ArgType::OutSession) {
        if (IsDomainObject(ctx->obj_holder)) {
            auto domain = ctx->obj_holder->GetServiceObject<IDomainObject>();
            domain->SetObject(arg.GetObjectId(), std::move(arg.GetHolder()));
        } else {
            ctx->manager->AddSession(ctx->out_object_server_handles[out_obj_index++], std::move(arg.GetHolder()));
        }
    }
}

template<typename MetaInfo, typename ArgsTuple>
struct Encoder;

template <typename MetaInfo, typename ...Args>
struct Encoder<MetaInfo, std::tuple<Args...>> {

    static constexpr void EncodeFailure(IpcResponseContext *ctx, Result rc) {
        memset(armGetTls(), 0, 0x100);
        ipcInitialize(&ctx->reply);
        struct {
            u64 magic;
            u64 result;
        } *raw;

        if (IsDomainObject(ctx->obj_holder)) {
            raw = (decltype(raw))ipcPrepareHeaderForDomain(&ctx->reply, sizeof(*raw), 0);
            auto resp_header = (DomainResponseHeader *)((uintptr_t)raw - sizeof(DomainResponseHeader));
            *resp_header = {};
        } else {
            raw = (decltype(raw))ipcPrepareHeader(&ctx->reply, sizeof(*raw));
        }
        raw->magic = SFCO_MAGIC;
        raw->result = rc;
    }



	static constexpr void EncodeSuccess(IpcResponseContext *ctx, Args... args) {
        size_t out_obj_index = 0;

        ((EncodeArgument<MetaInfo, Args>(ctx, out_obj_index, args)), ...);

        const bool is_domain = IsDomainObject(ctx->obj_holder);

        if (!is_domain) {
            for (unsigned int i = 0; i < MetaInfo::NumOutSessions; i++) {
                ipcSendHandleMove(&ctx->reply, ctx->out_handles[MetaInfo::NumOutHandles + i].handle);
            }
        }

        struct {
            u64 magic;
            u64 result;
        } *raw;
        if (is_domain) {
            raw = (decltype(raw))ipcPrepareHeaderForDomain(&ctx->reply, sizeof(*raw) + MetaInfo::OutRawArgSize + sizeof(*ctx->out_object_ids) * MetaInfo::NumOutSessions, 0);
            auto resp_header = (DomainResponseHeader *)((uintptr_t)raw - sizeof(DomainResponseHeader));
            *resp_header = {};
            resp_header->NumObjectIds = MetaInfo::NumOutSessions;
        } else {
            raw = (decltype(raw))ipcPrepareHeader(&ctx->reply, sizeof(*raw)+ MetaInfo::OutRawArgSize);
        }

        raw->magic = SFCO_MAGIC;
        raw->result = 0;

        memcpy((void *)((uintptr_t)raw + sizeof(*raw)), ctx->out_data, MetaInfo::OutRawArgSize);
        if (is_domain) {
            memcpy((void *)((uintptr_t)raw + sizeof(*raw) + MetaInfo::OutRawArgSize), ctx->out_object_ids, sizeof(*ctx->out_object_ids) * MetaInfo::NumOutSessions);
        }
    }

};

/* ================================================================================= */

template<auto MemberFunction>
struct MemberFunctionTraits {
    private:
        template<typename R, typename C, typename... A>
        static R GetReturnTypeImpl(R(C::*)(A...));

        template<typename R, typename C, typename... A>
        static C GetClassTypeImpl(R(C::*)(A...));

        template<typename R, typename C, typename... A>
        static std::tuple<A...> GetArgsImpl(R(C::*)(A...));
    public:
        using ReturnType = decltype(GetReturnTypeImpl(MemberFunction));
        using ClassType  = decltype(GetClassTypeImpl(MemberFunction));
        using ArgsType   = decltype(GetArgsImpl(MemberFunction));
};

template<auto IpcCommandImpl, typename ClassType = typename MemberFunctionTraits<IpcCommandImpl>::ClassType>
constexpr Result WrapIpcCommandImpl(IpcResponseContext *ctx) {
    using Traits        = MemberFunctionTraits<IpcCommandImpl>;
    using ArgsType      = typename Traits::ArgsType;
    using ReturnType    = typename Traits::ReturnType;
    using BaseClassType = typename Traits::ClassType;
    static_assert(std::is_base_of_v<BaseClassType, ClassType>, "Override class type incorrect");
    using CommandMetaData = CommandMetaInfo<ArgsType, ReturnType>;

    static_assert(CommandMetaData::ReturnsResult || CommandMetaData::ReturnsVoid, "IpcCommandImpls must return Result or void");

    ipcInitialize(&ctx->reply);
    memset(ctx->out_data, 0, CommandMetaData::OutRawArgSize);

    R_TRY(Validator::Validate<CommandMetaData>(ctx));

    ClassType *this_ptr = nullptr;
    if (IsDomainObject(ctx->obj_holder)) {
        this_ptr = ctx->obj_holder->GetServiceObject<IDomainObject>()->GetObject(ctx->request.InThisObjectId)->GetServiceObject<ClassType>();
    } else {
        this_ptr = ctx->obj_holder->GetServiceObject<ClassType>();
    }
    if (this_ptr == nullptr) {
        return ResultServiceFrameworkTargetNotFound;
    }

    size_t num_out_objects;
    std::shared_ptr<IServiceObject> out_objects[CommandMetaData::NumOutSessions];

    auto cleanup_guard = SCOPE_GUARD {
        /* Clean up objects as necessary. */
        if (IsDomainObject(ctx->obj_holder)) {
            for (unsigned int i = 0; i < num_out_objects; i++) {
                ctx->obj_holder->GetServiceObject<IDomainObject>()->FreeObject(ctx->out_object_ids[i]);
            }
        } else {
            for (unsigned int i = 0; i < num_out_objects; i++) {
                svcCloseHandle(ctx->out_object_server_handles[i]);
                svcCloseHandle(ctx->out_handles[CommandMetaData::NumOutHandles + i].handle);
            }
        }

        for (unsigned int i = 0; i < num_out_objects; i++) {
            ctx->out_objs[i] = nullptr;
        }
    };

    /* Allocate out object IDs. */
    if (IsDomainObject(ctx->obj_holder)) {
        for (num_out_objects = 0; num_out_objects < CommandMetaData::NumOutSessions; num_out_objects++) {
            R_TRY_CLEANUP(ctx->obj_holder->GetServiceObject<IDomainObject>()->ReserveObject(&ctx->out_object_ids[num_out_objects]), {
                std::apply(Encoder<CommandMetaData, typename CommandMetaData::Args>::EncodeFailure, std::tuple_cat(std::make_tuple(ctx), std::make_tuple(R_CLEANUP_RESULT)));
            });
            ctx->out_objs[num_out_objects] = &out_objects[num_out_objects];
        }
    } else {
        for (num_out_objects = 0; num_out_objects < CommandMetaData::NumOutSessions; num_out_objects++) {
            Handle server_h, client_h;
            R_TRY_CLEANUP(SessionManagerBase::CreateSessionHandles(&server_h, &client_h), {
                std::apply(Encoder<CommandMetaData, typename CommandMetaData::Args>::EncodeFailure, std::tuple_cat(std::make_tuple(ctx), std::make_tuple(R_CLEANUP_RESULT)));
            });
            ctx->out_object_server_handles[num_out_objects] = server_h;
            ctx->out_handles[CommandMetaData::NumOutHandles + num_out_objects].handle = client_h;
            ctx->out_objs[num_out_objects] = &out_objects[num_out_objects];
        }
    }

    /* Decode, apply, encode. */
    {
        auto args = Decoder<CommandMetaData>::Decode(ctx);

        if constexpr (CommandMetaData::ReturnsResult) {
            R_TRY_CLEANUP(std::apply( [=](auto&&... args) { return (this_ptr->*IpcCommandImpl)(args...); }, args), {
                std::apply(Encoder<CommandMetaData, decltype(args)>::EncodeFailure, std::tuple_cat(std::make_tuple(ctx), std::make_tuple(R_CLEANUP_RESULT)));
            });
        } else {
            std::apply( [=](auto&&... args) { (this_ptr->*IpcCommandImpl)(args...); }, args);
        }

        std::apply(Encoder<CommandMetaData, decltype(args)>::EncodeSuccess, std::tuple_cat(std::make_tuple(ctx), args));
    }

    /* Cancel object guard, clear remaining object references. */
    cleanup_guard.Cancel();
    for (unsigned int i = 0; i < num_out_objects; i++) {
        ctx->out_objs[i] = nullptr;
    }

    return ResultSuccess;
}

template <auto CommandId, auto CommandImpl, typename OverrideClassType, FirmwareVersion Low = FirmwareVersion_Min, FirmwareVersion High = FirmwareVersion_Max>
inline static constexpr ServiceCommandMeta MakeServiceCommandMeta() {
    return {
        .fw_low = Low,
        .fw_high = High,
        .cmd_id = static_cast<u32>(CommandId),
        .handler = WrapIpcCommandImpl<CommandImpl, OverrideClassType>,
    };
};

#define MAKE_SERVICE_COMMAND_META(class, name, ...) MakeServiceCommandMeta<CommandId::name, &class::name, class, ##__VA_ARGS__>()

#pragma GCC diagnostic pop
