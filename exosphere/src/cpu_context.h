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
 
#ifndef EXOSPHERE_CPU_CTX_H
#define EXOSPHERE_CPU_CTX_H

#include "utils.h"
#include "synchronization.h"

/* Exosphere CPU Management functionality. */

extern critical_section_t g_boot_critical_section;

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
  uint32_t SPSR_EL3; /* not in official code */
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

bool is_core_active(uint32_t core);
void set_core_is_active(uint32_t core, bool is_active);
void set_current_core_active(void);
void set_current_core_inactive(void);

void use_core_entrypoint_and_argument(uint32_t core, uintptr_t *entrypoint_addr, uint64_t *argument);
void set_core_entrypoint_and_argument(uint32_t core, uintptr_t entrypoint_addr, uint64_t argument);
void __attribute__((noreturn)) core_jump_to_lower_el(void);

uint32_t cpu_on(uint32_t core, uintptr_t entrypoint_addr, uint64_t argument);
uint32_t cpu_off(void);


#endif
