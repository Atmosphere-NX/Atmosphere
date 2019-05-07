/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include "boot_functions.hpp"
#include "boot_registers_pmc.hpp"
#include "boot_wake_control_configs.hpp"

static void UpdatePmcControlBit(const u32 reg_offset, const u32 mask_val, const bool flag) {
    Boot::WritePmcRegister(PmcBase + reg_offset, flag ? UINT32_MAX : 0, mask_val);
    Boot::ReadPmcRegister(PmcBase + reg_offset);
}

static void InitializePmcWakeConfiguration(const bool is_blink) {
    /* Initialize APBDEV_PMC_WAKE_DEBOUNCE_EN, do a dummy read. */
    Boot::WritePmcRegister(PmcBase + APBDEV_PMC_WAKE_DEBOUNCE_EN, 0);
    Boot::ReadPmcRegister(PmcBase + APBDEV_PMC_WAKE_DEBOUNCE_EN);

    /* Initialize APBDEV_PMC_BLINK_TIMER, do a dummy read. */
    Boot::WritePmcRegister(PmcBase + APBDEV_PMC_BLINK_TIMER, 0x8008800);
    Boot::ReadPmcRegister(PmcBase + APBDEV_PMC_BLINK_TIMER);

    /* Set control bits, do dummy reads. */
    for (size_t i = 0; i < NumWakeControlConfigs; i++) {
        UpdatePmcControlBit(WakeControlConfigs[i].reg_offset, WakeControlConfigs[i].mask_val, WakeControlConfigs[i].flag_val);
    }

    /* Set bit 0x80 in APBDEV_PMC_CNTRL based on is_blink, do dummy read. */
    UpdatePmcControlBit(APBDEV_PMC_CNTRL, 0x80, is_blink);

    /* Set bit 0x100000 in APBDEV_PMC_DPD_PADS_ORIDE based on is_blink, do dummy read. */
    UpdatePmcControlBit(APBDEV_PMC_DPD_PADS_ORIDE, 0x100000, is_blink);
}

void Boot::SetInitialWakePinConfiguration() {
    InitializePmcWakeConfiguration(false);

    /* TODO: Wake event levels, wake event enables. */
}
