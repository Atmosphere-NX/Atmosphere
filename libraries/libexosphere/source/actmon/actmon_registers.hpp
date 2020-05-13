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
#include <exosphere.hpp>

namespace ams::actmon {

    #define ACTMON_GLB_PERIOD_CTRL (0x004)
    #define ACTMON_COP_CTRL        (0x0C0)
    #define ACTMON_COP_UPPER_WMARK (0x0C4)
    #define ACTMON_COP_INTR_STATUS (0x0E4)

    /* Actmon source enums. */
    #define ACTMON_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (ACTMON, NAME)
    #define ACTMON_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (ACTMON, NAME, VALUE)
    #define ACTMON_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (ACTMON, NAME, ENUM)
    #define ACTMON_REG_BITS_ENUM_SEL(NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_NAMED_BITS_ENUM_SEL(ACTMON, NAME, __COND__, TRUE_ENUM, FALSE_ENUM)

    #define DEFINE_ACTMON_REG(NAME, __OFFSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (ACTMON, NAME, __OFFSET__, __WIDTH__)
    #define DEFINE_ACTMON_REG_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (ACTMON, NAME, __OFFSET__, ZERO, ONE)
    #define DEFINE_ACTMON_REG_TWO_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (ACTMON, NAME, __OFFSET__, ZERO, ONE, TWO, THREE)
    #define DEFINE_ACTMON_REG_THREE_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(ACTMON, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)
    #define DEFINE_ACTMON_REG_FOUR_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (ACTMON, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)

    DEFINE_ACTMON_REG(GLB_PERIOD_CTRL_SAMPLE_PERIOD, 0, 8);
    DEFINE_ACTMON_REG_BIT_ENUM(GLB_PERIOD_CTRL_SOURCE, 8, MSEC, USEC);

    DEFINE_ACTMON_REG(COP_CTRL_K_VAL, 10, 3);
    DEFINE_ACTMON_REG_BIT_ENUM(COP_CTRL_ENB_PERIODIC,               18, DISABLE, ENABLE);
    DEFINE_ACTMON_REG_BIT_ENUM(COP_CTRL_AT_END_EN,                  19, DISABLE, ENABLE);
    DEFINE_ACTMON_REG_BIT_ENUM(COP_CTRL_AVG_BELOW_WMARK_EN,         20, DISABLE, ENABLE);
    DEFINE_ACTMON_REG_BIT_ENUM(COP_CTRL_AVG_ABOVE_WMARK_EN,         21, DISABLE, ENABLE);
    DEFINE_ACTMON_REG_BIT_ENUM(COP_CTRL_WHEN_OVERFLOW_EN,           22, DISABLE, ENABLE);
    DEFINE_ACTMON_REG(COP_CTRL_BELOW_WMARK_NUM, 23, 3);
    DEFINE_ACTMON_REG(COP_CTRL_ABOVE_WMARK_NUM, 26, 3);
    DEFINE_ACTMON_REG_BIT_ENUM(COP_CTRL_CONSECUTIVE_BELOW_WMARK_EN, 29, DISABLE, ENABLE);
    DEFINE_ACTMON_REG_BIT_ENUM(COP_CTRL_CONSECUTIVE_ABOVE_WMARK_EN, 30, DISABLE, ENABLE);
    DEFINE_ACTMON_REG_BIT_ENUM(COP_CTRL_ENB,                        31, DISABLE, ENABLE);

}
