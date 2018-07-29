#pragma once
#include <switch.h>
#include <cstdio>

#include "ldr_nso.hpp"

class PatchUtils {  
    public:
        static void ApplyPatches(u64 title_id, const NsoUtils::NsoHeader *header, u8 *mapped_nso, size_t size);
};