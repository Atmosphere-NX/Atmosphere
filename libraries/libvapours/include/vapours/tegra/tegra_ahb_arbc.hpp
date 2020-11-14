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

#define AHB_ARBC(x) (0x6000c000 + x)

#define AHB_ARBITRATION_DISABLE       (0x004)
#define AHB_ARBITRATION_PRIORITY_CTRL (0x008)
#define AHB_MASTER_SWID               (0x018)
#define AHB_MASTER_SWID_1             (0x038)
#define AHB_GIZMO_TZRAM               (0x054)
#define AHB_AHB_SPARE_REG             (0x110)

#define AHB_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (AHB_, NAME)
#define AHB_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (AHB_, NAME, VALUE)
#define AHB_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (AHB_, NAME, ENUM)
#define AHB_REG_BITS_ENUM_SEL(NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_NAMED_BITS_ENUM_SEL(AHB_, NAME, __COND__, TRUE_ENUM, FALSE_ENUM)

#define DEFINE_AHB_REG(NAME, __OFFSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (AHB_, NAME, __OFFSET__, __WIDTH__)
#define DEFINE_AHB_REG_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (AHB_, NAME, __OFFSET__, ZERO, ONE)
#define DEFINE_AHB_REG_TWO_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (AHB_, NAME, __OFFSET__, ZERO, ONE, TWO, THREE)
#define DEFINE_AHB_REG_THREE_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(AHB_, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)
#define DEFINE_AHB_REG_FOUR_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (AHB_, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)

DEFINE_AHB_REG_BIT_ENUM(ARBITRATION_DISABLE_COP,     1, ENABLE, DISABLE);
DEFINE_AHB_REG_BIT_ENUM(ARBITRATION_DISABLE_AHBDMA,  5, ENABLE, DISABLE);
DEFINE_AHB_REG_BIT_ENUM(ARBITRATION_DISABLE_USB,     6, ENABLE, DISABLE);
DEFINE_AHB_REG_BIT_ENUM(ARBITRATION_DISABLE_USB2,   18, ENABLE, DISABLE);

DEFINE_AHB_REG(AHB_SPARE_REG_CSITE_PADMACRO3_TRIM_SEL, 0, 5);
DEFINE_AHB_REG_BIT_ENUM(AHB_SPARE_REG_OBS_OVERRIDE_EN, 5, DISABLE, ENABLE);
DEFINE_AHB_REG_BIT_ENUM(AHB_SPARE_REG_APB2JTAG_OVERRIDE_EN, 6, DISABLE, ENABLE);
DEFINE_AHB_REG(AHB_SPARE_REG_AHB_SPARE_REG, 12, 32-12);
