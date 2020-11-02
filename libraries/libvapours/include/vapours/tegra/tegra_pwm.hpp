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

#define PWM_CONTROLLER_PWM_CHANNEL_OFFSET(channel) (0x10 * channel)

#define PWM_CONTROLLER_PWM_CSR (0x000)


#define PWM_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (PWM_CONTROLLER, NAME)
#define PWM_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (PWM_CONTROLLER, NAME, VALUE)
#define PWM_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (PWM_CONTROLLER, NAME, ENUM)
#define PWM_REG_BITS_ENUM_SEL(NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_NAMED_BITS_ENUM_SEL(PWM_CONTROLLER, NAME, __COND__, TRUE_ENUM, FALSE_ENUM)

#define DEFINE_PWM_REG(NAME, __OFFSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (PWM_CONTROLLER, NAME, __OFFSET__, __WIDTH__)
#define DEFINE_PWM_REG_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (PWM_CONTROLLER, NAME, __OFFSET__, ZERO, ONE)
#define DEFINE_PWM_REG_TWO_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (PWM_CONTROLLER, NAME, __OFFSET__, ZERO, ONE, TWO, THREE)
#define DEFINE_PWM_REG_THREE_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(PWM_CONTROLLER, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)
#define DEFINE_PWM_REG_FOUR_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (PWM_CONTROLLER, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)

DEFINE_PWM_REG(PWM_CSR_PFM, 0, 13);
DEFINE_PWM_REG(PWM_CSR_PWM, 16, 15);
DEFINE_PWM_REG_BIT_ENUM(PWM_CSR_ENB, 31, DISABLE, ENABLE);

