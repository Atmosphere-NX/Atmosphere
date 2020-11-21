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
#include "fatal_sound.hpp"

namespace ams::secmon::fatal {

    namespace {

        constexpr inline int I2cAddressMaxAlc5639 = 0x1C;

        constexpr inline uintptr_t GPIO = secmon::MemoryRegionVirtualDeviceGpio.GetAddress();

        constexpr size_t GPIO_PORT7_CNF_1 = 0x604;
        constexpr size_t GPIO_PORT7_OE_1  = 0x614;
        constexpr size_t GPIO_PORT7_OUT_1 = 0x624;

        void WriteAlc5639Register(int r, u16 val) {
            i2c::Send(i2c::Port_1, I2cAddressMaxAlc5639, r, std::addressof(val), sizeof(val));
        }

    }

    void StopSound() {
        /* Mute output to the speaker, setting left/right volume to 0 DB. */
        WriteAlc5639Register(0x01, 0xC8C8);

        /* Mute output to headphones, setting left/right volume to 0 DB. */
        WriteAlc5639Register(0x02, 0xC8C8);

        /* Clear all Power Management Control registers by writing 0x0000 to them. */
        for (int r = 0x61; r <= 0x66; ++r) {
            WriteAlc5639Register(r, 0x0000);
        }

        /* Configure CodecLdoEn as GPIO. */
        reg::SetBits(GPIO + GPIO_PORT7_CNF_1, (1u << 4));

        /* Configure CodecLdoEn as Output. */
        reg::SetBits(GPIO + GPIO_PORT7_OE_1, (1u << 4));

        /* Wait 200 milliseconds for config to take effect. */
        util::WaitMicroSeconds(200'000ul);

        /* Pull CodecLdoEn low. */
        reg::ClearBits(GPIO + GPIO_PORT7_OUT_1, (1u << 4));
    }

}
