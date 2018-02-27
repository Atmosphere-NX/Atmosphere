#ifndef EXOSPHERE_CPU_CTX_H
#define EXOSPHERE_CPU_CTX_H

#include <stdint.h>

/* Exosphere CPU Management functionality. */

typedef struct {
  uint64_t argument;
  uint64_t ELR_EL3;
  int is_active;
  int is_saved;
  uint32_t OSDTRRX_EL1;
  uint32_t OSDTRTX_EL1;
  uint32_t MDSCR_EL1;
  uint32_t OSECCR_EL1;
  uint32_t MDCCINT_EL1;
  uint32_t DBGCLAIMCLR_EL1;
  uint32_t DBGVCR32_EL2;
  uint32_t SDER32_EL3;
  uint32_t MDCR_EL2;
  uint32_t MDCR_EL3;
  uint64_t DBGBVR0_EL1;
  uint64_t DBGBCR0_EL1;
  uint64_t DBGBVR1_EL1;
  uint64_t DBGBCR1_EL1;
  uint64_t DBGBVR2_EL1;
  uint64_t DBGBCR2_EL1;
  uint64_t DBGBVR3_EL1;
  uint64_t DBGBCR3_EL1;
  uint64_t DBGBVR4_EL1;
  uint64_t DBGBCR4_EL1;
  uint64_t DBGBVR5_EL1;
  uint64_t DBGBCR5_EL1;
  uint64_t DBGWVR0_EL1;
  uint64_t DBGWCR0_EL1;
  uint64_t DBGWVR1_EL1;
  uint64_t DBGWCR1_EL1;
  uint64_t DBGWVR2_EL1;
  uint64_t DBGWCR2_EL1;
  uint64_t DBGWVR3_EL1;
  uint64_t DBGWCR3_EL1;
} saved_cpu_context_t;

#define NUM_CPU_CORES 4

void save_current_core_context(void);
void restore_current_core_context(void);

void set_core_entrypoint_and_argument(uint32_t core, uint64_t entrypoint_addr, uint64_t argument);

uint32_t cpu_on(uint32_t core, uint64_t entrypoint_addr, uint64_t argument);
uint32_t cpu_off(void); /* TODO */


#endif
