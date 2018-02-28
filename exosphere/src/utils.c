#include <stdbool.h>
#include "utils.h"
#include "se.h"
#include "fuse.h"
#include "pmc.h"
#include "timers.h"


__attribute__ ((noreturn)) void panic(uint32_t code) {
    /* Set Panic Code for NX_BOOTLOADER. */
    if (APBDEV_PMC_SCRATCH200_0 == 0) {
        APBDEV_PMC_SCRATCH200_0 = code;
    }
    
    /* TODO: Custom Panic Driver, which displays to screen without rebooting. */
    /* For now, just use NX BOOTLOADER's panic. */
    fuse_disable_programming();
    APBDEV_PMC_CRYPTO_OP_0 = 1; /* Disable all SE operations. */
    watchdog_reboot();
}

__attribute__ ((noreturn)) void generic_panic(void) {
    panic(0xFF000006);
}

__attribute__((noinline)) bool overlaps(uint64_t as, uint64_t ae, uint64_t bs, uint64_t be)
{
    if(as <= bs && bs <= ae)
        return true;
    if(bs <= as && as <= be)
        return true;
    return false;
}
