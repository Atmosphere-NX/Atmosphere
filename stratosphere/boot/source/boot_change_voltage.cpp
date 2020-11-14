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
#include "boot_change_voltage.hpp"

namespace ams::boot {

    namespace {

        /* Convenience definitions. */
        constexpr u32 Sdmmc3VoltageBit = (1 << 13); /* SDMMC3 */
        constexpr u32 AudioVoltageBit  = (1 << 18); /* AUDIO_HV */
        constexpr u32 GpioVoltageBit   = (1 << 21); /* GPIO */
        constexpr u32 SpiVoltageBit    = (1 << 23); /* SPI_HV */

        constexpr u32 VoltageChangeMask = SpiVoltageBit | GpioVoltageBit | AudioVoltageBit | Sdmmc3VoltageBit;

        constexpr u32 PmcPwrDet    = 0x7000E448;
        constexpr u32 PmcPwrDetVal = 0x7000E4E4;

    }

    void ChangeGpioVoltageTo1_8v() {
        /* Write mask to APBDEV_PMC_PWR_DET, then clear APBDEV_PMC_PWR_DET_VAL. */
        dd::ReadModifyWriteIoRegister(PmcPwrDet,    VoltageChangeMask, VoltageChangeMask);
        dd::ReadModifyWriteIoRegister(PmcPwrDetVal,                 0, VoltageChangeMask);

        /* Sleep for 100 us. */
        os::SleepThread(TimeSpan::FromMicroSeconds(100));
    }

}
