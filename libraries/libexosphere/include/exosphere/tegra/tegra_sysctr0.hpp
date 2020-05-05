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
#pragma once
#include <vapours.hpp>

#define SYSCTR0_CNTFID0  (0x020)
#define SYSCTR0_CNTFID1  (0x024)


#define SYSCTR0_COUNTERID4  (0xFD0)
#define SYSCTR0_COUNTERID5  (0xFD4)
#define SYSCTR0_COUNTERID6  (0xFD8)
#define SYSCTR0_COUNTERID7  (0xFDC)
#define SYSCTR0_COUNTERID0  (0xFE0)
#define SYSCTR0_COUNTERID1  (0xFE4)
#define SYSCTR0_COUNTERID2  (0xFE8)
#define SYSCTR0_COUNTERID3  (0xFEC)
#define SYSCTR0_COUNTERID8  (0xFF0)
#define SYSCTR0_COUNTERID9  (0xFF4)
#define SYSCTR0_COUNTERID10 (0xFF8)
#define SYSCTR0_COUNTERID11 (0xFFC)

#define SYSCTR0_COUNTERID(n) SYSCTR0_COUNTERID##n