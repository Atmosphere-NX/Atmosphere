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

#include "utils.h"
#include "lp0.h"
#include "emc.h"
#include "pmc.h"
#include "timer.h"

void emc_configure_pmacro_training(void) {
    /* Set DISABLE_CFG_BYTEN for all N. */
    EMC_PMACRO_CFG_PM_GLOBAL_0_0 = 0xFF0000;
    
    /* Set CHN_TRAINING_E_WRPTR for channel 0 + channel 1. */
    EMC_PMACRO_TRAINING_CTRL_0_0 = 8;
    EMC_PMACRO_TRAINING_CTRL_1_0 = 8;
    
    /* Clear DISABLE_CFG_BYTEN for all N. */
    EMC_PMACRO_CFG_PM_GLOBAL_0_0 = 0x0;
}