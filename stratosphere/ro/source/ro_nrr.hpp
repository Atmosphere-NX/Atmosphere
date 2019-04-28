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

enum RoModuleType : u32 {
    RoModuleType_ForSelf = 0,
    RoModuleType_ForOthers = 1,
};

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
    u64 title_id;
    u32 nrr_size;
    u8  nrr_type; /* 7.0.0+ */
    u8  _0x33D[3];
    u32 hash_offset;
    u32 num_hashes;
    u64 _0x348;
};
static_assert(sizeof(NrrHeader) == 0x350, "NrrHeader definition!");

class NrrUtils {
    public:
        static constexpr u32 MagicNrr0 = 0x3052524E;
    private:
        static Result ValidateNrrSignature(const NrrHeader *header);
    public:
        static Result ValidateNrr(const NrrHeader *header, u64 size, u64 title_id, RoModuleType expected_type, bool enforce_type);
};