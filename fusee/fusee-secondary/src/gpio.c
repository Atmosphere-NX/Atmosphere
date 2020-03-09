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
 
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "gpio.h"
#include "utils.h"

/* Set GPIO's value. */
static void gpio_register_set(uint32_t pin, bool do_set, uint32_t offset) {
    volatile tegra_gpio_t *gpio = gpio_get_regs();
    
    /* Retrieve the register set that corresponds to the given pin and offset. */
    volatile uint32_t *cluster = (uint32_t *)((uintptr_t)&gpio->bank[(pin >> GPIO_BANK_SHIFT)] + offset);

    /* Figure out the offset into the cluster, and the mask to be used. */
    uint32_t port = ((pin >> GPIO_PORT_SHIFT) & GPIO_PORT_MASK);
    uint32_t mask = (1 << (pin & GPIO_PIN_MASK));

    /* Set or clear the bit, as appropriate. */
    if (do_set)
        cluster[port] |= mask;
    else
        cluster[port] &= ~mask;
    
    /* Dummy read. */
    cluster[port];
}

/* Get GPIO's value. */
static bool gpio_register_get(uint32_t pin, uint32_t offset) {
    volatile tegra_gpio_t *gpio = gpio_get_regs();
    
    /* Retrieve the register set that corresponds to the given pin and offset. */
    volatile uint32_t *cluster = (uint32_t *)((uintptr_t)&gpio->bank[(pin >> GPIO_BANK_SHIFT)] + offset);

    /* Figure out the offset into the cluster, and the mask to be used. */
    uint32_t port = ((pin >> GPIO_PORT_SHIFT) & GPIO_PORT_MASK);
    uint32_t mask = (1 << (pin & GPIO_PIN_MASK));

    /* Convert the given value to a boolean. */
    return !!(cluster[port] & mask);
}

/* Configure GPIO's mode. */
void gpio_configure_mode(uint32_t pin, uint32_t mode) {
    gpio_register_set(pin, mode == GPIO_MODE_GPIO, offsetof(tegra_gpio_bank_t, config));
}

/* Configure GPIO's direction. */
void gpio_configure_direction(uint32_t pin, uint32_t dir) {
    gpio_register_set(pin, dir == GPIO_DIRECTION_OUTPUT, offsetof(tegra_gpio_bank_t, direction)); 
}

/* Write to GPIO. */
void gpio_write(uint32_t pin, uint32_t value) {
    gpio_register_set(pin, value == GPIO_LEVEL_HIGH, offsetof(tegra_gpio_bank_t, out)); 
}

/* Read from GPIO. */
uint32_t gpio_read(uint32_t pin) {
    return gpio_register_get(pin, offsetof(tegra_gpio_bank_t, in)); 
}
