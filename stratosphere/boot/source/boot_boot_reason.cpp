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
#include "boot_boot_reason.hpp"
#include "boot_pmic_driver.hpp"
#include "boot_rtc_driver.hpp"

namespace ams::boot {

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
            R_ABORT_UNLESS(pmic_driver.GetPowerIntr(&power_intr));
            R_ABORT_UNLESS(pmic_driver.GetNvErc(&nv_erc));
            R_ABORT_UNLESS(pmic_driver.GetAcOk(&ac_ok));
        }

        /* Get values from RTC. */
        {
            RtcDriver rtc_driver;
            R_ABORT_UNLESS(rtc_driver.GetRtcIntr(&rtc_intr));
            R_ABORT_UNLESS(rtc_driver.GetRtcIntrM(&rtc_intr_m));
        }

        /* Set global derived boot reason. */
        g_boot_reason = MakeBootReason(power_intr, rtc_intr & ~rtc_intr_m, nv_erc, ac_ok);

        /* Set boot reason for SPL. */
        if (hos::GetVersion() >= hos::Version_3_0_0) {
            BootReasonValue boot_reason_value;
            boot_reason_value.power_intr = power_intr;
            boot_reason_value.rtc_intr = rtc_intr & ~rtc_intr_m;
            boot_reason_value.nv_erc = nv_erc;
            boot_reason_value.boot_reason = g_boot_reason;
            R_ABORT_UNLESS(splSetBootReason(boot_reason_value.value));
        }

        g_detected_boot_reason = true;
    }

    u32 GetBootReason() {
        AMS_ABORT_UNLESS(g_detected_boot_reason);
        return g_boot_reason;
    }

}