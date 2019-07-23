/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
 
#include <stdint.h>

#include "gpio.h"
#include "../../utils.h"

static volatile tegra_gpio_bank_t *gpio_get_bank(uint32_t pin) {
    volatile tegra_gpio_t *gpio = gpio_get_regs();
    uint32_t bank_number = (pin >> GPIO_BANK_SHIFT);

    return &gpio->bank[bank_number];
}

static volatile uint32_t gpio_get_port(uint32_t pin) {
    return ((pin >> GPIO_PORT_SHIFT) & GPIO_PORT_MASK);
}

static volatile uint32_t gpio_get_mask(uint32_t pin) {
    uint32_t pin_number = (pin & GPIO_PIN_MASK);
    return (1 << pin_number);
}

static void gpio_simple_register_set(uint32_t pin, bool should_be_set, uint32_t offset) {
    /* Retrieve the register set that corresponds to the given pin and offset. */
    uintptr_t cluster_addr = (uintptr_t)gpio_get_bank(pin) + offset;
    uint32_t *cluster = (uint32_t *)cluster_addr;

    /* Figure out the offset into the cluster, and the mask to be used. */
    uint32_t port = gpio_get_port(pin);
    uint32_t mask = gpio_get_mask(pin);

    /* Set or clear the bit, as appropriate. */
    if (should_be_set)
        cluster[port] |= mask;
    else
        cluster[port] &= ~mask;
    
    /* Dummy read. */
    (void)cluster[port];
}

static bool gpio_simple_register_get(uint32_t pin, uint32_t offset) {
    /* Retrieve the register set that corresponds to the given pin and offset. */
    uintptr_t cluster_addr = (uintptr_t)gpio_get_bank(pin) + offset;
    uint32_t *cluster = (uint32_t *)cluster_addr;

    /* Figure out the offset into the cluster, and the mask to be used. */
    uint32_t port = gpio_get_port(pin);
    uint32_t mask = gpio_get_mask(pin);

    /* Convert the given value to a boolean. */
    return !!(cluster[port] & mask);
}

void gpio_configure_mode(uint32_t pin, uint32_t mode) {
    gpio_simple_register_set(pin, mode == GPIO_MODE_GPIO, offsetof(tegra_gpio_bank_t, config));
}

void gpio_configure_direction(uint32_t pin, uint32_t dir) {
    gpio_simple_register_set(pin, dir == GPIO_DIRECTION_OUTPUT, offsetof(tegra_gpio_bank_t, direction)); 
}

void gpio_write(uint32_t pin, uint32_t value) {
    gpio_simple_register_set(pin, value == GPIO_LEVEL_HIGH, offsetof(tegra_gpio_bank_t, out)); 
}

uint32_t gpio_read(uint32_t pin) {
    return gpio_simple_register_get(pin, offsetof(tegra_gpio_bank_t, in)); 
}
