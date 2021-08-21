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

namespace ams::tsec {

    namespace {

        enum TsecResult {
            TsecResult_Success = 0xB0B0B0B0,
            TsecResult_Failure = 0xD0D0D0D0,
        };

        bool RunFirmwareImpl(const void *fw, size_t fw_size) {
            /* Enable relevant clocks. */
            clkrst::EnableHost1xClock();
            clkrst::EnableTsecClock();
            clkrst::EnableSorSafeClock();
            clkrst::EnableSor0Clock();
            clkrst::EnableSor1Clock();
            clkrst::EnableKfuseClock();

            /* Disable clocks once we're done. */
            ON_SCOPE_EXIT {
                clkrst::DisableHost1xClock();
                clkrst::DisableTsecClock();
                clkrst::DisableSorSafeClock();
                clkrst::DisableSor0Clock();
                clkrst::DisableSor1Clock();
                clkrst::DisableKfuseClock();
            };

            /* TODO */
            AMS_UNUSED(fw, fw_size);
            return true;
        }

    }

    bool RunTsecFirmware(const void *fw, size_t fw_size) {
        /* TODO */
        AMS_UNUSED(fw, fw_size);
        return RunFirmwareImpl(fw, fw_size);
    }

    void Lock() {
        /* Set the tsec host1x syncpoint (160) to be secure. */
        /* TODO: constexpr value. */
        reg::ReadWrite(0x500038F8, REG_BITS_VALUE(0, 1, 0));

        /* Clear the tsec host1x syncpoint. */
        reg::Write(0x50003300, 0);
    }

}