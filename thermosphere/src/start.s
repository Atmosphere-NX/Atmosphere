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

.section ".text"
.global _start


/**
 * Simple macro to help with generating vector table entries.
 */ 
.macro  ventry  label
    .align  7
    b       \label
.endm


/**
 * Start of day code. This is the first code that executes after we're launched
 * by the bootloader. We use this only to set up a C environment.
 */
_start:

        // Create a simple stack for the hypervisor, while executing in EL2.
        ldr     x1, =el2_stack_end
        mov     sp, x1

        // Clear out our binary's bss.
        stp     x0, x1, [sp, #-16]!
        bl      _clear_bss
        ldp     x0, x1, [sp], #16

        // Run the main routine. This shouldn't return.
        b       main

        // We shouldn't ever reach here; trap.
1:      b       1b


/*
 * Vector table for interrupts/exceptions that reach EL2.
 */
.align  11
.global el2_vector_table;
el2_vector_table:
        ventry _unhandled_vector                // Synchronous EL2t
        ventry _unhandled_vector                // IRQ EL2t
        ventry _unhandled_vector                // FIQ EL2t
        ventry _unhandled_vector                // Error EL2t

        ventry _unhandled_vector                // Synchronous EL2h
        ventry _unhandled_vector                // IRQ EL2h
        ventry _unhandled_vector                // FIQ EL2h
        ventry _unhandled_vector                // Error EL2h

        ventry _handle_hypercall                // Synchronous 64-bit EL0/EL1
        ventry _unhandled_vector                // IRQ 64-bit EL0/EL1
        ventry _unhandled_vector                // FIQ 64-bit EL0/EL1
        ventry _unhandled_vector                // Error 64-bit EL0/EL1

        ventry _unhandled_vector                // Synchronous 32-bit EL0/EL1
        ventry _unhandled_vector                // IRQ 32-bit EL0/EL1
        ventry _unhandled_vector                // FIQ 32-bit EL0/EL1
        ventry _unhandled_vector                // Error 32-bit EL0/EL1



/*
 * Switch down to EL1 and then execute the second half of our stub.
 * Implemented in assembly, as this manipulates the stack.
 *
 * Obliterates the stack, but leaves the rest of memory intact. This should be
 *  fine, as we should be hiding the EL2 memory from the rest of the system.
 *
 * x0: The location of the device tree to be passed into EL0.
 */
.global switch_to_el1
switch_to_el1:

        // Set up a post-EL1-switch return address...
        ldr     x2, =_post_el1_switch
        msr     elr_el2, x2

        // .. and set up the CPSR after we switch to EL1.
        // We overwrite the saved program status register. Note that setting
        // this with the EL = EL1 is what actually causes the switch.
        mov     x2, #0x3c5     // EL1_SP1 | D | A | I | F
        msr     spsr_el2, x2

        // Reset the stack pointer to the very end of the stack, so it's
        // fresh and clean for when we jump back up into EL2.
        ldr     x2, =el2_stack_end
        mov     sp, x2

        // ... and switch down to EL1. (This essentially asks the processor
        // to switch down to EL1 and then load ELR_EL2 to the PC.)
        eret


/*
 * Entry point after the switch to EL1.
 */
.global _post_el1_switch
_post_el1_switch:

        // Create a simple stack for us to use while at EL1.
        // We use this temporarily to launch Horizon.
        ldr     x2, =el1_stack_end
        mov     sp, x2

        // Run the main routine. This shouldn't return.
        b       main_el1

        // We shouldn't ever reach here; trap.
1:      b       1b


/**
 * Push and pop 'psuedo-op' macros that simplify the ARM syntax to make the below pretty.
 */
.macro  push, xreg1, xreg2
    stp     \xreg1, \xreg2, [sp, #-16]!
.endm
.macro  pop, xreg1, xreg2
    ldp     \xreg1, \xreg2, [sp], #16
.endm

/**
 * Macro that saves registers onto the stack when entering an exception handler--
 * effectively saving the guest state. Once this method is complete, *sp will
 * point to a struct guest_state.
 *
 * You can modify this to save whatever you'd like, but:
 *   1) We can only push in pairs due to armv8 architecture quirks.
 *   2) Be careful not to trounce registers until after you've saved them.
 *   3) r31 is your stack pointer, and doesn't need to be saved. You'll want to
 *      save the lesser EL's stack pointers separately.
 *   4) Make sure any changes you make are reflected both in _restore_registers_
 *      and in struct guest_state, or things will break pretty badly.
 */
.macro  save_registers
        // General purpose registers x1 - x30
        push    x29, x30
        push    x27, x28
        push    x25, x26
        push    x23, x24
        push    x21, x22
        push    x19, x20
        push    x17, x18
        push    x15, x16
        push    x13, x14
        push    x11, x12
        push    x9,  x10
        push    x7,  x8
        push    x5,  x6
        push    x3,  x4
        push    x1,  x2

        // x0 and the el2_esr
        mrs     x20, esr_el2
        push    x20, x0

        // the el1_sp and el0_sp
        mrs     x0, sp_el0
        mrs     x1, sp_el1
        push    x0, x1

        // the el1 elr/spsr
        mrs     x0, elr_el1
        mrs     x1, spsr_el1
        push    x0, x1

        // the el2 elr/spsr
        mrs     x0, elr_el2
        mrs     x1, spsr_el2
        push    x0, x1

.endm

/**
 * Macro that restores registers when returning from EL2.
 * Mirrors save_registers.
 */
.macro restore_registers
        // the el2 elr/spsr
        pop     x0, x1
        msr     elr_el2, x0
        msr     spsr_el2, x1

        // the el1 elr/spsr
        pop     x0, x1
        msr     elr_el1, x0
        msr     spsr_el1, x1

        // the el1_sp and el0_sp
        pop     x0, x1
        msr     sp_el0, x0
        msr     sp_el1, x1

        // x0, and the el2_esr
        // Note that we don't restore el2_esr, as this wouldn't
        // have any meaning.
        pop    x20, x0

        // General purpose registers x1 - x30
        pop    x1,  x2
        pop    x3,  x4
        pop    x5,  x6
        pop    x7,  x8
        pop    x9,  x10
        pop    x11, x12
        pop    x13, x14
        pop    x15, x16
        pop    x17, x18
        pop    x19, x20
        pop    x21, x22
        pop    x23, x24
        pop    x25, x26
        pop    x27, x28
        pop    x29, x30
.endm

/*
 * Handler for any vector we're not equipped to handle.
 */
_unhandled_vector:
        // TODO: Save interrupt state and turn off interrupts.
        save_registers

        // Point x0 at our saved registers, and then call our C handler.
        mov     x0, sp
        bl    unhandled_vector

        restore_registers
        eret


/*
 * Handler for any synchronous event coming from the guest (any trap-to-EL2).
 * This _stub_ only uses this to handle hypercalls-- hence the name.
 */
_handle_hypercall:
        // TODO: Save interrupt state and turn off interrupts.
        save_registers

        // Point x0 at our saved registers, and then call our C handler.
        mov     x0, sp
        bl    handle_hypercall

        restore_registers
        eret
