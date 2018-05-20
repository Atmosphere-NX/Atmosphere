#include <stdbool.h>
#include <stdarg.h>
#include "utils.h"
#include "se.h"
#include "fuse.h"
#include "pmc.h"
#include "timers.h"
#include "panic.h"

#include "lib/printk.h"
#include "hwinit/btn.h"

__attribute__((noreturn)) void watchdog_reboot(void) {
    volatile watchdog_timers_t *wdt = GET_WDT(4);
    wdt->PATTERN = WDT_REBOOT_PATTERN;
    wdt->COMMAND = 2; /* Disable Counter. */
    GET_WDT_REBOOT_CFG_REG(4) = 0xC0000000;
    wdt->CONFIG = 0x8019; /* Full System Reset after Fourth Counter expires, using TIMER(9). */
    wdt->COMMAND = 1; /* Enable Counter. */
    while (true) {
        /* Wait for reboot. */
    }
}

__attribute__((noreturn)) void pmc_reboot(uint32_t scratch0) {
    APBDEV_PMC_SCRATCH0_0 = scratch0;

    /* Reset the processor. */
    APBDEV_PMC_CONTROL = BIT(4);

    while (true) {
        /* Wait for reboot. */
    }
}

__attribute__((noreturn)) void wait_for_button_and_pmc_reboot(void) {
    uint32_t button;
    while (true) {
        button = btn_read();
        if (button & BTN_POWER) {
            /* Reboot into RCM. */
            pmc_reboot(BIT(1) | 0);
        } else if (button & (BTN_VOL_UP | BTN_VOL_DOWN)) {
            /* Reboot normally. */
            pmc_reboot(0);
        }
    }
}

__attribute__ ((noreturn)) void generic_panic(void) {
    panic(0xFF000006);
}

__attribute__((noreturn)) void fatal_error(const char *fmt, ...) {
    va_list args;
    printk("Fatal error: ");
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
    printk("\nPress POWER to reboot into RCM, VOL+/VOL- to reboot normally.\n");
    wait_for_button_and_pmc_reboot();
}

__attribute__((noinline)) bool overlaps(uint64_t as, uint64_t ae, uint64_t bs, uint64_t be)
{
    if(as <= bs && bs <= ae)
        return true;
    if(bs <= as && as <= be)
        return true;
    return false;
}
