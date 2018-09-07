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
#include <cstdio>

#include "ldr_registration.hpp"
#define MAGIC_NRO0 0x304F524E
#define MAGIC_NRR0 0x3052524E

class NroUtils {
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
            u32 _0x33C;
            u32 hash_offset;
            u32 num_hashes;
            u64 _0x348;
        };
        
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
        
        
        static_assert(sizeof(NrrHeader) == 0x350, "Incorrectly defined NrrHeader!");
        static_assert(sizeof(NroHeader) == 0x80, "Incorrectly defined NroHeader!");
        static Result ValidateNrrHeader(NrrHeader *header, u64 size, u64 title_id_min);
        static Result LoadNro(Registration::Process *target_proc, Handle process_h, u64 nro_heap_address, u64 nro_heap_size, u64 bss_heap_address, u64 bss_heap_size, u64 *out_address);
};