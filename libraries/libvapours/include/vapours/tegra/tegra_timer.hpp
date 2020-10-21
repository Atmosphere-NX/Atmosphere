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

#define TIMERUS_USEC_CFG              (0x014)
#define TIMER_SHARED_TIMER_SECURE_CFG (0x1A4)

#define TIMER_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (TIMER, NAME)
#define TIMER_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (TIMER, NAME, VALUE)
#define TIMER_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (TIMER, NAME, ENUM)
#define TIMER_REG_BITS_ENUM_SEL(NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_NAMED_BITS_ENUM_SEL(TIMER, NAME, __COND__, TRUE_ENUM, FALSE_ENUM)

#define DEFINE_TIMER_REG(NAME, __OFFSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (TIMER, NAME, __OFFSET__, __WIDTH__)
#define DEFINE_TIMER_REG_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (TIMER, NAME, __OFFSET__, ZERO, ONE)
#define DEFINE_TIMER_REG_TWO_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (TIMER, NAME, __OFFSET__, ZERO, ONE, TWO, THREE)
#define DEFINE_TIMER_REG_THREE_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(TIMER, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)
#define DEFINE_TIMER_REG_FOUR_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (TIMER, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)

DEFINE_TIMER_REG(USEC_CFG_USEC_DIVISOR,  0, 8);
DEFINE_TIMER_REG(USEC_CFG_USEC_DIVIDEND, 8, 8);

DEFINE_TIMER_REG_BIT_ENUM(SHARED_TIMER_SECURE_CFG_TMR5, 5, DISABLE, ENABLE);
DEFINE_TIMER_REG_BIT_ENUM(SHARED_TIMER_SECURE_CFG_TMR6, 6, DISABLE, ENABLE);
DEFINE_TIMER_REG_BIT_ENUM(SHARED_TIMER_SECURE_CFG_TMR7, 7, DISABLE, ENABLE);
DEFINE_TIMER_REG_BIT_ENUM(SHARED_TIMER_SECURE_CFG_TMR8, 8, DISABLE, ENABLE);

DEFINE_TIMER_REG_BIT_ENUM(SHARED_TIMER_SECURE_CFG_WDT0, 12, DISABLE, ENABLE);
DEFINE_TIMER_REG_BIT_ENUM(SHARED_TIMER_SECURE_CFG_WDT1, 13, DISABLE, ENABLE);
DEFINE_TIMER_REG_BIT_ENUM(SHARED_TIMER_SECURE_CFG_WDT2, 14, DISABLE, ENABLE);
DEFINE_TIMER_REG_BIT_ENUM(SHARED_TIMER_SECURE_CFG_WDT3, 15, DISABLE, ENABLE);