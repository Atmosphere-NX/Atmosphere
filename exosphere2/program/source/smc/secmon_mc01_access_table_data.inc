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

SetRegisterAllowed(MC_STAT_CONTROL);                          /* 0x100 */
SetRegisterAllowed(MC_STAT_EMC_CLOCK_LIMIT);                  /* 0x108 */
SetRegisterAllowed(MC_STAT_EMC_CLOCK_LIMIT_MSBS);             /* 0x10C */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET0_ADR_LIMIT_LO);     /* 0x118 */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET0_ADR_LIMIT_HI);     /* 0x11C */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET0_SPARE);            /* 0x124 */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET0_CLIENT_0);         /* 0x128 */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET0_CLIENT_1);         /* 0x12C */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET0_CLIENT_2);         /* 0x130 */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET0_CLIENT_3);         /* 0x134 */
SetRegisterAllowed(MC_STAT_EMC_SET0_COUNT);                   /* 0x138 */
SetRegisterAllowed(MC_STAT_EMC_SET0_COUNT_MSBS);              /* 0x13C */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET1_ADR_LIMIT_LO);     /* 0x158 */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET1_ADR_LIMIT_HI);     /* 0x15C */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET1_SPARE);            /* 0x164 */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET1_CLIENT_0);         /* 0x168 */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET1_CLIENT_1);         /* 0x16C */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET1_CLIENT_2);         /* 0x170 */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET1_CLIENT_3);         /* 0x174 */
SetRegisterAllowed(MC_STAT_EMC_SET1_COUNT);                   /* 0x178 */
SetRegisterAllowed(MC_STAT_EMC_SET1_COUNT_MSBS);              /* 0x17C */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET0_ADR_LIMIT_UPPER);  /* 0xA20 */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET1_ADR_LIMIT_UPPER);  /* 0xA24 */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET0_CLIENT_4);         /* 0xB88 */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET1_CLIENT_4);         /* 0xB8C */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET0_CLIENT_5);         /* 0xBC4 */
SetRegisterAllowed(MC_STAT_EMC_FILTER_SET1_CLIENT_5);         /* 0xBC8 */