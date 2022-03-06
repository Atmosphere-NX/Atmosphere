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

#if defined(ATMOSPHERE_OS_WINDOWS) || defined(ATMOSPHERE_OS_LINUX) || defined(ATMOSPHERE_OS_MACOS)

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ATMOSPHERE_COMPILER_CLANG)
#define AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR ALWAYS_INLINE
#else
#define AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR constexpr ALWAYS_INLINE
#endif

#define HIPC_AUTO_RECV_STATIC UINT8_MAX
#define HIPC_RESPONSE_NO_PID  UINT32_MAX

typedef struct HipcMetadata {
    u32 type;
    u32 num_send_statics;
    u32 num_send_buffers;
    u32 num_recv_buffers;
    u32 num_exch_buffers;
    u32 num_data_words;
    u32 num_recv_statics; // also accepts HIPC_AUTO_RECV_STATIC
    u32 send_pid;
    u32 num_copy_handles;
    u32 num_move_handles;
} HipcMetadata;

typedef struct HipcHeader {
    u32 type               : 16;
    u32 num_send_statics   : 4;
    u32 num_send_buffers   : 4;
    u32 num_recv_buffers   : 4;
    u32 num_exch_buffers   : 4;
    u32 num_data_words     : 10;
    u32 recv_static_mode   : 4;
    u32 padding            : 6;
    u32 recv_list_offset   : 11; // Unused.
    u32 has_special_header : 1;
} HipcHeader;

typedef struct HipcSpecialHeader {
    u32 send_pid         : 1;
    u32 num_copy_handles : 4;
    u32 num_move_handles : 4;
    u32 padding          : 23;
} HipcSpecialHeader;

typedef struct HipcStaticDescriptor {
    u32 index        : 6;
    u32 address_high : 6;
    u32 address_mid  : 4;
    u32 size         : 16;
    u32 address_low;
} HipcStaticDescriptor;

typedef struct HipcBufferDescriptor {
    u32 size_low;
    u32 address_low;
    u32 mode         : 2;
    u32 address_high : 22;
    u32 size_high    : 4;
    u32 address_mid  : 4;
} HipcBufferDescriptor;

typedef struct HipcRecvListEntry {
    u32 address_low;
    u32 address_high : 16;
    u32 size         : 16;
} HipcRecvListEntry;

typedef struct HipcRequest {
    HipcStaticDescriptor* send_statics;
    HipcBufferDescriptor* send_buffers;
    HipcBufferDescriptor* recv_buffers;
    HipcBufferDescriptor* exch_buffers;
    u32* data_words;
    HipcRecvListEntry* recv_list;
    u32* copy_handles;
    u32* move_handles;
} HipcRequest;

typedef struct HipcParsedRequest {
    HipcMetadata meta;
    HipcRequest data;
    u64 pid;
} HipcParsedRequest;

typedef struct HipcResponse {
    u64 pid;
    u32 num_statics;
    u32 num_data_words;
    u32 num_copy_handles;
    u32 num_move_handles;
    HipcStaticDescriptor* statics;
    u32* data_words;
    u32* copy_handles;
    u32* move_handles;
} HipcResponse;

typedef enum HipcBufferMode {
    HipcBufferMode_Normal    = 0,
    HipcBufferMode_NonSecure = 1,
    HipcBufferMode_Invalid   = 2,
    HipcBufferMode_NonDevice = 3,
} HipcBufferMode;


AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR HipcStaticDescriptor hipcMakeSendStatic(const void* buffer, size_t size, u8 index) {
    return (HipcStaticDescriptor){
        .index        = index,
        .address_high = (u32)((uintptr_t)buffer >> 36),
        .address_mid  = (u32)((uintptr_t)buffer >> 32),
        .size         = (u32)size,
        .address_low  = (u32)(uintptr_t)buffer,
    };
}

AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR HipcBufferDescriptor hipcMakeBuffer(const void* buffer, size_t size, HipcBufferMode mode) {
    return (HipcBufferDescriptor){
        .size_low     = (u32)size,
        .address_low  = (u32)(uintptr_t)buffer,
        .mode         = mode,
        .address_high = (u32)((uintptr_t)buffer >> 36),
        .size_high    = (u32)(size >> 32),
        .address_mid  = (u32)((uintptr_t)buffer >> 32),
    };
}

AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR HipcRecvListEntry hipcMakeRecvStatic(void* buffer, size_t size) {
    return (HipcRecvListEntry){
        .address_low  = (u32)((uintptr_t)buffer),
        .address_high = (u32)((uintptr_t)buffer >> 32),
        .size         = (u32)size,
    };
}

AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR void* hipcGetStaticAddress(const HipcStaticDescriptor* desc)
{
    return (void*)(desc->address_low | ((uintptr_t)desc->address_mid << 32) | ((uintptr_t)desc->address_high << 36));
}

AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR size_t hipcGetStaticSize(const HipcStaticDescriptor* desc)
{
    return desc->size;
}

AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR void* hipcGetBufferAddress(const HipcBufferDescriptor* desc)
{
    return (void*)(desc->address_low | ((uintptr_t)desc->address_mid << 32) | ((uintptr_t)desc->address_high << 36));
}

AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR size_t hipcGetBufferSize(const HipcBufferDescriptor* desc)
{
    return desc->size_low | ((size_t)desc->size_high << 32);
}

AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR HipcRequest hipcCalcRequestLayout(HipcMetadata meta, void* base) {
    // Copy handles
    u32* copy_handles = NULL;
    if (meta.num_copy_handles) {
        copy_handles = (u32*)base;
        base = copy_handles + meta.num_copy_handles;
    }

    // Move handles
    u32* move_handles = NULL;
    if (meta.num_move_handles) {
        move_handles = (u32*)base;
        base = move_handles + meta.num_move_handles;
    }

    // Send statics
    HipcStaticDescriptor* send_statics = NULL;
    if (meta.num_send_statics) {
        send_statics = (HipcStaticDescriptor*)base;
        base = send_statics + meta.num_send_statics;
    }

    // Send buffers
    HipcBufferDescriptor* send_buffers = NULL;
    if (meta.num_send_buffers) {
        send_buffers = (HipcBufferDescriptor*)base;
        base = send_buffers + meta.num_send_buffers;
    }

    // Recv buffers
    HipcBufferDescriptor* recv_buffers = NULL;
    if (meta.num_recv_buffers) {
        recv_buffers = (HipcBufferDescriptor*)base;
        base = recv_buffers + meta.num_recv_buffers;
    }

    // Exch buffers
    HipcBufferDescriptor* exch_buffers = NULL;
    if (meta.num_exch_buffers) {
        exch_buffers = (HipcBufferDescriptor*)base;
        base = exch_buffers + meta.num_exch_buffers;
    }

    // Data words
    u32* data_words = NULL;
    if (meta.num_data_words) {
        data_words = (u32*)base;
        base = data_words + meta.num_data_words;
    }

    // Recv list
    HipcRecvListEntry* recv_list = NULL;
    if (meta.num_recv_statics)
        recv_list = (HipcRecvListEntry*)base;

    return (HipcRequest){
        .send_statics = send_statics,
        .send_buffers = send_buffers,
        .recv_buffers = recv_buffers,
        .exch_buffers = exch_buffers,
        .data_words = data_words,
        .recv_list = recv_list,
        .copy_handles = copy_handles,
        .move_handles = move_handles,
    };
}

AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR HipcRequest hipcMakeRequest(void* base, HipcMetadata meta) {
    // Write message header
    bool has_special_header = meta.send_pid || meta.num_copy_handles || meta.num_move_handles;
    HipcHeader* hdr = (HipcHeader*)base;
    base = hdr+1;
    *hdr = (HipcHeader){
        .type = meta.type,
        .num_send_statics   = meta.num_send_statics,
        .num_send_buffers   = meta.num_send_buffers,
        .num_recv_buffers   = meta.num_recv_buffers,
        .num_exch_buffers   = meta.num_exch_buffers,
        .num_data_words     = meta.num_data_words,
        .recv_static_mode   = meta.num_recv_statics ? (meta.num_recv_statics != HIPC_AUTO_RECV_STATIC ? 2u + meta.num_recv_statics : 2u) : 0u,
        .padding            = 0,
        .recv_list_offset   = 0,
        .has_special_header = has_special_header,
    };

    // Write special header
    if (has_special_header) {
        HipcSpecialHeader* sphdr = (HipcSpecialHeader*)base;
        base = sphdr+1;
        *sphdr = (HipcSpecialHeader){
            .send_pid         = meta.send_pid,
            .num_copy_handles = meta.num_copy_handles,
            .num_move_handles = meta.num_move_handles,
        };
        if (meta.send_pid)
            base = (u8*)base + sizeof(u64);
    }

    // Calculate layout
    return hipcCalcRequestLayout(meta, base);
}

#define hipcMakeRequestInline(_base,...) hipcMakeRequest((_base),(HipcMetadata){ __VA_ARGS__ })

AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR HipcParsedRequest hipcParseRequest(void* base) {
    // Parse message header
    HipcHeader hdr = {};
    __builtin_memcpy(&hdr, base, sizeof(hdr));
    base = (u8*)base + sizeof(hdr);
    u32 num_recv_statics = 0;
    u64 pid = 0;

    // Parse recv static mode
    if (hdr.recv_static_mode) {
        if (hdr.recv_static_mode == 2u)
            num_recv_statics = HIPC_AUTO_RECV_STATIC;
        else if (hdr.recv_static_mode > 2u)
            num_recv_statics = hdr.recv_static_mode - 2u;
    }

    // Parse special header
    HipcSpecialHeader sphdr = {};
    if (hdr.has_special_header) {
        __builtin_memcpy(&sphdr, base, sizeof(sphdr));
        base = (u8*)base + sizeof(sphdr);

        // Read PID descriptor
        if (sphdr.send_pid) {
            pid = *(u64*)base;
            base = (u8*)base + sizeof(u64);
        }
    }

    const HipcMetadata meta = {
        .type             = hdr.type,
        .num_send_statics = hdr.num_send_statics,
        .num_send_buffers = hdr.num_send_buffers,
        .num_recv_buffers = hdr.num_recv_buffers,
        .num_exch_buffers = hdr.num_exch_buffers,
        .num_data_words   = hdr.num_data_words,
        .num_recv_statics = num_recv_statics,
        .send_pid         = sphdr.send_pid,
        .num_copy_handles = sphdr.num_copy_handles,
        .num_move_handles = sphdr.num_move_handles,
    };

    return (HipcParsedRequest){
        .meta = meta,
        .data = hipcCalcRequestLayout(meta, base),
        .pid  = pid,
    };
}

AMS_SF_HIPC_PARSE_IMPL_CONSTEXPR HipcResponse hipcParseResponse(void* base) {
    // Parse header
    HipcHeader hdr = {};
    __builtin_memcpy(&hdr, base, sizeof(hdr));
    base = (u8*)base + sizeof(hdr);

    // Initialize response
    HipcResponse response = {};
    response.num_statics = hdr.num_send_statics;
    response.num_data_words = hdr.num_data_words;
    response.pid = HIPC_RESPONSE_NO_PID;

    // Parse special header
    if (hdr.has_special_header)
    {
        HipcSpecialHeader sphdr = {};
        __builtin_memcpy(&sphdr, base, sizeof(sphdr));
        base = (u8*)base + sizeof(sphdr);

        // Update response
        response.num_copy_handles = sphdr.num_copy_handles;
        response.num_move_handles = sphdr.num_move_handles;

        // Parse PID descriptor
        if (sphdr.send_pid) {
            response.pid = *(u64*)base;
            base = (u8*)base + sizeof(u64);
        }
    }

    // Copy handles
    response.copy_handles = (u32*)base;
    base = response.copy_handles + response.num_copy_handles;

    // Move handles
    response.move_handles = (u32*)base;
    base = response.move_handles + response.num_move_handles;

    // Send statics
    response.statics = (HipcStaticDescriptor*)base;
    base = response.statics + response.num_statics;

    // Data words
    response.data_words = (u32*)base;

    return response;
}

typedef enum CmifCommandType {
    CmifCommandType_Invalid            = 0,
    CmifCommandType_LegacyRequest      = 1,
    CmifCommandType_Close              = 2,
    CmifCommandType_LegacyControl      = 3,
    CmifCommandType_Request            = 4,
    CmifCommandType_Control            = 5,
    CmifCommandType_RequestWithContext = 6,
    CmifCommandType_ControlWithContext = 7,
} CmifCommandType;

typedef enum CmifDomainRequestType {
    CmifDomainRequestType_Invalid     = 0,
    CmifDomainRequestType_SendMessage = 1,
    CmifDomainRequestType_Close       = 2,
} CmifDomainRequestType;

typedef struct CmifInHeader {
    u32 magic;
    u32 version;
    u32 command_id;
    u32 token;
} CmifInHeader;

typedef struct CmifOutHeader {
    u32 magic;
    u32 version;
    Result result;
    u32 token;
} CmifOutHeader;

typedef struct CmifDomainInHeader {
    u8  type;
    u8  num_in_objects;
    u16 data_size;
    u32 object_id;
    u32 padding;
    u32 token;
} CmifDomainInHeader;

typedef struct CmifDomainOutHeader {
    u32 num_out_objects;
    u32 padding[3];
} CmifDomainOutHeader;

typedef struct CmifRequestFormat {
    u32 object_id;
    u32 request_id;
    u32 context;
    u32 data_size;
    u32 server_pointer_size;
    u32 num_in_auto_buffers;
    u32 num_out_auto_buffers;
    u32 num_in_buffers;
    u32 num_out_buffers;
    u32 num_inout_buffers;
    u32 num_in_pointers;
    u32 num_out_pointers;
    u32 num_out_fixed_pointers;
    u32 num_objects;
    u32 num_handles;
    u32 send_pid;
} CmifRequestFormat;

typedef struct CmifRequest {
    HipcRequest hipc;
    void* data;
    u16* out_pointer_sizes;
    u32* objects;
    u32 server_pointer_size;
    u32 cur_in_ptr_id;
} CmifRequest;

typedef struct CmifResponse {
    void* data;
    u32* objects;
    u32* copy_handles;
    u32* move_handles;
} CmifResponse;

enum {
    SfBufferAttr_In                             = BIT(0),
    SfBufferAttr_Out                            = BIT(1),
    SfBufferAttr_HipcMapAlias                   = BIT(2),
    SfBufferAttr_HipcPointer                    = BIT(3),
    SfBufferAttr_FixedSize                      = BIT(4),
    SfBufferAttr_HipcAutoSelect                 = BIT(5),
    SfBufferAttr_HipcMapTransferAllowsNonSecure = BIT(6),
    SfBufferAttr_HipcMapTransferAllowsNonDevice = BIT(7),
};

typedef struct SfBufferAttrs {
    u32 attr0;
    u32 attr1;
    u32 attr2;
    u32 attr3;
    u32 attr4;
    u32 attr5;
    u32 attr6;
    u32 attr7;
} SfBufferAttrs;

typedef struct SfBuffer {
    const void* ptr;
    size_t size;
} SfBuffer;

typedef enum SfOutHandleAttr {
    SfOutHandleAttr_None     = 0,
    SfOutHandleAttr_HipcCopy = 1,
    SfOutHandleAttr_HipcMove = 2,
} SfOutHandleAttr;

typedef struct SfOutHandleAttrs {
    SfOutHandleAttr attr0;
    SfOutHandleAttr attr1;
    SfOutHandleAttr attr2;
    SfOutHandleAttr attr3;
    SfOutHandleAttr attr4;
    SfOutHandleAttr attr5;
    SfOutHandleAttr attr6;
    SfOutHandleAttr attr7;
} SfOutHandleAttrs;

#ifdef __cplusplus
}
#endif

#endif
