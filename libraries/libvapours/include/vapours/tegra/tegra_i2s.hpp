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
#pragma once
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/literals.hpp>
#include <vapours/util.hpp>
#include <vapours/results.hpp>
#include <vapours/reg.hpp>

#define I2S_REG(x) (0x702d1000 + x)


#define I2S0_I2S_CG   (0x088)
#define I2S0_I2S_CTRL (0x0A0)
#define I2S1_I2S_CG   (0x188)
#define I2S1_I2S_CTRL (0x1A0)
#define I2S2_I2S_CG   (0x288)
#define I2S2_I2S_CTRL (0x2A0)
#define I2S3_I2S_CG   (0x388)
#define I2S3_I2S_CTRL (0x3A0)
#define I2S4_I2S_CG   (0x488)
#define I2S4_I2S_CTRL (0x4A0)

#define I2S_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (I2S, NAME)
#define I2S_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (I2S, NAME, VALUE)
#define I2S_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (I2S, NAME, ENUM)
#define I2S_REG_BITS_ENUM_SEL(NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_NAMED_BITS_ENUM_SEL(I2S, NAME, __COND__, TRUE_ENUM, FALSE_ENUM)

#define DEFINE_I2S_REG(NAME, __OFFSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (I2S, NAME, __OFFSET__, __WIDTH__)
#define DEFINE_I2S_REG_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (I2S, NAME, __OFFSET__, ZERO, ONE)
#define DEFINE_I2S_REG_TWO_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (I2S, NAME, __OFFSET__, ZERO, ONE, TWO, THREE)
#define DEFINE_I2S_REG_THREE_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(I2S, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)
#define DEFINE_I2S_REG_FOUR_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (I2S, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)


DEFINE_I2S_REG_BIT_ENUM(I2S_CG_SLCG_ENABLE, 0, FALSE, TRUE);

DEFINE_I2S_REG_BIT_ENUM(I2S_CTRL_MASTER, 10, DISABLE, ENABLE);