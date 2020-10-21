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

#define MSELECT(x) (0x50060000 + x)

#define MSELECT_CONFIG   (0x000)

#define MSELECT_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (MSELECT, NAME)
#define MSELECT_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (MSELECT, NAME, VALUE)
#define MSELECT_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (MSELECT, NAME, ENUM)
#define MSELECT_REG_BITS_ENUM_SEL(NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_NAMED_BITS_ENUM_SEL(MSELECT, NAME, __COND__, TRUE_ENUM, FALSE_ENUM)

#define DEFINE_MSELECT_REG(NAME, __OFFSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (MSELECT, NAME, __OFFSET__, __WIDTH__)
#define DEFINE_MSELECT_REG_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (MSELECT, NAME, __OFFSET__, ZERO, ONE)
#define DEFINE_MSELECT_REG_TWO_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (MSELECT, NAME, __OFFSET__, ZERO, ONE, TWO, THREE)
#define DEFINE_MSELECT_REG_THREE_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(MSELECT, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)
#define DEFINE_MSELECT_REG_FOUR_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (MSELECT, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)

DEFINE_MSELECT_REG_BIT_ENUM(CONFIG_ERR_RESP_EN_SLAVE1,  24, DISABLE, ENABLE);
DEFINE_MSELECT_REG_BIT_ENUM(CONFIG_ERR_RESP_EN_SLAVE2,  25, DISABLE, ENABLE);
DEFINE_MSELECT_REG_BIT_ENUM(CONFIG_WRAP_TO_INCR_SLAVE0, 27, DISABLE, ENABLE);
DEFINE_MSELECT_REG_BIT_ENUM(CONFIG_WRAP_TO_INCR_SLAVE1, 28, DISABLE, ENABLE);
DEFINE_MSELECT_REG_BIT_ENUM(CONFIG_WRAP_TO_INCR_SLAVE2, 29, DISABLE, ENABLE);
DEFINE_MSELECT_REG_BIT_ENUM(CONFIG_WRAP_TO_INCR_SLAVE3, 30, DISABLE, ENABLE);
