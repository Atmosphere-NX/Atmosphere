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

#include <stratosphere/spl.hpp>

#include "boot_boot_reason.hpp"
#include "boot_pmic_driver.hpp"
#include "boot_rtc_driver.hpp"

namespace sts::boot {

    namespace {

        /* Types. */
        struct BootReasonValue {
            union {
                struct {
                    u8 power_intr;
                    u8 rtc_intr;
                    u8 nv_erc;
                    u8 boot_reason;
                };
                u32 value;
            };
        };

        /* Globals. */
        u32 g_boot_reason = 0;
        bool g_detected_boot_reason = false;

        /* Helpers. */
        u32 MakeBootReason(u32 power_intr, u8 rtc_intr, u8 nv_erc, bool ac_ok) {
            if (power_intr & 0x08) {
                return 2;
            }
            if (rtc_intr & 0x02)  {
                return 3;
            }
            if (power_intr & 0x80) {
                return 1;
            }
            if (rtc_intr & 0x04) {
                if (nv_erc != 0x80 && !spl::IsRecoveryBoot()) {
                    return 4;
                }
            }
            if ((nv_erc & 0x40) && ac_ok) {
                return 1;
            }
            return 0;
        }

    }

    void DetectBootReason() {
        u8 power_intr;
        u8 rtc_intr;
        u8 rtc_intr_m;
        u8 nv_erc;
        bool ac_ok;

        /* Get values from PMIC. */
        {
            PmicDriver pmic_driver;
            if (R_FAILED(pmic_driver.GetPowerIntr(&power_intr))) {
                std::abort();
            }
            if (R_FAILED(pmic_driver.GetNvErc(&nv_erc))) {
                std::abort();
            }
            if (R_FAILED(pmic_driver.GetAcOk(&ac_ok))) {
                std::abort();
            }
        }

        /* Get values from RTC. */
        {
            RtcDriver rtc_driver;
            if (R_FAILED(rtc_driver.GetRtcIntr(&rtc_intr))) {
                std::abort();
            }
            if (R_FAILED(rtc_driver.GetRtcIntrM(&rtc_intr_m))) {
                std::abort();
            }
        }

        /* Set global derived boot reason. */
        g_boot_reason = MakeBootReason(power_intr, rtc_intr & ~rtc_intr_m, nv_erc, ac_ok);

        /* Set boot reason for SPL. */
        if (GetRuntimeFirmwareVersion() >= FirmwareVersion_300) {
            BootReasonValue boot_reason_value;
            boot_reason_value.power_intr = power_intr;
            boot_reason_value.rtc_intr = rtc_intr & ~rtc_intr_m;
            boot_reason_value.nv_erc = nv_erc;
            boot_reason_value.boot_reason = g_boot_reason;
            if (R_FAILED(splSetBootReason(boot_reason_value.value))) {
                std::abort();
            }
        }

        g_detected_boot_reason = true;
    }

    u32 GetBootReason() {
        if (!g_detected_boot_reason) {
            std::abort();
        }

        return g_boot_reason;
    }

}