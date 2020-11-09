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

#define MIPI_CAL_MIPI_CAL_CTRL          (0x000)
#define MIPI_CAL_CIL_MIPI_CAL_STATUS    (0x008)
#define MIPI_CAL_CILA_MIPI_CAL_CONFIG   (0x014)
#define MIPI_CAL_CILB_MIPI_CAL_CONFIG   (0x018)
#define MIPI_CAL_CILC_MIPI_CAL_CONFIG   (0x01C)
#define MIPI_CAL_CILD_MIPI_CAL_CONFIG   (0x020)
#define MIPI_CAL_CILE_MIPI_CAL_CONFIG   (0x024)
#define MIPI_CAL_CILF_MIPI_CAL_CONFIG   (0x028)
#define MIPI_CAL_DSIA_MIPI_CAL_CONFIG   (0x038)
#define MIPI_CAL_DSIB_MIPI_CAL_CONFIG   (0x03C)
#define MIPI_CAL_DSIC_MIPI_CAL_CONFIG   (0x040)
#define MIPI_CAL_DSID_MIPI_CAL_CONFIG   (0x044)
#define MIPI_CAL_MIPI_BIAS_PAD_CFG0     (0x058)
#define MIPI_CAL_MIPI_BIAS_PAD_CFG1     (0x05C)
#define MIPI_CAL_MIPI_BIAS_PAD_CFG2     (0x060)
#define MIPI_CAL_DSIA_MIPI_CAL_CONFIG_2 (0x064)
#define MIPI_CAL_DSIB_MIPI_CAL_CONFIG_2 (0x068)
#define MIPI_CAL_DSIC_MIPI_CAL_CONFIG_2 (0x070)
#define MIPI_CAL_DSID_MIPI_CAL_CONFIG_2 (0x074)

#define MIPI_CAL_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (MIPI_CAL, NAME)
#define MIPI_CAL_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (MIPI_CAL, NAME, VALUE)
#define MIPI_CAL_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (MIPI_CAL, NAME, ENUM)
#define MIPI_CAL_REG_BITS_ENUM_SEL(NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_NAMED_BITS_ENUM_SEL(MIPI_CAL, NAME, __COND__, TRUE_ENUM, FALSE_ENUM)

#define DEFINE_MIPI_CAL_REG(NAME, __OFFSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (MIPI_CAL, NAME, __OFFSET__, __WIDTH__)
#define DEFINE_MIPI_CAL_REG_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (MIPI_CAL, NAME, __OFFSET__, ZERO, ONE)
#define DEFINE_MIPI_CAL_REG_TWO_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (MIPI_CAL, NAME, __OFFSET__, ZERO, ONE, TWO, THREE)
#define DEFINE_MIPI_CAL_REG_THREE_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(MIPI_CAL, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)
#define DEFINE_MIPI_CAL_REG_FOUR_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (MIPI_CAL, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)
