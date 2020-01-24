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

/* For some reason GAS doesn't know about it, even with .cpu cortex-a57 */
#define cpuactlr_el1 s3_1_c15_c2_0
#define cpuectlr_el1 s3_1_c15_c2_1

.section    .crt0.text.start, "ax", %progbits
.global     _start
_start:
    /* TODO */
    b _start
    .word (__metadata_begin - _start)

__metadata_begin:
    .ascii "MSS0"                     /* Magic */
    .word  0                          /* KInitArguments */
    .word  0                          /* INI1 base address. */
    .word  0                          /* Kernel Loader base address. */
__metadata_kernel_layout:
    .word _start - _start             /* rx_offset */
    .word __rodata_start     - _start /* rx_end_offset */
    .word __rodata_start     - _start /* ro_offset */
    .word __data_start       - _start /* ro_end_offset */
    .word __data_start       - _start /* rw_offset */
    .word __bss_start__      - _start /* rw_end_offset */
    .word __bss_start__      - _start /* bss_offset */
    .word __bss_end__        - _start /* bss_end_offset */
    .word __end__            - _start /* ini_load_offset */
    .word _DYNAMIC           - _start /* dynamic_offset */
    .word __init_array_start - _start /* init_array_offset */
    .word __init_array_end   - _start /* init_array_end_offset */
.if (. - __metadata_begin) != 0x40
    .error "Incorrect Mesosphere Metadata"
.endif