/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
 
#ifndef EXOSPHERE_INTERRUPT_H
#define EXOSPHERE_INTERRUPT_H

#include <stdint.h>
#include "memory_map.h"

/* Exosphere driver for the Tegra X1 GIC-400 registers. */


#define MAX_REGISTERED_INTERRUPTS 4
#define INTERRUPT_ID_SECURITY_ENGINE 0x5A
#define INTERRUPT_ID_ACTIVITY_MONITOR_4X 0x4D
#define INTERRUPT_ID_1C 0x1C
#define INTERRUPT_ID_USER_SECURITY_ENGINE 0x2C

static inline uintptr_t get_gicd_base(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_GICD);
}

static inline uintptr_t get_gicc_base(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_GICC);
}

#define GICD_BASE  (get_gicd_base())
#define GICC_BASE  (get_gicc_base())

#define GICD_CTLR       MAKE_REG32(GICD_BASE + 0x000ull)
#define GICD_IGROUPR    ((volatile uint32_t *)(GICD_BASE + 0x080ull))
#define GICD_ISENABLER  ((volatile uint32_t *)(GICD_BASE + 0x100ull))
#define GICD_ISPENDR    ((volatile uint32_t *)(GICD_BASE + 0x200ull))
#define GICD_IPRIORITYR ((volatile uint8_t *)(GICD_BASE + 0x400ull))
#define GICD_ITARGETSR  ((volatile uint8_t *)(GICD_BASE + 0x800ull))
#define GICD_ICFGR      ((volatile uint32_t *)(GICD_BASE + 0xC00ull))

#define GICC_CTLR       MAKE_REG32(GICC_BASE + 0x0000ull)
#define GICC_PMR        MAKE_REG32(GICC_BASE + 0x0004ull)
#define GICC_BPR        MAKE_REG32(GICC_BASE + 0x0008ull)
#define GICC_IAR        MAKE_REG32(GICC_BASE + 0x000CULL)
#define GICC_EOIR       MAKE_REG32(GICC_BASE + 0x0010ull)

#define GIC_PRI_HIGHEST_SECURE 0x00
#define GIC_PRI_HIGHEST_NONSECURE 0x80

#define GIC_GROUP_SECURE 0
#define GIC_GROUP_NONSECURE 1

/* To be called by FIQ handler. */
void handle_registered_interrupt(void);

/* Initializes the GIC. This must be called during wakeup. */
void intr_initialize_gic(void);
void intr_initialize_gic_nonsecure(void);

void intr_prepare_gicc_for_sleep(void);


void intr_register_handler(unsigned int id, void (*handler)(void));
void intr_set_group(unsigned int id, int group);
void intr_set_pending(unsigned int id);
void intr_set_priority(unsigned int id, uint8_t priority);
void intr_set_cpu_mask(unsigned int id, uint8_t mask);
void intr_set_edge_level(unsigned int id, int edge_level);
void intr_set_enabled(unsigned int id, int enabled);
#endif
