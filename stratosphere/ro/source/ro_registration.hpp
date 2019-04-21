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

#include <stratosphere.hpp>

class Registration {
    public:
        static constexpr size_t MaxSessions = 0x8;
        static constexpr size_t MaxNrrInfos = 0x40;
        static constexpr size_t MaxNroInfos = 0x40;
    public:
        struct NrrHeader {
            u32 magic;
            u32 _0x4;
            u32 _0x8;
            u32 _0xC;
            u64 title_id_mask;
            u64 title_id_pattern;
            u64 _0x20;
            u64 _0x28;
            u8  modulus[0x100];
            u8  fixed_key_signature[0x100];
            u8  nrr_signature[0x100];
            u64 title_id_min;
            u32 nrr_size;
            u8  nrr_type; /* 7.0.0+ */
            u8  _0x33D[3];
            u32 hash_offset;
            u32 num_hashes;
            u64 _0x348;
        };
        static_assert(sizeof(NrrHeader) == 0x350, "NrrHeader definition!");

        struct NroHeader {
            u32 entrypoint_insn;
            u32 mod_offset;
            u64 padding;
            u32 magic;
            u32 _0x14;
            u32 nro_size;
            u32 _0x1C;
            u32 text_offset;
            u32 text_size;
            u32 ro_offset;
            u32 ro_size;
            u32 rw_offset;
            u32 rw_size;
            u32 bss_size;
            u32 _0x3C;
            unsigned char build_id[0x20];
            u8 _0x60[0x20];
        };
        static_assert(sizeof(NroHeader) == 0x80, "NroHeader definition!");

        struct ModuleId {
            u8 build_id[0x20];
        };
        static_assert(sizeof(ModuleId) == sizeof(LoaderModuleInfo::build_id), "ModuleId definition!");

        struct NroInfo {
            u64 base_address;
            u64 nro_heap_address;
            u64 nro_heap_size;
            u64 bss_heap_address;
            u64 bss_heap_size;
            u64 code_size;
            u64 rw_size;
            ModuleId module_id;
        };
        struct NrrInfo {
            NrrHeader *header;
            u64 nrr_heap_address;
            u64 nrr_heap_size;
            u64 mapped_code_address;
        };
        struct RoProcessContext {
            bool nro_in_use[MaxNroInfos];
            bool nrr_in_use[MaxNrrInfos];
            NroInfo nro_infos[MaxNroInfos];
            NrrInfo nrr_infos[MaxNrrInfos];
            Handle process_handle;
            u64 process_id;
            bool in_use;
        };
    public:
        static Result RegisterProcess(RoProcessContext **out_context, Handle process_handle, u64 process_id);
        static void   UnregisterProcess(RoProcessContext *context);

        static Result GetProcessModuleInfo(u32 *out_count, LoaderModuleInfo *out_infos, size_t max_out_count, u64 process_id);

        static Result UnmapNrr(Handle process_handle, const NrrHeader *header, u64 nrr_heap_address, u64 nrr_heap_size, u64 mapped_code_address);
};