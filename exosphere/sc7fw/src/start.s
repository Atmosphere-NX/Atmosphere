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
 
.section .text.start
.align 4
.global _start
_start:
    b crt0
.global _reboot
    b reboot

.global crt0
.type crt0, %function
crt0:
    @ setup to call sc7_entry_main
    msr cpsr_cxsf, #0xD3
    ldr sp, =__stack_top__
    ldr lr, =reboot
    b sc7_entry_main


.global spinlock_wait
.type spinlock_wait, %function
spinlock_wait:
    subs r0, r0, #1
    bgt spinlock_wait
    bx lr
