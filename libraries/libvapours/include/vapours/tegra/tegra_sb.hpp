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

#define SB_CSR             (0x200)
#define SB_PFCFG           (0x208)
#define SB_AA64_RESET_LOW  (0x230)
#define SB_AA64_RESET_HIGH (0x234)


#define SB_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (SB, NAME)
#define SB_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (SB, NAME, VALUE)
#define SB_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (SB, NAME, ENUM)
#define SB_REG_BITS_ENUM_SEL(NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_NAMED_BITS_ENUM_SEL(SB, NAME, __COND__, TRUE_ENUM, FALSE_ENUM)

#define DEFINE_SB_REG(NAME, __OFFSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (SB, NAME, __OFFSET__, __WIDTH__)
#define DEFINE_SB_REG_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (SB, NAME, __OFFSET__, ZERO, ONE)
#define DEFINE_SB_REG_TWO_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (SB, NAME, __OFFSET__, ZERO, ONE, TWO, THREE)
#define DEFINE_SB_REG_THREE_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(SB, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)
#define DEFINE_SB_REG_FOUR_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (SB, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)

DEFINE_SB_REG_BIT_ENUM(CSR_SECURE_BOOT_FLAG,  0, DISABLE, ENABLE);
DEFINE_SB_REG_BIT_ENUM(CSR_NS_RST_VEC_WR_DIS, 1, ENABLE, DISABLE);
DEFINE_SB_REG_BIT_ENUM(CSR_PIROM_DISABLE,     4, ENABLE, DISABLE);
DEFINE_SB_REG_BIT_ENUM(CSR_HANG,              6, DISABLE, ENABLE);
DEFINE_SB_REG_BIT_ENUM(CSR_SWDM_ENABLE,       7, DISABLE, ENABLE);
DEFINE_SB_REG(CSR_SWDM_FAIL_COUNT,  8, 4);
DEFINE_SB_REG(CSR_COT_FAIL_COUNT,  12, 4);

DEFINE_SB_REG_BIT_ENUM(PFCFG_SPNIDEN, 0, DISABLE, ENABLE);
DEFINE_SB_REG_BIT_ENUM(PFCFG_SPIDEN,  1, DISABLE, ENABLE);
DEFINE_SB_REG_BIT_ENUM(PFCFG_NIDEN,   2, DISABLE, ENABLE);
DEFINE_SB_REG_BIT_ENUM(PFCFG_DBGEN,   3, DISABLE, ENABLE);
