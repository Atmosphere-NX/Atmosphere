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
#include "boot_pmc_wrapper.hpp"
#include "boot_wake_pins.hpp"

#include "boot_registers_pmc.hpp"

namespace ams::boot {

    /* Include configuration into anonymous namespace. */
    namespace {

        struct WakePinConfig {
            u32 index;
            bool enabled;
            u32 level;
        };

#include "boot_wake_control_configs.inc"
#include "boot_wake_pin_configuration_icosa.inc"
#include "boot_wake_pin_configuration_copper.inc"
#include "boot_wake_pin_configuration_hoag.inc"
#include "boot_wake_pin_configuration_iowa.inc"
#include "boot_wake_pin_configuration_calcio.inc"

    }

    namespace {

        /* Helpers. */
        void UpdatePmcControlBit(const u32 reg_offset, const u32 mask_val, const bool flag) {
            WritePmcRegister(PmcBase + reg_offset, flag ? UINT32_MAX : 0, mask_val);
            ReadPmcRegister(PmcBase + reg_offset);
        }

        void InitializePmcWakeConfiguration(const bool is_blink) {
            /* Initialize APBDEV_PMC_WAKE_DEBOUNCE_EN, do a dummy read. */
            WritePmcRegister(PmcBase + APBDEV_PMC_WAKE_DEBOUNCE_EN, 0);
            ReadPmcRegister(PmcBase + APBDEV_PMC_WAKE_DEBOUNCE_EN);

            /* Initialize APBDEV_PMC_BLINK_TIMER, do a dummy read. */
            WritePmcRegister(PmcBase + APBDEV_PMC_BLINK_TIMER, 0x8008800);
            ReadPmcRegister(PmcBase + APBDEV_PMC_BLINK_TIMER);

            /* Set control bits, do dummy reads. */
            for (size_t i = 0; i < NumWakeControlConfigs; i++) {
                UpdatePmcControlBit(WakeControlConfigs[i].reg_offset, WakeControlConfigs[i].mask_val, WakeControlConfigs[i].flag_val);
            }

            /* Set bit 0x80 in APBDEV_PMC_CNTRL based on is_blink, do dummy read. */
            UpdatePmcControlBit(APBDEV_PMC_CNTRL, 0x80, is_blink);

            /* Set bit 0x100000 in APBDEV_PMC_DPD_PADS_ORIDE based on is_blink, do dummy read. */
            UpdatePmcControlBit(APBDEV_PMC_DPD_PADS_ORIDE, 0x100000, is_blink);
        }

        void SetWakeEventLevel(u32 index, u32 level) {
            u32 pmc_wake_level_reg_offset = index <= 0x1F ? APBDEV_PMC_WAKE_LVL : APBDEV_PMC_WAKE2_LVL;
            u32 pmc_wake_level_mask_reg_offset = index <= 0x1F ? APBDEV_PMC_AUTO_WAKE_LVL_MASK : APBDEV_PMC_AUTO_WAKE2_LVL_MASK;
            if (level != 2) {
                std::swap(pmc_wake_level_reg_offset, pmc_wake_level_mask_reg_offset);
            }

            const u32 mask_val = (1 << (index & 0x1F));

            /* Clear level reg bit. */
            UpdatePmcControlBit(pmc_wake_level_reg_offset, mask_val, false);

            /* Set or clear mask reg bit. */
            UpdatePmcControlBit(pmc_wake_level_mask_reg_offset, mask_val, level > 0);
        }

        void SetWakeEventEnabled(u32 index, bool enabled) {
            /* Set or clear enabled bit. */
            UpdatePmcControlBit(index <= 0x1F ? APBDEV_PMC_WAKE_MASK : APBDEV_PMC_WAKE2_MASK, (1 << (index & 0x1F)), enabled);
        }

    }

    void SetInitialWakePinConfiguration() {
        InitializePmcWakeConfiguration(false);

        /* Set wake event levels, wake event enables. */
        const WakePinConfig *configs = nullptr;
        size_t num_configs = 0;

        switch (spl::GetHardwareType()) {
            case spl::HardwareType::Icosa:
                configs     = WakePinConfigsIcosa;
                num_configs = NumWakePinConfigsIcosa;
                break;
            case spl::HardwareType::Copper:
                configs     = WakePinConfigsCopper;
                num_configs = NumWakePinConfigsCopper;
                break;
            case spl::HardwareType::Hoag:
                configs     = WakePinConfigsHoag;
                num_configs = NumWakePinConfigsHoag;
                break;
            case spl::HardwareType::Iowa:
                configs     = WakePinConfigsIowa;
                num_configs = NumWakePinConfigsIowa;
            case spl::HardwareType::Calcio:
                configs     = WakePinConfigsCalcio;
                num_configs = NumWakePinConfigsCalcio;
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        AMS_ABORT_UNLESS(configs != nullptr);

        for (size_t i = 0; i < num_configs; i++) {
            SetWakeEventLevel(configs[i].index, configs[i].level);
            SetWakeEventEnabled(configs[i].index, configs[i].enabled);
        }
    }

}
