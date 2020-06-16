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

.section    .text._ZN3ams8warmboot8SpinLoopEj, "ax", %progbits
.global     _ZN3ams8warmboot8SpinLoopEj
.thumb_func
.syntax unified
_ZN3ams8warmboot8SpinLoopEj:
    1:
        /* Subtract one from the count. */
        subs r0, r0, #1

        /* If we aren't at zero, continue looping. */
        bgt  1b

        /* Return. */
        bx lr