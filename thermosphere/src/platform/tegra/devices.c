/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include "devices.h"
#include "../../memory_map.h"
#include "../../utils.h"

#include "uart.h"
#include "car.h"
#include "gpio.h"
#include "pinmux.h"

void devicesMapAllExtra(void)
{
    uartSetRegisterBase(memoryMapPlatformMmio(MEMORY_MAP_PA_UART, 0x1000));

    car_set_regs(memoryMapPlatformMmio(MEMORY_MAP_PA_CAR, 0x1000));
    gpio_set_regs(memoryMapPlatformMmio(MEMORY_MAP_PA_GPIO, 0x1000));
    pinmux_set_regs(memoryMapPlatformMmio(MEMORY_MAP_PA_PINMUX, 0x1000));

    // Don't broadcast, since it's only ran once per boot by only one core, before the others are started...
    __tlb_invalidate_el2_local();
    __dsb();
    __isb();
}
