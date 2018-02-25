#include "utils.h"

void panic(uint32_t code) {
    (void)code; /* TODO */
}

void generic_panic(void) {
    /* TODO */
}

__attribute__((noinline)) bool overlaps(uint64_t as, uint64_t ae, uint64_t bs, uint64_t be)
{
    if(as <= bs && bs <= ae)
        return true;
    if(bs <= as && as <= be)
        return true;
    return false;
}
