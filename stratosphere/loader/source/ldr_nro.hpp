#pragma once
#include <switch.h>
#include <cstdio>

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
        
        
        static_assert(sizeof(NrrHeader) == 0x350, "Incorrectly defined NrrHeader!");
        static Result ValidateNrrHeader(NrrHeader *header, u64 size, u64 title_id_min);
};