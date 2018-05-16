#include <stdbool.h>
#include "utils.h"
#include "se.h"
#include "fuse.h"
#include "pmc.h"
#include "panic_color.h"
#include "timers.h"

#include <stdio.h>
#include <inttypes.h>

__attribute__ ((noreturn)) void panic(uint32_t code) {
    /* Set Panic Code for NX_BOOTLOADER. */
    if (APBDEV_PMC_SCRATCH200_0 == 0) {
        APBDEV_PMC_SCRATCH200_0 = code;
    }

    /* TODO: Custom Panic Driver, which displays to screen without rebooting. */
    /* For now, just use NX BOOTLOADER's panic. */
    fuse_disable_programming();
    APBDEV_PMC_CRYPTO_OP_0 = 1; /* Disable all SE operations. */
    /* TODO: watchdog_reboot(); */
    while (1) { }
}

__attribute__ ((noreturn)) void generic_panic(void) {
    panic(0xFF000006);
}

__attribute__ ((noreturn)) void panic_predefined(uint32_t which) {
    static const uint32_t codes[0x10] = {COLOR_0, COLOR_1, COLOR_2, COLOR_3, COLOR_4, COLOR_5, COLOR_6, COLOR_7, COLOR_8, COLOR_9, COLOR_A, COLOR_B, COLOR_C, COLOR_D, COLOR_E, COLOR_F};
    panic(codes[which & 0xF]);
}

__attribute__((noinline)) bool overlaps(uint64_t as, uint64_t ae, uint64_t bs, uint64_t be)
{
    if(as <= bs && bs <= ae)
        return true;
    if(bs <= as && as <= be)
        return true;
    return false;
}

/* Adapted from https://gist.github.com/ccbrown/9722406 */
void hexdump(const void* data, size_t size, uintptr_t addrbase) {
    const uint8_t *d = (const uint8_t *)data;
    char ascii[17];
    ascii[16] = '\0';

    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) {
            printf("%0*" PRIXPTR ": | ", 2 * sizeof(addrbase), addrbase + i);
        }
        printf("%02X ", d[i]);
        if (d[i] >= ' ' && d[i] <= '~') {
            ascii[i % 16] = d[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            printf(" ");
            if ((i+1) % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    printf(" ");
                }
                for (size_t j = (i+1) % 16; j < 16; j++) {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}
