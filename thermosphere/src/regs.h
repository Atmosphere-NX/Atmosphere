/*
 * Copyright (c) Atmosphère-NX
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

#ifndef __REGS_H__
#define __REGS_H__

/**
 * Access to system registers.
 */
#define WRITE_SYSREG(sysreg, val, type) \
    asm volatile ("msr        "#sysreg", %0\n" : : "r"((type)(val)))
#define READ_SYSREG(sysreg, val, type) \
    asm volatile ("mrs        %0, "#sysreg"\n" : "=r"((type)(val)))

#define READ_SYSREG_32(sysreg, val)   READ_SYSREG(sysreg, val, uint32_t)
#define WRITE_SYSREG_32(sysreg, val)  WRITE_SYSREG(sysreg, val, uint32_t)

#define READ_SYSREG_64(sysreg, val)   READ_SYSREG(sysreg, val, uint64_t)
#define WRITE_SYSREG_64(sysreg, val)  WRITE_SYSREG(sysreg, val, uint64_t)


/**
 * Returns the system's current Execution Level (EL).
 */
inline static uint32_t get_current_el(void) {
    uint32_t val;

    // Read the CurrentEl register, and extract the bits that tell us our EL.
    READ_SYSREG_32(CurrentEl, val);
    return val >> 2;
}

/**
 * Sets the base address of the EL2 exception table.
 */
inline static void set_vbar_el2(void * address) {
    WRITE_SYSREG_64(vbar_el2, (uint64_t)address);
}


/**
 * Sets the address to 'return to' when leaving EL2.
 */
inline static void set_elr_el2(void * address) {
    WRITE_SYSREG_64(elr_el2, (uint64_t)address);
}


/**
 * Returns the MMU status bit from the SCTLR register.
 */
inline static uint32_t get_el2_mmu_status(void) {
    uint32_t val;

    // Read the CurrentEl register, and extract the bits that tell us our EL.
    READ_SYSREG_32(sctlr_el2, val);
    return val & 1;
}

#endif
