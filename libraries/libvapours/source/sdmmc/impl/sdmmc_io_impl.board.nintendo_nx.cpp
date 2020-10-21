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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "sdmmc_io_impl.board.nintendo_nx.hpp"

namespace ams::sdmmc::impl {

    Result SetSdCardVoltageEnabled(bool en) {
        /* TODO */
    }

    Result SetSdCardVoltageValue(u32 micro_volts) {
        /* TODO */
    }

    namespace pinmux_impl {

        void SetPinAssignment(PinAssignment assignment) {
            /* TODO */
        }

    }

    namespace gpio_impl {

        void OpenSession(GpioPadName pad) {
            /* TODO */
        }

        void CloseSession(GpioPadName pad) {
            /* TODO */
        }

        void SetDirection(GpioPadName pad, Direction direction) {
            /* TODO */
        }

        void SetValue(GpioPadName pad, GpioValue value) {
            /* TODO */
        }


    }

}
