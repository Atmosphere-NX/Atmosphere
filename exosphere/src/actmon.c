/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
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