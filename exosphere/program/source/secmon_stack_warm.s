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

.section    .warmboot.data._ZN3ams6secmon23CommonWarmbootStackLockE, "aw", %progbits
.global     _ZN3ams6secmon23CommonWarmbootStackLockE
_ZN3ams6secmon23CommonWarmbootStackLockE:
    /* Define storage for the global common warmboot stack bakery lock. */
    .word 0
