/* Copyright (C) 1995-2018 Free Software Foundation, Inc.
This file is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.
This file is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.
Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.
You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */


.section    .text.__gnu_thumb1_case_uqi, "ax", %progbits
.globl __gnu_thumb1_case_uqi
.align      0
.thumb_func
.syntax unified
__gnu_thumb1_case_uqi:
    push    {r1}
    mov     r1, lr
    lsrs    r1, r1, #1
    lsls    r1, r1, #1
    ldrb    r1, [r1, r0]
    lsls    r1, r1, #1
    add     lr, lr, r1
    pop     {r1}
    bx      lr
.type __gnu_thumb1_case_uqi, %function
.size __gnu_thumb1_case_uqi, .-__gnu_thumb1_case_uqi

.section    .text.__gnu_thumb1_case_uhi, "ax", %progbits
.globl __gnu_thumb1_case_uhi
.align      0
.thumb_func
.syntax unified
__gnu_thumb1_case_uhi:
    push    {r0, r1}
    mov     r1, lr
    lsrs    r1, r1, #1
    lsls    r0, r0, #1
    lsls    r1, r1, #1
    ldrh    r1, [r1, r0]
    lsls    r1, r1, #1
    add     lr, lr, r1
    pop     {r0, r1}
    bx  lr
.type __gnu_thumb1_case_uhi, %function
.size __gnu_thumb1_case_uhi, .-__gnu_thumb1_case_uhi
