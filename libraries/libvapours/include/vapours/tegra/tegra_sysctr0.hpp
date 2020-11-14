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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/literals.hpp>
#include <vapours/util.hpp>
#include <vapours/results.hpp>
#include <vapours/reg.hpp>

#define SYSCTR0_CNTCR    (0x000)
#define SYSCTR0_CNTCV0   (0x008)
#define SYSCTR0_CNTCV1   (0x00C)
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

#define SYSCTR0_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (SYSCTR0, NAME)
#define SYSCTR0_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (SYSCTR0, NAME, VALUE)
#define SYSCTR0_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (SYSCTR0, NAME, ENUM)
#define SYSCTR0_REG_BITS_ENUM_SEL(NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_NAMED_BITS_ENUM_SEL(SYSCTR0, NAME, __COND__, TRUE_ENUM, FALSE_ENUM)

#define DEFINE_SYSCTR0_REG(NAME, __OFFSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (SYSCTR0, NAME, __OFFSET__, __WIDTH__)
#define DEFINE_SYSCTR0_REG_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (SYSCTR0, NAME, __OFFSET__, ZERO, ONE)
#define DEFINE_SYSCTR0_REG_TWO_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (SYSCTR0, NAME, __OFFSET__, ZERO, ONE, TWO, THREE)
#define DEFINE_SYSCTR0_REG_THREE_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(SYSCTR0, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)
#define DEFINE_SYSCTR0_REG_FOUR_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (SYSCTR0, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)

DEFINE_SYSCTR0_REG_BIT_ENUM(CNTCR_EN, 0, DISABLE, ENABLE);
DEFINE_SYSCTR0_REG_BIT_ENUM(CNTCR_HDBG, 1, DISABLE, ENABLE);