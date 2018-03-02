#include <stdint.h>

#include "utils.h"
#include "actmon.h"

static void (*g_actmon_callback)(void) = NULL;

void actmon_set_callback(void (*callback)(void)) {
    g_actmon_callback = callback;
}

void actmon_on_bpmp_wakeup(void) {
    /* This gets set as the actmon interrupt handler on 4.x. */
    panic(0xF0A00036);
}

void actmon_interrupt_handler(void) {
    ACTMON_COP_CTRL_0 = 0;
    ACTMON_COP_INTR_STATUS_0 = ACTMON_COP_INTR_STATUS_0;
    if (g_actmon_callback != NULL) {
        g_actmon_callback();
        g_actmon_callback = NULL;
    }
}