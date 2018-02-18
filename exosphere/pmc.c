#include "pmc.h"

volatile void *g_pmc_registers = NULL;

void set_pmc_address(void *pmc_base) {
    g_pmc_registers = pmc_base;
}

inline void *get_pmc_address(void) {
    return g_pmc_registers;
}