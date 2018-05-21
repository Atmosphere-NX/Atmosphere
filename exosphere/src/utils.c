#include <stdbool.h>
#include <string.h>
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
    
    strcpy((void *)MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_DEBUG_IRAM), (void *)"PANIC");
    MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x400ull) = 0x10;
    /* TODO: Custom Panic Driver, which displays to screen without rebooting. */
    /* For now, just use NX BOOTLOADER's panic. */
    fuse_disable_programming();
    APBDEV_PMC_CRYPTO_OP_0 = 1; /* Disable all SE operations. */
    while (1) { }
    watchdog_reboot();
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

uintptr_t get_iram_address_for_debug(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_DEBUG_IRAM);
}
