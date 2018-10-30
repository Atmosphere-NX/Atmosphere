/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include "../../boost/callable_traits.hpp"
#include <type_traits>
#include <memory>

#include "ipc_out.hpp"
#include "ipc_buffers.hpp"
#include "ipc_special.hpp"

#include "ipc_domain_object.hpp"

#include "ipc_response_context.hpp"

#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

template<typename Tuple>
struct PopFront;

template<typename Head, typename... Tail>
struct PopFront<std::tuple<Head, Tail...>> {
    using type = std::tuple<Tail...>;
};

template <typename ...> struct WhichType;

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

template<typename T>
struct RawSizeElementAdder {
    static constexpr size_t GetUpdateElementSize(size_t &size) {
        constexpr size_t t_align = RawDataHelper<T>::align;
        constexpr size_t t_size = RawDataHelper<T>::size;
        if (size % t_align == 0) {
            size += t_align - (size % t_align);
        }
        size += t_size;
        return size;
    }
};

template<typename Ts>
struct GetRawDataSize;

template<typename... Ts>
struct GetRawDataSize<std::tuple<Ts...>> {
    static constexpr size_t Size() {
        if constexpr (sizeof...(Ts) == 0) {
            return 0;
        } else {
            size_t s = 0;
            size_t ends[] = { RawSizeElementAdder<Ts>::GetUpdateElementSize(s)... };
            return (ends[sizeof...(Ts)-1] + 3) & ~3;
        }
    }
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
    
    static constexpr size_t InRawArgSize = GetRawDataSize<InDatas>::Size();
    static constexpr size_t InRawArgSizeWithOutPointers = ((InRawArgSize + NumClientSizeOutPointers * sizeof(u16)) + 3) & ~3;
    
    static constexpr size_t OutRawArgSize = GetRawDataSize<OutDatas>::Size();
};


/* ================================================================================= */
/* Actual wrapping implementation goes here. */

/* Validator. */
struct Validator {
    
    template <typename T>
    static constexpr bool ValidateCommandArgument(IpcResponseContext *ctx, size_t& a_index, size_t& b_index, size_t& x_index, size_t& h_index, size_t& cur_c_size_offset, size_t& total_c_size) {
        constexpr ArgType argT = GetArgType<T>();
        if constexpr (argT == ArgType::InBuffer) {
            return ctx->request.Buffers[a_index] != nullptr && ctx->request.BufferDirections[a_index] == BufferDirection_Send && ctx->request.BufferTypes[a_index++] == T::expected_type;
        } else if constexpr (argT == ArgType::OutBuffer) {
            return ctx->request.Buffers[b_index] != nullptr && ctx->request.BufferDirections[b_index] == BufferDirection_Recv && ctx->request.BufferTypes[b_index++] == T::expected_type;
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
            } else if constexpr (argT == ArgType::OutPointerServerSize) {
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
            return 0xF601;
        }
                        
        if (ctx->request.NumBuffers != MetaInfo::NumInBuffers + MetaInfo::NumOutBuffers) {
            return 0xF601;
        }
        
        if (ctx->request.NumStatics != MetaInfo::NumInPointers) {
            return 0xF601;
        }

        if (ctx->request.NumStaticsOut != MetaInfo::NumClientSizeOutPointers + MetaInfo::NumServerSizeOutPointers) {
            return 0xF601;
        }
        
        if (ctx->request.NumHandles != MetaInfo::NumInHandles) {
            return 0xF601;
        }
        
                
        if ((ctx->request.HasPid && MetaInfo::NumPidDescs == 0) || (!ctx->request.HasPid && MetaInfo::NumPidDescs != 0)) {
            return 0xF601;
        }
        
        if (((u32 *)ctx->request.Raw)[0] != SFCI_MAGIC) {
            return 0xF601;
        }
        
        size_t a_index = 0, b_index = MetaInfo::NumInBuffers, x_index = 0, h_index = 0;
        size_t cur_c_size_offset = MetaInfo::InRawArgSize + (0x10 - ((uintptr_t)ctx->request.Raw - (uintptr_t)ctx->request.RawWithoutPadding));
        size_t total_c_size = 0;
        
        if (!ValidateCommandTuple<typename MetaInfo::Args>::IsValid(ctx, a_index, b_index, x_index, h_index, cur_c_size_offset, total_c_size)) {
            return 0xF601;
        }
        
        if (total_c_size > ctx->pb_size) {
            return 0xF601;
        }

        return 0;
	}
};

/* ================================================================================= */

/* Decoder. */
struct Decoder {
    
    template<typename T>
    static constexpr T DecodeCommandArgument(IpcResponseContext *ctx, size_t& a_index, size_t& b_index, size_t& x_index, size_t& c_index, size_t& in_h_index, size_t& out_h_index, size_t& out_obj_index, size_t& in_rd_offset, size_t& out_rd_offset, size_t& pb_offset, size_t& c_sz_offset) {
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
            constexpr size_t t_align = RawDataHelper<T>::align;
            constexpr size_t t_size = RawDataHelper<T>::size;
            if (in_rd_offset % t_align) {
                in_rd_offset += (t_align - (in_rd_offset % t_align));
            }
            uintptr_t ptr = ((uintptr_t)ctx->request.Raw + 0x10 + in_rd_offset);
            in_rd_offset += t_size;
            *(u64 *)ptr = ctx->request.Pid;
            return T(ctx->request.Pid);
        } else if constexpr (argT == ArgType::InData) {
            constexpr size_t t_align = RawDataHelper<T>::align;
            constexpr size_t t_size = RawDataHelper<T>::size;
            if (in_rd_offset % t_align) {
                in_rd_offset += (t_align - (in_rd_offset % t_align));
            }
            uintptr_t ptr = ((uintptr_t)ctx->request.Raw + 0x10 + in_rd_offset);
            in_rd_offset += t_size;
            if constexpr (std::is_same_v<bool, T>) {
                return *((u8 *)ptr) & 1;
            } else {
                return *((T *)ptr);
            }
        } else if constexpr (argT == ArgType::OutData) {
            constexpr size_t t_align = RawDataHelper<T>::align;
            constexpr size_t t_size = RawDataHelper<T>::size;
            if (out_rd_offset % t_align) {
                out_rd_offset += (t_align - (out_rd_offset % t_align));
            }
            uintptr_t ptr = ((uintptr_t)ctx->out_data + out_rd_offset);
            out_rd_offset += t_size;
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
        static constexpr std::tuple<Ts...> GetArgs(IpcResponseContext *ctx, size_t& a_index, size_t& b_index, size_t& x_index, size_t& c_index, size_t& in_h_index, size_t& out_h_index, size_t& out_obj_index, size_t& in_rd_offset, size_t& out_rd_offset, size_t& pb_offset, size_t& c_sz_offset) {
            return std::tuple<Ts... > {
                    DecodeCommandArgument<Ts>(ctx, a_index, b_index, x_index, c_index, in_h_index, out_h_index, out_obj_index, in_rd_offset, out_rd_offset, pb_offset, c_sz_offset)
                    ...
            };
        }
    };

        
    template<typename MetaInfo>
	static constexpr typename MetaInfo::Args Decode(IpcResponseContext *ctx) {
        size_t a_index = 0, b_index = MetaInfo::NumInBuffers, x_index = 0, c_index = 0, in_h_index = 0, out_h_index = 0, out_obj_index = 0;
        size_t in_rd_offset = 0x0, out_rd_offset = 0, pb_offset = 0;
        size_t c_sz_offset = MetaInfo::InRawArgSize + (0x10 - ((uintptr_t)ctx->request.Raw - (uintptr_t)ctx->request.RawWithoutPadding));
        return DecodeTuple<typename MetaInfo::Args>::GetArgs(ctx, a_index, b_index, x_index, c_index,  in_h_index, out_h_index, out_obj_index, in_rd_offset, out_rd_offset, pb_offset, c_sz_offset);
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
            *resp_header = {0};
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
            raw = (decltype(raw))ipcPrepareHeaderForDomain(&ctx->reply, sizeof(*raw) + MetaInfo::OutRawArgSize, 0);
            auto resp_header = (DomainResponseHeader *)((uintptr_t)raw - sizeof(DomainResponseHeader));
            *resp_header = {0};
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

template<auto IpcCommandImpl>
constexpr Result WrapIpcCommandImpl(IpcResponseContext *ctx) {
    using InArgs = typename PopFront<typename boost::callable_traits::args_t<decltype(IpcCommandImpl)>>::type;
    using OutArgs = typename boost::callable_traits::return_type_t<decltype(IpcCommandImpl)>;
    using ClassType = typename boost::callable_traits::class_of_t<decltype(IpcCommandImpl)>;
    
    using CommandMetaData = CommandMetaInfo<InArgs, OutArgs>;
        
    static_assert(CommandMetaData::ReturnsResult || CommandMetaData::ReturnsVoid, "IpcCommandImpls must return Result or void");
                
    ipcInitialize(&ctx->reply);
    memset(ctx->out_data, 0, CommandMetaData::OutRawArgSize);

    Result rc = Validator::Validate<CommandMetaData>(ctx);
        
    if (R_FAILED(rc)) {
        return 0xAAEE;
    }
    
    ClassType *this_ptr = nullptr;
    if (IsDomainObject(ctx->obj_holder)) {
        this_ptr = ctx->obj_holder->GetServiceObject<IDomainObject>()->GetObject(ctx->request.InThisObjectId)->GetServiceObject<ClassType>();
    } else {
        this_ptr = ctx->obj_holder->GetServiceObject<ClassType>();
    }
    if (this_ptr == nullptr) {
        return 0xBBEE;
    }
    
    std::shared_ptr<IServiceObject> out_objects[CommandMetaData::NumOutSessions];
    
    /* Allocate out object IDs. */
    size_t num_out_objects;
    if (IsDomainObject(ctx->obj_holder)) {
        for (num_out_objects = 0; num_out_objects < CommandMetaData::NumOutSessions; num_out_objects++) {
            if (R_FAILED((rc = ctx->obj_holder->GetServiceObject<IDomainObject>()->ReserveObject(&ctx->out_object_ids[num_out_objects])))) {
                break;
            }
            ctx->out_objs[num_out_objects] = &out_objects[num_out_objects];
        }
    } else {
        for (num_out_objects = 0; num_out_objects < CommandMetaData::NumOutSessions; num_out_objects++) {
            Handle server_h, client_h;
            if (R_FAILED((rc = SessionManagerBase::CreateSessionHandles(&server_h, &client_h)))) {
                break;
            }
            ctx->out_object_server_handles[num_out_objects] = server_h;
            ctx->out_handles[CommandMetaData::NumOutHandles + num_out_objects].handle = client_h;
            ctx->out_objs[num_out_objects] = &out_objects[num_out_objects];
        }
    }
    
    ON_SCOPE_EXIT {
        /* Clean up objects as necessary. */
        if (IsDomainObject(ctx->obj_holder) && R_FAILED(rc)) {
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
    
    if (R_SUCCEEDED(rc)) {
        auto args = Decoder::Decode<CommandMetaData>(ctx);
        
        if constexpr (CommandMetaData::ReturnsResult) {
            rc = std::apply( [=](auto&&... args) { return (this_ptr->*IpcCommandImpl)(args...); }, args);
        } else {
            std::apply( [=](auto&&... args) { (this_ptr->*IpcCommandImpl)(args...); }, args);
        }
    
        if (R_SUCCEEDED(rc)) {
            std::apply(Encoder<CommandMetaData, decltype(args)>::EncodeSuccess, std::tuple_cat(std::make_tuple(ctx), args));
        } else {
            std::apply(Encoder<CommandMetaData, decltype(args)>::EncodeFailure, std::tuple_cat(std::make_tuple(ctx), std::make_tuple(rc)));
        }
    } else {
        std::apply(Encoder<CommandMetaData, typename CommandMetaData::Args>::EncodeFailure, std::tuple_cat(std::make_tuple(ctx), std::make_tuple(rc)));
    }
    
    return rc;
}


template <u32 c, auto CommandImpl, FirmwareVersion l = FirmwareVersion_Min, FirmwareVersion h = FirmwareVersion_Max>
inline static constexpr ServiceCommandMeta MakeServiceCommandMeta() {
    return {
        .fw_low = l,
        .fw_high = h,
        .cmd_id = c,
        .handler = WrapIpcCommandImpl<CommandImpl>,
    };
};


#pragma GCC diagnostic pop