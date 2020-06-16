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

.section    .text._ZN3ams6secmon5StartEv, "ax", %progbits
.align      4
.global     _ZN3ams6secmon5StartEv
_ZN3ams6secmon5StartEv:
    /* Set SPSEL 1 stack pointer to the core 0 exception stack address. */
    msr  spsel, #1
    ldr  x20, =0x1F01F6F00
    mov  sp, x20

    /* Set SPSEL 0 stack pointer to a temporary location in volatile memory. */
    msr  spsel, #0
    ldr  x20, =0x1F01C0800
    mov  sp, x20

    /* Setup X18 to point to the global context. */
    ldr  x18, =0x1F01FA000

    /* Invoke main. */
    bl _ZN3ams6secmon4MainEv

    /* Clear boot code high. */
    bl _ZN3ams6secmon17ClearBootCodeHighEv

    /* Set the stack pointer to the core 3 exception stack address. */
    ldr  x20, =0x1F01F9000
    mov  sp, x20

    /* Unmap the boot code region (and clear the low part). */
    bl _ZN3ams6secmon13UnmapBootCodeEv

    /* Initialize the random cache. */
    /* NOTE: Nintendo does this much earlier, but we reuse volatile space. */
    bl _ZN3ams6secmon3smc15FillRandomCacheEv

    /* Jump to lower exception level. */
    b _ZN3ams6secmon25JumpToLowerExceptionLevelEv

.section    .text._ZN3ams6secmon20StartWarmbootVirtualEv, "ax", %progbits
.align      4
.global     _ZN3ams6secmon20StartWarmbootVirtualEv
_ZN3ams6secmon20StartWarmbootVirtualEv:
    /* Set the stack pointer to the shared warmboot stack address. */
    ldr x20, =0x1F01F67C0
    mov sp, x20

    /* Setup X18 to point to the global context. */
    ldr  x18, =0x1F01FA000

    /* Perform final warmboot setup. */
    bl _ZN3ams6secmon24SetupSocSecurityWarmbootEv

    /* Jump to lower exception level. */
    b _ZN3ams6secmon25JumpToLowerExceptionLevelEv

.section    .text._ZN3ams6secmon25JumpToLowerExceptionLevelEv, "ax", %progbits
.align      4
.global     _ZN3ams6secmon25JumpToLowerExceptionLevelEv
_ZN3ams6secmon25JumpToLowerExceptionLevelEv:
    /* Get the EntryContext. */
    sub sp, sp, #0x10
    mov x0, sp
    bl _ZN3ams6secmon15GetEntryContextEPNS0_12EntryContextE

    /* Load the entrypoint and argument from the context. */
    ldr x19, [sp, #0x00]
    ldr x0,  [sp, #0x08]

    /* Set the exception return address. */
    msr elr_el3, x0

    /* Get the core exception stack. */
    bl _ZN3ams6secmon28GetCoreExceptionStackVirtualEv
    mov sp, x0

    /* Release our exclusive access to the common warmboot stack. */
    bl _ZN3ams6secmon26ReleaseCommonWarmbootStackEv

    /* Configure SPSR_EL3. */
    mov x0, #0x3C5
    msr spsr_el3, x0

    /* Set x0 to the entry argument. */
    mov x0, x19

    /* Ensure instruction reordering doesn't happen around this point. */
    isb

    /* Return to lower level. */
    eret

    /* Infinite loop, though we should never get here. */
    1: b 1b

.section    .text._ZN3ams6secmon28GetCoreExceptionStackVirtualEv, "ax", %progbits
.align      4
.global     _ZN3ams6secmon28GetCoreExceptionStackVirtualEv
_ZN3ams6secmon28GetCoreExceptionStackVirtualEv:
    /* Get the current core id. */
    mrs x0, mpidr_el1
    and x0, x0, #3

    /* Jump to the appropriate core's stack handler. */
    cmp x0, #3
    b.eq 3f

    cmp x0, #2
    b.eq 2f

    cmp x0, #1
    b.eq 1f

    /* cmp x0, #0 */
    /* b.eq 0f    */

    0:
        ldr x0, =0x1F01F6F00
        ret
    1:
        ldr x0, =0x1F01F6F80
        ret
    2:
        ldr x0, =0x1F01F7000
        ret
    3:
        ldr x0, =0x1F01F9000
        ret

.section    .text._ZN3ams6secmon25AcquireCommonSmcStackLockEv, "ax", %progbits
.align      4
.global     _ZN3ams6secmon25AcquireCommonSmcStackLockEv
_ZN3ams6secmon25AcquireCommonSmcStackLockEv:
    /* Get the address of the lock. */
    ldr x0, =_ZN3ams6secmon18CommonSmcStackLockE

    /* Take the lock. */
    b _ZN3ams6secmon15AcquireSpinLockERj

.section    .text._ZN3ams6secmon25ReleaseCommonSmcStackLockEv, "ax", %progbits
.align      4
.global     _ZN3ams6secmon25ReleaseCommonSmcStackLockEv
_ZN3ams6secmon25ReleaseCommonSmcStackLockEv:
    /* Get the address of the lock. */
    ldr x0, =_ZN3ams6secmon18CommonSmcStackLockE

    /* Release the lock. */
    b _ZN3ams6secmon15ReleaseSpinLockERj

.section    .text._ZN3ams6secmon26ReleaseCommonWarmbootStackEv, "ax", %progbits
.align      4
.global     _ZN3ams6secmon26ReleaseCommonWarmbootStackEv
_ZN3ams6secmon26ReleaseCommonWarmbootStackEv:
    /* Get the virtual address of the lock. */
    ldr x0, =_ZN3ams6secmon23CommonWarmbootStackLockE
    ldr x1, =(0x1F00C0000 - 0x07C012000)
    add x1, x1, x0

    /* Get the bakery value for our core. */
    mrs  x0, mpidr_el1
    and  x0, x0, #3
    ldrb w2, [x1, x0]

    /* Clear our ticket number. */
    and  w2, w2, #(~0x7F)
    strb w2, [x1, x0]

    /* Flush the cache. */
    dc civac, x1

    /* Synchronize data for all cores. */
    dsb sy

    /* Send an event. */
    sev

    /* Return. */
    ret

.section    .text._ZN3ams6secmon19PivotStackAndInvokeEPvPFvvE, "ax", %progbits
.align      4
.global     _ZN3ams6secmon19PivotStackAndInvokeEPvPFvvE
_ZN3ams6secmon19PivotStackAndInvokeEPvPFvvE:
    /* Pivot to use the provided stack pointer. */
    mov  sp, x0

    /* Release our lock on the common smc stack. */
    mov  x19, x1
    bl   _ZN3ams6secmon25ReleaseCommonSmcStackLockEv

    /* Invoke the function with the new stack. */
    br   x19

.section    .data._ZN3ams6secmon18CommonSmcStackLockE, "aw", %progbits
.global     _ZN3ams6secmon18CommonSmcStackLockE
_ZN3ams6secmon18CommonSmcStackLockE:
    /* Define storage for the global common smc stack spinlock. */
    .word 0
