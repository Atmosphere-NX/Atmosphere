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
#include <stratosphere/gpio/gpio_types.hpp>
#include <stratosphere/gpio/driver/gpio_i_gpio_driver.hpp>
#include <stratosphere/gpio/driver/gpio_driver_service_api.hpp>
#include <stratosphere/gpio/driver/gpio_driver_client_api.hpp>

#if defined(ATMOSPHERE_BOARD_NINTENDO_NX)

    #include <stratosphere/gpio/driver/board/nintendo/nx/gpio_driver_api.hpp>

    namespace ams::gpio::driver::board {

        using namespace ams::gpio::driver::board::nintendo::nx;

    }

#else

    #error "Unknown board for ams::gpio::driver::"

#endif

