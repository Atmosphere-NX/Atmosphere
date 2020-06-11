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

SetRegisterAllowed(APBDEV_PMC_CNTRL);                /* 0x000 */
SetRegisterAllowed(APBDEV_PMC_WAKE_MASK);            /* 0x00C */
SetRegisterAllowed(APBDEV_PMC_WAKE_LVL);             /* 0x010 */
SetRegisterAllowed(APBDEV_PMC_WAKE_STATUS);          /* 0x014 */
SetRegisterAllowed(APBDEV_PMC_DPD_PADS_ORIDE);       /* 0x01C */
SetRegisterAllowed(APBDEV_PMC_DPD_SAMPLE);           /* 0x020 */
SetRegisterAllowed(APBDEV_PMC_CLAMP_STATUS);         /* 0x02C */
SetRegisterAllowed(APBDEV_PMC_PWRGATE_TOGGLE);       /* 0x030 */
SetRegisterAllowed(APBDEV_PMC_REMOVE_CLAMPING_CMD ); /* 0x034 */
SetRegisterAllowed(APBDEV_PMC_PWRGATE_STATUS);       /* 0x038 */
SetRegisterAllowed(APBDEV_PMC_PWRGOOD_TIMER);        /* 0x03C */
SetRegisterAllowed(APBDEV_PMC_BLINK_TIMER);          /* 0x040 */
SetRegisterAllowed(APBDEV_PMC_NO_IOPOWER);           /* 0x044 */
SetRegisterAllowed(APBDEV_PMC_PWR_DET);              /* 0x048 */
SetRegisterAllowed(APBDEV_PMC_AUTO_WAKE_LVL_MASK);   /* 0x0DC */
SetRegisterAllowed(APBDEV_PMC_WAKE_DELAY);           /* 0x0E0 */
SetRegisterAllowed(APBDEV_PMC_PWR_DET_VAL);          /* 0x0E4 */
SetRegisterAllowed(APBDEV_PMC_WAKE2_MASK);           /* 0x160 */
SetRegisterAllowed(APBDEV_PMC_WAKE2_LVL);            /* 0x164 */
SetRegisterAllowed(APBDEV_PMC_WAKE2_STATUS);         /* 0x168 */
SetRegisterAllowed(APBDEV_PMC_AUTO_WAKE2_LVL_MASK ); /* 0x170 */
SetRegisterAllowed(APBDEV_PMC_CLK_OUT_CNTRL);        /* 0x1A8 */
SetRegisterAllowed(APBDEV_PMC_IO_DPD_REQ);           /* 0x1B8 */
SetRegisterAllowed(APBDEV_PMC_IO_DPD_STATUS);        /* 0x1BC */
SetRegisterAllowed(APBDEV_PMC_IO_DPD2_REQ);          /* 0x1C0 */
SetRegisterAllowed(APBDEV_PMC_IO_DPD2_STATUS);       /* 0x1C4 */
SetRegisterAllowed(APBDEV_PMC_SEL_DPD_TIM);          /* 0x1C8 */
SetRegisterAllowed(APBDEV_PMC_TSC_MULT);             /* 0x2B4 */
SetRegisterAllowed(APBDEV_PMC_GPU_RG_CNTRL);         /* 0x2D4 */
SetRegisterAllowed(APBDEV_PMC_CNTRL2);               /* 0x440 */
SetRegisterAllowed(APBDEV_PMC_WAKE_DEBOUNCE_EN);     /* 0x4D8 */