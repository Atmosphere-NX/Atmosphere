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
#include <stratosphere.hpp>
#include "boot_boot_reason.hpp"
#include "boot_pmic_driver.hpp"
#include "boot_rtc_driver.hpp"

namespace ams::boot {

    namespace {

        /* Globals. */
        constinit spl::BootReason g_boot_reason = spl::BootReason_Unknown;
        constinit bool g_detected_boot_reason = false;

        /* Helpers. */
        spl::BootReason DetectBootReason(u32 power_intr, u8 rtc_intr, u8 nv_erc, bool ac_ok) {
            if (power_intr & 0x08) {
                return spl::BootReason_OnKey;
            }
            if (rtc_intr & 0x02)  {
                return spl::BootReason_RtcAlarm1;
            }
            if (power_intr & 0x80) {
                return spl::BootReason_AcOk;
            }
            if (rtc_intr & 0x04) {
                if (nv_erc != 0x80 && !spl::IsRecoveryBoot()) {
                    return spl::BootReason_RtcAlarm2;
                }
            }
            if ((nv_erc & 0x40) && ac_ok) {
                return spl::BootReason_AcOk;
            }
            return spl::BootReason_Unknown;
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
            R_ABORT_UNLESS(pmic_driver.GetOnOffIrq(std::addressof(power_intr)));
            R_ABORT_UNLESS(pmic_driver.GetNvErc(std::addressof(nv_erc)));
            R_ABORT_UNLESS(pmic_driver.GetAcOk(std::addressof(ac_ok)));
        }

        /* Get values from RTC. */
        {
            RtcDriver rtc_driver;
            R_ABORT_UNLESS(rtc_driver.GetRtcIntr(std::addressof(rtc_intr)));
            R_ABORT_UNLESS(rtc_driver.GetRtcIntrM(std::addressof(rtc_intr_m)));
        }

        /* Set global derived boot reason. */
        g_boot_reason = DetectBootReason(power_intr, rtc_intr & ~rtc_intr_m, nv_erc, ac_ok);

        /* Set boot reason for SPL. */
        if (hos::GetVersion() >= hos::Version_3_0_0) {
            /* Create the boot reason value. */
            spl::BootReasonValue boot_reason_value = {};
            boot_reason_value.power_intr  = power_intr;
            boot_reason_value.rtc_intr    = rtc_intr & ~rtc_intr_m;
            boot_reason_value.nv_erc      = nv_erc;
            boot_reason_value.boot_reason = g_boot_reason;

            /* Set the boot reason value. */
            R_ABORT_UNLESS(spl::SetBootReason(boot_reason_value));
        }

        g_detected_boot_reason = true;
    }

    spl::BootReason GetBootReason() {
        AMS_ABORT_UNLESS(g_detected_boot_reason);
        return g_boot_reason;
    }

}