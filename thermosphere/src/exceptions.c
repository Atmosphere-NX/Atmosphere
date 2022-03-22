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
#include "exceptions.h"

#include "lib/printk.h"

/**
 * Simple debug function that prints all of our saved registers.
 */
static void print_registers(struct guest_state *regs)
{
    // print x0-29
    for(int i = 0; i < 30; i += 2) {
        printk("x%d:\t0x%p\t", i,     regs->x[i]);
        printk("x%d:\t0x%p\n", i + 1, regs->x[i + 1]);
    }

    // print x30; don't bother with x31 (SP), as it's used by the stack that's
    // storing this stuff; we really care about the saved SP anyways
    printk("x30:\t0x%p\n", regs->x[30]);

    // Special registers.
    printk("pc:\t0x%p\tcpsr:\t0x%p\n", regs->pc, regs->cpsr);
    printk("sp_el1:\t0x%p\tsp_el0:\t0x%p\n", regs->sp_el1, regs->sp_el0);
    printk("elr_el1:0x%p\tspsr_el1:0x%p\n", regs->elr_el1, regs->spsr_el1);

    // Note that we don't print ESR_EL2, as this isn't really part of the saved state.
}

/**
 * Placeholder function that triggers whenever a vector happens we're not
 * expecting. Currently prints out some debug information.
 */
void unhandled_vector(struct guest_state *regs)
{
    printk("\nAn unexpected vector happened!\n");
    print_registers(regs);
    printk("\n\n");
}


/**
 * Handles an HVC call.
 */
static void handle_hvc(struct guest_state *regs, int call_number)
{

    switch(call_number) {


    default:
        printk("Got a HVC call from 64-bit code.\n");
        printk("Calling instruction was: hvc %d\n\n", call_number);
        printk("Calling context (you can use these regs as hypercall args!):\n");
        print_registers(regs);
        printk("\n\n");
        break;
    }
}


/**
 * Placeholder function that triggers whenever a user event triggers a
 * synchronous interrupt. Currently, we really only care about 'hvc',
 * so that's all we're going to handle here.
 */

void handle_hypercall(struct guest_state *regs)
{
    // This is demonstration code.
    // In the future, you'd stick your hypercall table here.

    switch (regs->esr_el2.ec) {

    case HSR_EC_HVC64: {
        // Read the hypercall number.
        int hvc_nr = regs->esr_el2.iss & 0xFFFF;

        // ... and handle the hypercall.
        handle_hvc(regs, hvc_nr);
        break;
    }
    default:
        printk("Unexpected hypercall! ESR=%p\n", regs->esr_el2.bits);
        print_registers(regs);
        printk("\n\n");
        break;

    }
}


