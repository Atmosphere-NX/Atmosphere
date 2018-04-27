#include <switch.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <picosha2.hpp>
#include "ldr_nro.hpp"
#include "ldr_map.hpp"
#include "ldr_random.hpp"

Result NroUtils::ValidateNrrHeader(NrrHeader *header, u64 size, u64 title_id_min) {
    if (header->magic != MAGIC_NRR0) {
        return 0x6A09;
    }
    if (header->nrr_size != size) {
        return 0xA409;
    }
    
    /* TODO: Check NRR signature. */
    if (false) {
        return 0x6C09;
    }
    
    if (header->title_id_min != title_id_min) {
        return 0x6A09;
    }
    
    return 0x0;
}