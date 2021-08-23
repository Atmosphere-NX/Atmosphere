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
#include <exosphere.hpp>
#include "fusee_display.hpp"
#include "fusee_secure_initialize.hpp"
#include "fusee_sdram.hpp"
#include "fusee_sd_card.hpp"

namespace ams::nxboot {

    void Main() {
        /* Perform secure hardware initialization. */
        SecureInitialize(true);

        /* Initialize Sdram. */
        InitializeSdram();

        /* Initialize cache. */
        hw::InitializeDataCache();

        /* Initialize SD card. */
        Result result = InitializeSdCard();

        /* DEBUG: Fatal error context */
        {
            const ams::impl::FatalErrorContext *f_ctx = reinterpret_cast<const ams::impl::FatalErrorContext *>(0x4003E000);
            InitializeDisplay();
            ShowDisplay(f_ctx, ResultSuccess());
        }

        /* DEBUG: Check SD card connection. */
        {
            *reinterpret_cast<volatile u32 *>(0x40038000) = 0xAAAAAAAA;
            *reinterpret_cast<volatile u32 *>(0x40038004) = result.GetValue();
            if (R_SUCCEEDED(result)) {
                sdmmc::SpeedMode sm;
                sdmmc::BusWidth bw;
                *reinterpret_cast<volatile u32 *>(0x40038008) = CheckSdCardConnection(std::addressof(sm), std::addressof(bw)).GetValue();
                *reinterpret_cast<volatile u32 *>(0x4003800C) = static_cast<u32>(sm);
                *reinterpret_cast<volatile u32 *>(0x40038010) = static_cast<u32>(bw);
            }

            //*reinterpret_cast<volatile u32 *>(0x7000E400) = 0x10;
        }

        /* TODO */
        AMS_INFINITE_LOOP();
    }

}
