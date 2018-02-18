#ifndef EXOSPHERE_PMC_H
#define EXOSPHERE_PMC_H

#include <stdint.h>

/* Exosphere register definitions for the Tegra X1 PMC. */

void set_pmc_address(void *pmc_base);
void *get_pmc_address(void); /* This is inlined in pmc.c */

#define APBDEV_PMC_PWRGATE_TOGGLE_0 (*((volatile uint32_t *)(get_pmc_address() + 0x430)))
#define APBDEV_PMC_PWRGATE_STATUS_0 (*((volatile uint32_t *)(get_pmc_address() + 0x438)))

#endif