#include <stdint.h>
#include <stdbool.h>

#include "utils.h"
#include "timers.h"

void wait(uint32_t microseconds) {
    uint32_t old_time = TIMERUS_CNTR_1US_0;
    while (TIMERUS_CNTR_1US_0 - old_time <= microseconds) {
        /* Spin-lock. */
    }
}

__attribute__ ((noreturn)) void watchdog_reboot(void) {
    unsigned int current_core = get_core_id();
    volatile watchdog_timers_t *wdt = GET_WDT(current_core);
    wdt->PATTERN = WDT_REBOOT_PATTERN;
    wdt->COMMAND = 2; /* Disable Counter. */
    GET_WDT_REBOOT_CFG_REG(current_core) = 0xC0000000;
    wdt->CONFIG = 0x8015 + current_core; /* Full System Reset after Fourth Counter expires, using TIMER(5+core_id). */
    wdt->COMMAND = 1; /* Enable Counter. */
    while (true) {
        /* Wait for reboot. */
    }
}