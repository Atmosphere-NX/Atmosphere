#pragma once
#include <switch.h>

class MapUtils { 
    public:
        static Result LocateSpaceForMapDeprecated(u64 *out, u64 out_size);
        static Result LocateSpaceForMapModern(u64 *out, u64 out_size);
        static Result LocateSpaceForMap(u64 *out, u64 out_size);
};