#ifndef EXOSPHERE_INTERRUPT_H
#define EXOSPHERE_INTERRUPT_H

#include <stdint.h>
#include "mmu.h"

/* Exosphere driver for the Tegra X1 GIC-400 registers. */


#define MAX_REGISTERED_INTERRUPTS 4
#define INTERRUPT_ID_SECURITY_ENGINE 0x5A

#define GICD_BASE  (mmio_get_device_address(MMIO_DEVID_GICD))
#define GICC_BASE  (mmio_get_device_address(MMIO_DEVID_GICC))

#define GICD_IGROUPR    ((volatile uint32_t *)(GICD_BASE + 0x080ULL))
#define GICD_ISENABLER  ((volatile uint32_t *)(GICD_BASE + 0x100ULL))
#define GICD_ISPENDR    ((volatile uint32_t *)(GICD_BASE + 0x200ULL))
#define GICD_IPRIORITYR ((volatile uint8_t *)(GICD_BASE + 0x400ULL))
#define GICD_ITARGETSR  ((volatile uint8_t *)(GICD_BASE + 0x800ULL))
#define GICD_ICFGR      ((volatile uint32_t *)(GICD_BASE + 0xC00ULL))

#define GICC_CTLR       (*((volatile uint32_t *)(GICC_BASE + 0x0000ULL)))
#define GICC_PMR        (*((volatile uint32_t *)(GICC_BASE + 0x0004ULL)))
#define GICC_BPR        (*((volatile uint32_t *)(GICC_BASE + 0x0008ULL)))
#define GICC_IAR        (*((volatile uint32_t *)(GICC_BASE + 0x000CULL)))
#define GICC_EOIR       (*((volatile uint32_t *)(GICC_BASE + 0x0010ULL)))

#define GIC_PRI_HIGHEST_SECURE 0x00
#define GIC_PRI_HIGHEST_NONSECURE 0x80

#define GIC_GROUP_SECURE 0
#define GIC_GROUP_NONSECURE 1

/* To be called by FIQ handler. */
void handle_registered_interrupt(void);

/* Initializes the GIC. TODO: This must be called during wakeup. */
void intr_initialize_gic(void);


void intr_register_handler(unsigned int id, void (*handler)(void));
void intr_set_group(unsigned int id, int group);
void intr_set_pending(unsigned int id);
void intr_set_priority(unsigned int id, uint8_t priority);
void intr_set_cpu_mask(unsigned int id, uint8_t mask);
void intr_set_edge_level(unsigned int id, int edge_level);
void intr_set_enabled(unsigned int id, int enabled);
#endif