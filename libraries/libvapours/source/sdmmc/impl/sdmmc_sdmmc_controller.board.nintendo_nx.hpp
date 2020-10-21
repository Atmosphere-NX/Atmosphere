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
#include "sdmmc_sd_host_standard_controller.hpp"
#include "sdmmc_clock_reset_controller.hpp"

namespace ams::sdmmc::impl {

    bool IsSocMariko();

    constexpr inline size_t SdmmcRegistersSize = 0x200;

    class SdmmcController : public SdHostStandardController {
        private:
            struct SdmmcRegisters {
                /* Standard registers. */
                volatile SdHostStandardRegisters sd_host_standard_registers;

                /* Vendor specific registers */
                volatile uint32_t vendor_clock_cntrl;
                volatile uint32_t vendor_sys_sw_cntrl;
                volatile uint32_t vendor_err_intr_status;
                volatile uint32_t vendor_cap_overrides;
                volatile uint32_t vendor_boot_cntrl;
                volatile uint32_t vendor_boot_ack_timeout;
                volatile uint32_t vendor_boot_dat_timeout;
                volatile uint32_t vendor_debounce_count;
                volatile uint32_t vendor_misc_cntrl;
                volatile uint32_t max_current_override;
                volatile uint32_t max_current_override_hi;
                volatile uint32_t _0x12c[0x20];
                volatile uint32_t vendor_io_trim_cntrl;

                /* Start of sdmmc2/sdmmc4 only */
                volatile uint32_t vendor_dllcal_cfg;
                volatile uint32_t vendor_dll_ctrl0;
                volatile uint32_t vendor_dll_ctrl1;
                volatile uint32_t vendor_dllcal_cfg_sta;
                /* End of sdmmc2/sdmmc4 only */

                volatile uint32_t vendor_tuning_cntrl0;
                volatile uint32_t vendor_tuning_cntrl1;
                volatile uint32_t vendor_tuning_status0;
                volatile uint32_t vendor_tuning_status1;
                volatile uint32_t vendor_clk_gate_hysteresis_count;
                volatile uint32_t vendor_preset_val0;
                volatile uint32_t vendor_preset_val1;
                volatile uint32_t vendor_preset_val2;
                volatile uint32_t sdmemcomppadctrl;
                volatile uint32_t auto_cal_config;
                volatile uint32_t auto_cal_interval;
                volatile uint32_t auto_cal_status;
                volatile uint32_t io_spare;
                volatile uint32_t sdmmca_mccif_fifoctrl;
                volatile uint32_t timeout_wcoal_sdmmca;
                volatile uint32_t _0x1fc;
            };
            static_assert(sizeof(SdmmcRegisters) == SdmmcRegistersSize);
        private:
            SdmmcRegisters *sdmmc_registers;
            /* TODO */
        public:
            explicit SdmmcController(dd::PhysicalAddress registers_phys_addr) : SdHostStandardController(registers_phys_addr, SdmmcRegistersSize) {
                /* Set sdmmc registers. */
                static_assert(offsetof(SdmmcRegisters, sd_host_standard_registers) == 0);
                this->sdmmc_registers = reinterpret_cast<SdmmcRegisters *>(this->registers);
            }

            /* TODO */
    };

    class Sdmmc2And4Controller : public SdmmcController {
        /* TODO */
        public:
            explicit Sdmmc2And4Controller(dd::PhysicalAddress registers_phys_addr) : SdmmcController(registers_phys_addr) {
                /* ... */
            }

            /* TODO */
    };

    constexpr inline dd::PhysicalAddress Sdmmc4RegistersPhysicalAddress = UINT64_C(0x700B0600);

    class Sdmmc4Controller : public Sdmmc2And4Controller {
        /* TODO */
        public:
            Sdmmc4Controller() : Sdmmc2And4Controller(Sdmmc4RegistersPhysicalAddress) {
                /* ... */
            }

            /* TODO */
    };

}
