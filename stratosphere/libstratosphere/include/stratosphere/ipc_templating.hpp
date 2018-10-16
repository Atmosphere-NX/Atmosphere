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
#include "../boost/callable_traits.hpp"
#include <type_traits>

#include "domainowner.hpp"

#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

/* Base for In/Out Buffers. */
struct IpcBufferBase {};

/* Represents an A descriptor. */
struct InBufferBase : IpcBufferBase {};

template <typename T, BufferType e_t = BufferType_Normal>
struct InBuffer : InBufferBase {
    T *buffer;
    size_t num_elements;
    BufferType type;
    static const BufferType expected_type = e_t;
    
    InBuffer(void *b, size_t n, BufferType t) : buffer((T *)b), num_elements(n/sizeof(T)), type(t) { }
};

/* Represents a B descriptor. */
struct OutBufferBase : IpcBufferBase {};

template <typename T, BufferType e_t = BufferType_Normal>
struct OutBuffer : OutBufferBase {
    T *buffer;
    size_t num_elements;
    BufferType type;
    static const BufferType expected_type = e_t;
    
    OutBuffer(void *b, size_t n, BufferType t) : buffer((T *)b), num_elements(n/sizeof(T)), type(t) { }
};

/* Represents an X descriptor. */
template <typename T>
struct InPointer : IpcBufferBase {
    T *pointer;
    size_t num_elements;
    
    InPointer(void *p, size_t n) : pointer((T *)p), num_elements(n/sizeof(T)) { }
};

/* Represents a C descriptor. */
struct OutPointerWithServerSizeBase : IpcBufferBase {};
 
template <typename T, size_t n>
struct OutPointerWithServerSize : OutPointerWithServerSizeBase {
    T *pointer;
    static const size_t num_elements = n;
    
    OutPointerWithServerSize(void *p) : pointer((T *)p) { }
};

/* Represents a C descriptor with size in raw data. */
template <typename T>
struct OutPointerWithClientSize : IpcBufferBase {
    T *pointer;
    size_t num_elements;
    
    OutPointerWithClientSize(void *p, size_t n) : pointer((T *)p), num_elements(n/sizeof(T)) { }
};

/* Represents an input PID. */
struct PidDescriptor {
    u64 pid;
    
    PidDescriptor(u64 p) : pid(p) { }
};

/* Represents a moved handle. */
struct MovedHandle {
    Handle handle;
    
    MovedHandle(Handle h) : handle(h) { }
};

/* Represents a copied handle. */
struct CopiedHandle {
    Handle handle;
    
    CopiedHandle(Handle h) : handle(h) { }
};

/* Forward declarations. */
template <typename T>
class ISession;

/* Represents an output ServiceObject. */
struct OutSessionBase {};

template <typename T>
struct OutSession : OutSessionBase {
    ISession<T> *session;
    u32 domain_id;
    
    OutSession(ISession<T> *s) : session(s), domain_id(DOMAIN_ID_MAX) { }
};

/* Utilities. */
template <typename T, template <typename...> class Template>
struct is_specialization_of {
    static const bool value = false;
};

template <template <typename...> class Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> {
    static const bool value = true;
};

template<typename Tuple>
struct pop_front;

template<typename Head, typename... Tail>
struct pop_front<std::tuple<Head, Tail...>> {
    using type = std::tuple<Tail...>;
};

template <typename T>
struct is_ipc_buffer {
    static const bool value = std::is_base_of<IpcBufferBase, T>::value;
};

template <typename T>
struct is_ipc_handle {
    static const size_t value = (std::is_same<T, MovedHandle>::value || std::is_same<T, CopiedHandle>::value) ? 1 : 0;
};

template <typename T>
struct is_out_session {
    static const bool value = std::is_base_of<OutSessionBase, T>::value;
};

template <typename T>
struct size_in_raw_data {
    static const size_t value = (is_ipc_buffer<T>::value || is_ipc_handle<T>::value || is_out_session<T>::value) ? 0 : ((sizeof(T) < sizeof(u32)) ? sizeof(u32) : (sizeof(T) + 3) & (~3));
};

template <typename ...Args>
struct size_in_raw_data_for_arguments {
    static const size_t value = (size_in_raw_data<Args>::value + ... + 0);
};

template <typename ...Args>
struct num_out_sessions_in_arguments {
    static const size_t value = ((is_out_session<Args>::value ? 1 : 0) + ... + 0);
};

template <typename T>
struct size_in_raw_data_with_out_pointers {
    static const size_t value = is_specialization_of<T, OutPointerWithClientSize>::value ? 2 : size_in_raw_data<T>::value;
};

template <typename ...Args>
struct size_in_raw_data_with_out_pointers_for_arguments {
    static const size_t value = ((size_in_raw_data_with_out_pointers<Args>::value + ... + 0) + 3) & ~3;
};

template <typename T>
struct is_ipc_inbuffer {
    static const size_t value = (std::is_base_of<InBufferBase, T>::value) ? 1 : 0;
};

template <typename ...Args>
struct num_inbuffers_in_arguments {
    static const size_t value = (is_ipc_inbuffer<Args>::value + ... + 0);
};

template <typename T>
struct is_ipc_inpointer {
    static const size_t value = (is_specialization_of<T, InPointer>::value) ? 1 : 0;
};

template <typename ...Args>
struct num_inpointers_in_arguments {
    static const size_t value = (is_ipc_inpointer<Args>::value + ... + 0);
};

template <typename T>
struct is_ipc_outpointer {
    static const size_t value = (is_specialization_of<T, OutPointerWithClientSize>::value || std::is_base_of<OutPointerWithServerSizeBase, T>::value) ? 1 : 0;
};

template <typename ...Args>
struct num_outpointers_in_arguments {
    static const size_t value = (is_ipc_outpointer<Args>::value + ... + 0);
};

template <typename T>
struct is_ipc_inoutbuffer {
    static const size_t value = (std::is_base_of<InBufferBase, T>::value || std::is_base_of<OutBufferBase, T>::value) ? 1 : 0;
};

template <typename ...Args>
struct num_inoutbuffers_in_arguments {
    static const size_t value = (is_ipc_inoutbuffer<Args>::value + ... + 0);
};

template <typename ...Args>
struct num_handles_in_arguments {
    static const size_t value = (is_ipc_handle<Args>::value + ... + 0);
};

template <typename ...Args>
struct num_pids_in_arguments {
    static const size_t value = ((std::is_same<Args, PidDescriptor>::value ? 1 : 0) + ... + 0);
};

template<typename T>
T GetValueFromIpcParsedCommand(IpcParsedCommand& r, IpcCommand& out_c, u8 *pointer_buffer, size_t& pointer_buffer_offset, size_t& cur_rawdata_index, size_t& cur_c_size_offset, size_t& a_index, size_t& b_index, size_t& x_index, size_t& c_index, size_t& h_index) {
    const size_t old_rawdata_index = cur_rawdata_index;
    const size_t old_c_size_offset = cur_c_size_offset;
    const size_t old_pointer_buffer_offset = pointer_buffer_offset;
    if constexpr (std::is_base_of<InBufferBase, T>::value) {
        const T& value = T(r.Buffers[a_index], r.BufferSizes[a_index], r.BufferTypes[a_index]);
        ++a_index;
        return value;
    } else if constexpr (std::is_base_of<OutBufferBase, T>::value) {
        const T& value = T(r.Buffers[b_index], r.BufferSizes[b_index], r.BufferTypes[b_index]);
        ++b_index;
        return value;
    } else if constexpr (is_specialization_of<T, InPointer>::value) {
        const T& value{r.Statics[x_index], r.StaticSizes[x_index]};
        ++x_index;
        return value;
    } else if constexpr (std::is_base_of<OutPointerWithServerSizeBase, T>::value) {
        T t = T(pointer_buffer + old_pointer_buffer_offset);
        ipcAddSendStatic(&out_c, pointer_buffer + old_pointer_buffer_offset, t.num_elements * sizeof(*t.pointer), c_index++);
        return t;
    } else if constexpr (is_specialization_of<T, OutPointerWithClientSize>::value) {
        cur_c_size_offset += sizeof(u16);
        u16 sz = *((u16 *)((u8 *)(r.Raw) + old_c_size_offset));
        pointer_buffer_offset += sz;
        ipcAddSendStatic(&out_c, pointer_buffer + old_pointer_buffer_offset, sz, c_index++);
        return T(pointer_buffer + old_pointer_buffer_offset, sz);
    } else if constexpr (is_ipc_handle<T>::value) {
        return r.Handles[h_index++];
    } else if constexpr (std::is_same<T, PidDescriptor>::value) {
        cur_rawdata_index += sizeof(u64) / sizeof(u32);
        return PidDescriptor(r.Pid);
    } else if constexpr (std::is_same<T, bool>::value) {
        /* Official bools accept non-zero values with low bit unset as false. */
        cur_rawdata_index += size_in_raw_data<T>::value / sizeof(u32);
        return ((*(((u32 *)r.Raw + old_rawdata_index))) & 1) == 1;
    } else {
        cur_rawdata_index += size_in_raw_data<T>::value / sizeof(u32);
        return *((T *)((u32 *)r.Raw + old_rawdata_index));
    }
}

template <typename T>
bool ValidateIpcParsedCommandArgument(IpcParsedCommand& r, size_t& cur_rawdata_index, size_t& cur_c_size_offset, size_t& a_index, size_t& b_index, size_t& x_index, size_t& c_index, size_t& h_index, size_t& total_c_size) {
    const size_t old_c_size_offset = cur_c_size_offset;
    if constexpr (std::is_base_of<InBufferBase, T>::value) {
        return r.Buffers[a_index] != NULL && r.BufferDirections[a_index] == BufferDirection_Send && r.BufferTypes[a_index++] == T::expected_type;
    } else if constexpr (std::is_base_of<OutBufferBase, T>::value) {
        return r.Buffers[b_index] != NULL && r.BufferDirections[b_index] == BufferDirection_Recv && r.BufferTypes[b_index++] == T::expected_type;
    } else if constexpr (is_specialization_of<T, InPointer>::value) {
        return r.Statics[x_index] != NULL;
    } else if constexpr (std::is_base_of<OutPointerWithServerSizeBase, T>::value) {
        total_c_size += T::num_elements;
        return true;
    } else if constexpr (is_specialization_of<T, OutPointerWithClientSize>::value) {
        cur_c_size_offset += sizeof(u16);
        u16 sz = *((u16 *)((u8 *)(r.Raw) + old_c_size_offset));
        total_c_size += sz;
        return true;
    } else if constexpr (std::is_same<T, MovedHandle>::value) {
        return !r.WasHandleCopied[h_index++];
    } else if constexpr (std::is_same<T, CopiedHandle>::value) {
        return r.WasHandleCopied[h_index++];
    } else {
        return true;
    }
}

/* Validator. */
template <typename ArgsTuple>
struct Validator;

template<typename... Args>
struct Validator<std::tuple<Args...>> {
    IpcParsedCommand &r;
    size_t pointer_buffer_size;
    
	Result operator()() {
        if (r.RawSize < size_in_raw_data_with_out_pointers_for_arguments<Args... >::value) {
            return 0xF601;
        }
                        
        if (r.NumBuffers != num_inoutbuffers_in_arguments<Args... >::value) {
            return 0xF601;
        }
        
        if (r.NumStatics != num_inpointers_in_arguments<Args... >::value) {
            return 0xF601;
        }

        if (r.NumStaticsOut != num_outpointers_in_arguments<Args... >::value) {
            return 0xF601;
        }
        
        if (r.NumHandles != num_handles_in_arguments<Args... >::value) {
            return 0xF601;
        }
        
        constexpr size_t num_pids = num_pids_in_arguments<Args... >::value;
        
        static_assert(num_pids <= 1, "Number of PID descriptors in IpcCommandImpl cannot be > 1");
        
        if ((r.HasPid && num_pids == 0) || (!r.HasPid && num_pids)) {
            return 0xF601;
        }
        
        if (((u32 *)r.Raw)[0] != SFCI_MAGIC) {
            //return 0xF601;
        }
        
        size_t a_index = 0, b_index = num_inbuffers_in_arguments<Args ...>::value, x_index = 0, c_index = 0, h_index = 0;
		size_t cur_rawdata_index = 4;
        size_t cur_c_size_offset = 0x10 + size_in_raw_data_for_arguments<Args... >::value + (0x10 - ((uintptr_t)r.Raw - (uintptr_t)r.RawWithoutPadding));
        size_t total_c_size = 0;
                
        if (!(ValidateIpcParsedCommandArgument<Args>(r, cur_rawdata_index, cur_c_size_offset, a_index, b_index, x_index, c_index, h_index, total_c_size) && ...)) {
            return 0xF601;
        }
        
        if (total_c_size > pointer_buffer_size) {
            return 0xF601;
        }

        return 0;
	}
};


/* Decoder. */
template<typename OutTuple, typename ArgsTuple>
struct Decoder;

template<typename OutTuple, typename... Args>
struct Decoder<OutTuple, std::tuple<Args...>> {
	static std::tuple<Args...> Decode(IpcParsedCommand& r, IpcCommand &out_c, u8 *pointer_buffer) {
        size_t a_index = 0, b_index = num_inbuffers_in_arguments<Args ...>::value, x_index = 0, c_index = 0, h_index = 0;
		size_t cur_rawdata_index = 4;
        size_t cur_c_size_offset = 0x10 + size_in_raw_data_for_arguments<Args... >::value + (0x10 - ((uintptr_t)r.Raw - (uintptr_t)r.RawWithoutPadding));
        size_t pointer_buffer_offset = 0;
		return std::tuple<Args... > {
				GetValueFromIpcParsedCommand<Args>(r, out_c, pointer_buffer, pointer_buffer_offset, cur_rawdata_index, cur_c_size_offset, a_index, b_index, x_index, c_index, h_index)
				...
		};
	}
};

/* Encoder. */
template<typename ArgsTuple>
struct Encoder;

template<typename T>
constexpr size_t GetAndUpdateOffsetIntoRawData(DomainOwner *domain_owner, size_t& offset) {
	auto old = offset;
            
    if (old == 0) {
        offset += sizeof(u64);
    } else {
        if constexpr (is_out_session<T>::value) {
            if (domain_owner) {
                offset += sizeof(u32);
            }
        } else {
            offset += size_in_raw_data<T>::value;   
        } 
    }

	return old;
}

template<typename T>
void EncodeValueIntoIpcMessageBeforePrepare(DomainOwner *domain_owner, IpcCommand *c, T &value) {
    if constexpr (std::is_same<T, MovedHandle>::value) {
        ipcSendHandleMove(c, value.handle);
    } else if constexpr (std::is_same<T, CopiedHandle>::value) {
        ipcSendHandleCopy(c, value.handle);
    } else if constexpr (std::is_same<T, PidDescriptor>::value) {
        ipcSendPid(c);
    } else if constexpr (is_out_session<T>::value) {
        if (domain_owner && value.session) {
            /* TODO: Check error... */
            if (value.domain_id != DOMAIN_ID_MAX) {
                domain_owner->set_object(value.session->get_service_object(), value.domain_id);
            } else {
                domain_owner->reserve_object(value.session->get_service_object(), &value.domain_id);
            }
            value.session->close_handles();
            delete value.session;
        } else {
            ipcSendHandleMove(c, value.session ? value.session->get_client_handle() : 0x0);
        }
    }
}

template<typename T>
void EncodeValueIntoIpcMessageAfterPrepare(DomainOwner *domain_owner, u8 *cur_out, T value) {
    if constexpr (is_ipc_handle<T>::value || std::is_same<T, PidDescriptor>::value) {
        /* Do nothing. */
    } else if constexpr (is_out_session<T>::value) {
        if (domain_owner) {
            *((u32 *)cur_out) = value.domain_id;
        }
    } else {
        *((T *)(cur_out)) = value;
    }
}

template<typename... Args>
struct Encoder<std::tuple<Args...>> {
    IpcCommand &out_command;
    
	auto operator()(DomainOwner *domain_owner, Args... args) {
        static_assert(sizeof...(Args) > 0, "IpcCommandImpls must return std::tuple<Result, ...>");
		size_t offset = 0;
        
        u8 *tls = (u8 *)armGetTls();
        
        std::fill(tls, tls + 0x100, 0x00);
        
        ((EncodeValueIntoIpcMessageBeforePrepare<Args>(domain_owner, &out_command, args)), ...);
        
        /* Remove the extra space resulting from first Result type. */
        struct {
            u64 magic;
            u64 result;
        } *raw;
        if (domain_owner == NULL) {
            raw = (decltype(raw))ipcPrepareHeader(&out_command, sizeof(*raw) + size_in_raw_data_for_arguments<Args... >::value - sizeof(Result));
        } else {
            raw = (decltype(raw))ipcPrepareHeaderForDomain(&out_command, sizeof(*raw) + size_in_raw_data_for_arguments<Args... >::value + (num_out_sessions_in_arguments<Args... >::value * sizeof(u32)) - sizeof(Result), 0);
            auto resp_header = (DomainResponseHeader *)((uintptr_t)raw - sizeof(DomainResponseHeader));
            *resp_header = {0};
            resp_header->NumObjectIds = num_out_sessions_in_arguments<Args... >::value;
        }
        
        
        raw->magic = SFCO_MAGIC;
        
        u8 *raw_data = (u8 *)&raw->result;
        
        ((EncodeValueIntoIpcMessageAfterPrepare<Args>(domain_owner, raw_data + GetAndUpdateOffsetIntoRawData<Args>(domain_owner, offset), args)), ...);
        
        Result rc = raw->result;
                        
        if (R_FAILED(rc)) {
            std::fill(tls, tls + 0x100, 0x00);
            ipcInitialize(&out_command);
            if (domain_owner != NULL) {
                raw = (decltype(raw))ipcPrepareHeaderForDomain(&out_command, sizeof(raw), 0);
                auto resp_header = (DomainResponseHeader *)((uintptr_t)raw - sizeof(DomainResponseHeader));
                *resp_header = {0};
            } else {
                raw = (decltype(raw))ipcPrepareHeader(&out_command, sizeof(raw));
            }
            raw->magic = SFCO_MAGIC;
            raw->result = rc;
        }
                
        return rc;
	}
};


template<auto IpcCommandImpl, typename Class, typename... Args>
Result WrapDeferredIpcCommandImpl(Class *this_ptr, Args... args) {
    using InArgs = typename boost::callable_traits::args_t<decltype(IpcCommandImpl)>;
    using InArgsWithoutThis = typename pop_front<InArgs>::type;
    using OutArgs = typename boost::callable_traits::return_type_t<decltype(IpcCommandImpl)>;
    
    static_assert(is_specialization_of<OutArgs, std::tuple>::value, "IpcCommandImpls must return std::tuple<Result, ...>");
    static_assert(std::is_same_v<std::tuple_element_t<0, OutArgs>, Result>, "IpcCommandImpls must return std::tuple<Result, ...>");
    static_assert(std::is_same_v<InArgsWithoutThis, std::tuple<Args...>>, "Invalid Deferred Wrapped IpcCommandImpl arguments!");
    
    IpcCommand out_command;
    
    ipcInitialize(&out_command);

    auto tuple_args = std::make_tuple(args...);
    auto result = std::apply( [=](auto&&... a) { return (this_ptr->*IpcCommandImpl)(a...); }, tuple_args);
    
    DomainOwner *down = NULL;
    
    return std::apply(Encoder<OutArgs>{out_command}, std::tuple_cat(std::make_tuple(down), result));
}

template<auto IpcCommandImpl, typename Class>
Result WrapIpcCommandImpl(Class *this_ptr, IpcParsedCommand& r, IpcCommand &out_command, u8 *pointer_buffer, size_t pointer_buffer_size) {
    using InArgs = typename boost::callable_traits::args_t<decltype(IpcCommandImpl)>;
    using InArgsWithoutThis = typename pop_front<InArgs>::type;
    using OutArgs = typename boost::callable_traits::return_type_t<decltype(IpcCommandImpl)>;
    
    static_assert(is_specialization_of<OutArgs, std::tuple>::value, "IpcCommandImpls must return std::tuple<Result, ...>");
    static_assert(std::is_same_v<std::tuple_element_t<0, OutArgs>, Result>, "IpcCommandImpls must return std::tuple<Result, ...>");
    
    ipcInitialize(&out_command);

    Result rc = Validator<InArgsWithoutThis>{r, pointer_buffer_size}();
        
    if (R_FAILED(rc)) {
        return 0xF601;
    }

    auto args = Decoder<OutArgs, InArgsWithoutThis>::Decode(r, out_command, pointer_buffer);    
    auto result = std::apply( [=](auto&&... args) { return (this_ptr->*IpcCommandImpl)(args...); }, args);
    DomainOwner *down = NULL;
    if (r.IsDomainRequest) {
        down = this_ptr->get_owner();
    }
    
    return std::apply(Encoder<OutArgs>{out_command}, std::tuple_cat(std::make_tuple(down), result));
}

template<auto IpcCommandImpl>
Result WrapStaticIpcCommandImpl(IpcParsedCommand& r, IpcCommand &out_command, u8 *pointer_buffer, size_t pointer_buffer_size) {
    using InArgs = typename boost::callable_traits::args_t<decltype(IpcCommandImpl)>;
    using OutArgs = typename boost::callable_traits::return_type_t<decltype(IpcCommandImpl)>;
    
    static_assert(is_specialization_of<OutArgs, std::tuple>::value, "IpcCommandImpls must return std::tuple<Result, ...>");
    static_assert(std::is_same_v<std::tuple_element_t<0, OutArgs>, Result>, "IpcCommandImpls must return std::tuple<Result, ...>");
    
    ipcInitialize(&out_command);

    Result rc = Validator<InArgs>{r, pointer_buffer_size}();
        
    if (R_FAILED(rc)) {
        return 0xF601;
    }

    auto args = Decoder<OutArgs, InArgs>::Decode(r, out_command, pointer_buffer);    
    auto result = std::apply(IpcCommandImpl, args);
    DomainOwner *down = NULL;
    
    return std::apply(Encoder<OutArgs>{out_command}, std::tuple_cat(std::make_tuple(down), result));
}

#pragma GCC diagnostic pop

#include "isession.hpp"
