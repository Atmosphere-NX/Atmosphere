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

#include "ro_nrr.hpp"
#include "ro_types.hpp"

class Registration {
    public:
        /* NOTE: 2 ldr:ro, 2 ro:1. Nintendo only actually supports 2 total, but we'll be a little more generous. */
        static constexpr size_t MaxSessions = 0x4;
        static constexpr size_t MaxNrrInfos = 0x40;
        static constexpr size_t MaxNroInfos = 0x40;

        static constexpr u32 MagicNro0 = 0x304F524E;
    public:
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
    private:
        static Result MapAndValidateNrr(NrrHeader **out_header, u64 *out_mapped_code_address, Handle process_handle, u64 title_id, u64 nrr_heap_address, u64 nrr_heap_size, RoModuleType expected_type, bool enforce_type);
        static Result UnmapNrr(Handle process_handle, const NrrHeader *header, u64 nrr_heap_address, u64 nrr_heap_size, u64 mapped_code_address);
        static bool IsNroHashPresent(RoProcessContext *context, const Sha256Hash *hash);

        static Result MapNro(u64 *out_base_address, Handle process_handle, u64 nro_heap_address, u64 nro_heap_size, u64 bss_heap_address, u64 bss_heap_size);
        static Result ValidateNro(ModuleId *out_module_id, u64 *out_rx_size, u64 *out_ro_size, u64 *out_rw_size, RoProcessContext *context, u64 base_address, u64 nro_size, u64 bss_size);
        static Result SetNroPerms(Handle process_handle, u64 base_address, u64 rx_size, u64 ro_size, u64 rw_size);
        static Result UnmapNro(Handle process_handle, u64 base_address, u64 nro_heap_address, u64 bss_heap_address, u64 bss_heap_size, u64 code_size, u64 rw_size);
    public:
        static void Initialize();
        static bool ShouldEaseNroRestriction();

        static Result RegisterProcess(RoProcessContext **out_context, Handle process_handle, u64 process_id);
        static void   UnregisterProcess(RoProcessContext *context);

        static Result LoadNrr(RoProcessContext *context, u64 title_id, u64 nrr_address, u64 nrr_size, RoModuleType expected_type, bool enforce_type);
        static Result UnloadNrr(RoProcessContext *context, u64 nrr_address);
        static Result LoadNro(u64 *out_address, RoProcessContext *context, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size);
        static Result UnloadNro(RoProcessContext *context, u64 nro_address);

        static Result GetProcessModuleInfo(u32 *out_count, LoaderModuleInfo *out_infos, size_t max_out_count, u64 process_id);
};