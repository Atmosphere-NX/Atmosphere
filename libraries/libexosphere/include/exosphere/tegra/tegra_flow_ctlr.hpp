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


#define FLOW_CTLR_FLOW_DBG_QUAL        (0x050)
#define FLOW_CTLR_BPMP_CLUSTER_CONTROL (0x098)

#define FLOW_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (FLOW_CTLR, NAME)
#define FLOW_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (FLOW_CTLR, NAME, VALUE)
#define FLOW_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (FLOW_CTLR, NAME, ENUM)
#define FLOW_REG_BITS_ENUM_SEL(NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_NAMED_BITS_ENUM_SEL(FLOW_CTLR, NAME, __COND__, TRUE_ENUM, FALSE_ENUM)

#define DEFINE_FLOW_REG(NAME, __OFFSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (FLOW_CTLR, NAME, __OFFSET__, __WIDTH__)
#define DEFINE_FLOW_REG_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (FLOW_CTLR, NAME, __OFFSET__, ZERO, ONE)
#define DEFINE_FLOW_REG_TWO_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (FLOW_CTLR, NAME, __OFFSET__, ZERO, ONE, TWO, THREE)
#define DEFINE_FLOW_REG_THREE_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(FLOW_CTLR, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)
#define DEFINE_FLOW_REG_FOUR_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (FLOW_CTLR, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)

DEFINE_FLOW_REG_BIT_ENUM(FLOW_DBG_QUAL_FIQ2CCPLEX_ENABLE, 28, DISABLE, ENABLE);

DEFINE_FLOW_REG_BIT_ENUM(BPMP_CLUSTER_CONTROL_ACTIVE_CLUSTER,        0, FAST, SLOW);
DEFINE_FLOW_REG_BIT_ENUM(BPMP_CLUSTER_CONTROL_CLUSTER_SWITCH_ENABLE, 1, DISABLE, ENABLE);
DEFINE_FLOW_REG_BIT_ENUM(BPMP_CLUSTER_CONTROL_ACTIVE_CLUSTER_LOCK,   2, DISABLE, ENABLE);
