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

#define TSEC_FALCON_IRQMSET      (0x1010)
#define TSEC_FALCON_IRQDEST      (0x101C)
#define TSEC_FALCON_MAILBOX0     (0x1040)
#define TSEC_FALCON_MAILBOX1     (0x1044)
#define TSEC_FALCON_ITFEN        (0x1048)
#define TSEC_FALCON_CPUCTL       (0x1100)
#define TSEC_FALCON_BOOTVEC      (0x1104)
#define TSEC_FALCON_DMACTL       (0x110C)
#define TSEC_FALCON_DMATRFBASE   (0x1110)
#define TSEC_FALCON_DMATRFMOFFS  (0x1114)
#define TSEC_FALCON_DMATRFCMD    (0x1118)
#define TSEC_FALCON_DMATRFFBOFFS (0x111C)

#define TSEC_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (TSEC, NAME)
#define TSEC_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (TSEC, NAME, VALUE)
#define TSEC_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (TSEC, NAME, ENUM)
#define TSEC_REG_BITS_ENUM_TSECL(NAME, __COND__, TRUE_ENUM, FALTSEC_ENUM) REG_NAMED_BITS_ENUM_TSECL(TSEC, NAME, __COND__, TRUE_ENUM, FALTSEC_ENUM)

#define DEFINE_TSEC_REG(NAME, __OFFTSECT__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (TSEC, NAME, __OFFTSECT__, __WIDTH__)
#define DEFINE_TSEC_REG_BIT_ENUM(NAME, __OFFTSECT__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (TSEC, NAME, __OFFTSECT__, ZERO, ONE)
#define DEFINE_TSEC_REG_TWO_BIT_ENUM(NAME, __OFFTSECT__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (TSEC, NAME, __OFFTSECT__, ZERO, ONE, TWO, THREE)
#define DEFINE_TSEC_REG_THREE_BIT_ENUM(NAME, __OFFTSECT__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, TSECVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(TSEC, NAME, __OFFTSECT__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, TSECVEN)
#define DEFINE_TSEC_REG_FOUR_BIT_ENUM(NAME, __OFFTSECT__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, TSECVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (TSEC, NAME, __OFFTSECT__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, TSECVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)

DEFINE_TSEC_REG_BIT_ENUM(FALCON_CPUCTL_STARTCPU, 1, FALSE, TRUE);
DEFINE_TSEC_REG_BIT_ENUM(FALCON_CPUCTL_HALTED, 4, FALSE, TRUE);

DEFINE_TSEC_REG(FALCON_DMATRFMOFFS_OFFSET, 0, 16);

DEFINE_TSEC_REG_BIT_ENUM(FALCON_DMATRFCMD_BUSY, 1, BUSY, IDLE);
DEFINE_TSEC_REG_BIT_ENUM(FALCON_DMATRFCMD_TO, 4, DMEM, IMEM);
DEFINE_TSEC_REG_THREE_BIT_ENUM(FALCON_DMATRFCMD_SIZE, 8, 4B, 8B, 16B, 32B, 64B, 128B, 256B, RSVD7);