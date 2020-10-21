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

#define AVP_CACHE_ADDRESS(x) (0x50040000 + x)

#define AVP_CACHE_CONFIG       (0x000)

#define AVP_CACHE_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (AVP_CACHE, NAME)
#define AVP_CACHE_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (AVP_CACHE, NAME, VALUE)
#define AVP_CACHE_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (AVP_CACHE, NAME, ENUM)
#define AVP_CACHE_REG_BITS_ENUM_SEL(NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_NAMED_BITS_ENUM_SEL(AVP_CACHE, NAME, __COND__, TRUE_ENUM, FALSE_ENUM)

#define DEFINE_AVP_CACHE_REG(NAME, __OFFSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (AVP_CACHE, NAME, __OFFSET__, __WIDTH__)
#define DEFINE_AVP_CACHE_REG_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (AVP_CACHE, NAME, __OFFSET__, ZERO, ONE)
#define DEFINE_AVP_CACHE_REG_TWO_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (AVP_CACHE, NAME, __OFFSET__, ZERO, ONE, TWO, THREE)
#define DEFINE_AVP_CACHE_REG_THREE_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(AVP_CACHE, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)
#define DEFINE_AVP_CACHE_REG_FOUR_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (AVP_CACHE, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)

DEFINE_AVP_CACHE_REG_BIT_ENUM(DISABLE_WB, 10, FALSE, TRUE);
DEFINE_AVP_CACHE_REG_BIT_ENUM(DISABLE_RB, 11, FALSE, TRUE);
