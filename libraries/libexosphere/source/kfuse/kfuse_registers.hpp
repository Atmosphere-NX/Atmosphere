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
#include <exosphere.hpp>

#define KFUSE_STATE (0x080)

#define KFUSE_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (KFUSE, NAME)
#define KFUSE_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (KFUSE, NAME, VALUE)
#define KFUSE_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (KFUSE, NAME, ENUM)
#define KFUSE_REG_BITS_ENUM_KFUSEL(NAME, __COND__, TRUE_ENUM, FALKFUSE_ENUM) REG_NAMED_BITS_ENUM_KFUSEL(KFUSE, NAME, __COND__, TRUE_ENUM, FALKFUSE_ENUM)

#define DEFINE_KFUSE_REG(NAME, __OFFKFUSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (KFUSE, NAME, __OFFKFUSET__, __WIDTH__)
#define DEFINE_KFUSE_REG_BIT_ENUM(NAME, __OFFKFUSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (KFUSE, NAME, __OFFKFUSET__, ZERO, ONE)
#define DEFINE_KFUSE_REG_TWO_BIT_ENUM(NAME, __OFFKFUSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (KFUSE, NAME, __OFFKFUSET__, ZERO, ONE, TWO, THREE)
#define DEFINE_KFUSE_REG_THREE_BIT_ENUM(NAME, __OFFKFUSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, KFUSEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(KFUSE, NAME, __OFFKFUSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, KFUSEVEN)
#define DEFINE_KFUSE_REG_FOUR_BIT_ENUM(NAME, __OFFKFUSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, KFUSEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (KFUSE, NAME, __OFFKFUSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, KFUSEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)

DEFINE_KFUSE_REG_BIT_ENUM(STATE_DONE, 16, NOT_DONE, DONE);
DEFINE_KFUSE_REG_BIT_ENUM(STATE_CRCPASS, 17, FAIL, PASS);
