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
#include <stdbool.h>

#include "utils.h"
#include "interrupt.h"

/* Global of registered handlers. */
static struct {
    unsigned int id;
    void (*handler)(void);
} g_registered_interrupts[MAX_REGISTERED_INTERRUPTS] = { {0} };

static unsigned int get_interrupt_id(void) {
    return GICC_IAR;
}

/* Initializes the GIC. This must be called during wakeup. */
void intr_initialize_gic(void) {
    /* Setup interrupts 0-0x1F as nonsecure with highest non-secure priority. */
    GICD_IGROUPR[0] = 0xFFFFFFFF;
    for (unsigned int i = 0; i < 0x20; i++) {
        GICD_IPRIORITYR[i] = GIC_PRI_HIGHEST_NONSECURE;
    }
    
    /* Setup the GICC. */
    GICC_CTLR = 0x1D9;
    GICC_PMR = GIC_PRI_HIGHEST_NONSECURE;
    GICC_BPR = 7;
}

/* Initializes Interrupt Groups 1-7 in the GIC. Called by pk2ldr. */
void intr_initialize_gic_nonsecure(void) {
    for (unsigned int i = 1; i < 8; i++) {
        GICD_IGROUPR[i] = 0xFFFFFFFF;
    }
    
    for (unsigned int i = 0x20; i < 0xE0; i++) {
        GICD_IPRIORITYR[i] = GIC_PRI_HIGHEST_NONSECURE;
    }
    GICD_CTLR = 1;
}

/* Sets GICC_CTLR to appropriate pre-sleep value. */
void intr_prepare_gicc_for_sleep(void) {
    GICC_CTLR = 0x1E0;
}

/* Sets an interrupt's group in the GICD. */
void intr_set_group(unsigned int id, int group) {
    GICD_IGROUPR[id >> 5] = (GICD_IGROUPR[id >> 5] & (~(1 << (id & 0x1F)))) | ((group & 1) << (id & 0x1F));
}

/* Sets an interrupt id as pending in the GICD. */
void intr_set_pending(unsigned int id) {
    GICD_ISPENDR[id >> 5] = 1 << (id & 0x1F);
}

/* Sets an interrupt's priority in the GICD. */
void intr_set_priority(unsigned int id, uint8_t priority) {
    GICD_IPRIORITYR[id] = priority;
}

/* Sets an interrupt's target CPU mask in the GICD. */
void intr_set_cpu_mask(unsigned int id, uint8_t mask) {
    GICD_ITARGETSR[id] = mask;
}

/* Sets an interrupt's edge/level bits in the GICD. */
void intr_set_edge_level(unsigned int id, int edge_level) {
    GICD_ICFGR[id >> 4] = GICD_ICFGR[id >> 4] & ((~(3 << ((id & 0xF) << 1))) | (((edge_level & 1) << 1) << ((id & 0xF) << 1)));
}

/* Sets an interrupt's enabled status in the GICD. */
void intr_set_enabled(unsigned int id, int enabled) {
    GICD_ISENABLER[id >> 5] = (enabled & 1) << (id & 0x1F);
}

/* To be called by FIQ handler. */
void handle_registered_interrupt(void) {
    unsigned int interrupt_id = get_interrupt_id();
    if (interrupt_id <= 0xDF) {
        bool found_handler = false;
        for (unsigned int i = 0; i < MAX_REGISTERED_INTERRUPTS; i++) {
            if (g_registered_interrupts[i].id == interrupt_id) {
                found_handler = true;
                g_registered_interrupts[i].handler();
                /* Mark that interrupt is done. */
                GICC_EOIR = interrupt_id;
                break;
            }
        }
        /* We must have found a handler, or something went wrong. */
        if (!found_handler) {
            generic_panic();
        }
    }
}

/* Registers an interrupt into the global. */
void intr_register_handler(unsigned int id, void (*handler)(void)) {
    bool registered_handler = false; 
    for (unsigned int i = 0; i < MAX_REGISTERED_INTERRUPTS; i++) {
        if (g_registered_interrupts[i].id == 0) {
            g_registered_interrupts[i].handler = handler;
            g_registered_interrupts[i].id = id;
            registered_handler = true;
            break;
        }
    }
    /* Failure to register is an error condition. */
    if (!registered_handler) {
        generic_panic();
    }
}

