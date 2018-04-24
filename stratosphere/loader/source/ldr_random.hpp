#pragma once
#include <switch.h>

class RandomUtils {
    public:
        static u32 GetNext();
        static u32 GetRandomU32(u32 max);
        static u32 GetRandomU64(u64 max);
};