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

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "regs.h"
#include "lib/printk.h"

/**
 * Switches to EL1, and then calls main_el1.
 * Implemented in assembly in entry.S.
 */
void switch_to_el1();

/**
 * C entry point for execution at EL1.
 */
void main_el1(void * fdt);

/**
 * Reference to the EL2 vector table.
 * Note that the type here isn't reprsentative-- we just need the address of the label.
 */
extern uint64_t el2_vector_table;

/**
 * Clear out the system's bss.
 */
void _clear_bss(void)
{
    // These symbols don't actually have a meaningful type-- instead,
    // we care about the locations at which the linker /placed/ these
    // symbols, which happen to be at the start and end of the BSS.
    // We use chars here to make the math easy. :)
    extern char lds_bss_start, lds_bss_end;

    memset(&lds_bss_start, 0, &lds_bss_end - &lds_bss_start);
}


/**
 * Triggered on an unrecoverable condition; prints an error message
 * and terminates execution.
 */
void panic(const char * message)
{
    printk("\n\n");
    printk("-----------------------------------------------------------------\n");
    printk("PANIC: %s\n", message);
    printk("-----------------------------------------------------------------\n");

    // TODO: This should probably induce a reboot,
    // rather than sticking here.
    while(1);
}



/**
 * Launch an executable kernel image. Should be the last thing called by
 * Discharge, as it does not return.
 *
 * @param kernel The kernel to be executed.
 * @param fdt The device tree to be passed to the given kernel.
 */
void launch_kernel(const void *kernel)
{
    // Construct a function pointer to our kernel, which will allow us to
    // jump there immediately. Note that we don't care what this leaves on
    // the stack, as either our entire stack will be ignored, or it'll
    // be torn down by the target kernel anyways.
    void (*target_kernel)(void) = kernel;

    printk("Launching Horizon kernel...\n");
    target_kernel();
}



/**
 * Core section of the stub-- sets up the hypervisor from up in EL2.
 */
int main(int argc, void **argv)
{
    // Read the currrent execution level...
    uint32_t el = get_current_el();

    /* Say hello. */
    printk("Welcome to Atmosph\xe8re Thermosph\xe8" "re!\n");
    printk("Running at EL%d.\n", el);

    // ... and ensure we're in EL2.
    if (el != 2) {
        panic("Thermosph\xe8" "re must be launched from EL2!");
    }

    // Set up the vector table for EL2, so that the HVC instruction can be used
    // from EL1. This allows us to return to EL2 after starting the EL1 guest.
    set_vbar_el2(&el2_vector_table);

    // TODO:
    // Insert any setup you want done in EL2, here. For now, EL2 is set up
    // to do almost nothing-- it doesn't take control of any hardware,
    // and it hasn't set up any trap-to-hypervisor features.
    printk("\nSwitching to EL1...\n");
    switch_to_el1();
}


/**
 * Secondary section of the stub, executed once we've surrendered
 * hypervisor privileges.
 */
void main_el1(void * fdt)
{
    int rc;
    (void)rc;

    // Read the currrent execution level...
    uint32_t el = get_current_el();

    // Validate that we're in EL1.
    printk("Now executing from EL%d!\n", el);
    if(el != 1) {
        panic("Executing with more privilege than we expect!");
    }

    // If we've made it here, we failed to boot, and we can't recover.
    panic("We should launch Horizon here!");
}
