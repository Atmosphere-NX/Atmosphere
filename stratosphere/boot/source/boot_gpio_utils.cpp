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

#include "boot_functions.hpp"
#include "boot_gpio_map.hpp"

static bool g_initialized_gpio_vaddr = false;
static uintptr_t g_gpio_vaddr = 0;

static inline u32 GetGpioPadDescriptor(u32 gpio_pad_name) {
    if (gpio_pad_name >= GpioPadNameMax) {
        std::abort();
    }

    return GpioMap[gpio_pad_name];
}

static uintptr_t GetGpioBaseAddress() {
    if (!g_initialized_gpio_vaddr) {
        u64 vaddr;
        if (R_FAILED(svcQueryIoMapping(&vaddr, Boot::GpioPhysicalBase, 0x1000))) {
            std::abort();
        }
        g_gpio_vaddr = vaddr;
        g_initialized_gpio_vaddr = true;
    }
    return g_gpio_vaddr;
}

u32 Boot::GpioConfigure(u32 gpio_pad_name) {
    uintptr_t gpio_base_vaddr = GetGpioBaseAddress();

    /* Fetch this GPIO's pad descriptor */
    const u32 gpio_pad_desc = GetGpioPadDescriptor(gpio_pad_name);

    /* Discard invalid GPIOs */
    if (gpio_pad_desc == GpioInvalid) {
        return GpioInvalid;
    }

    /* Convert the GPIO pad descriptor into its register offset */
    u32 gpio_reg_offset = (((gpio_pad_desc << 0x03) & 0xFFFFFF00) | ((gpio_pad_desc >> 0x01) & 0x0C));

    /* Extract the bit and lock values from the GPIO pad descriptor */
    u32 gpio_cnf_val = ((0x01 << ((gpio_pad_desc & 0x07) | 0x08)) | (0x01 << (gpio_pad_desc & 0x07)));

    /* Write to the appropriate GPIO_CNF_x register (upper offset) */
    *(reinterpret_cast<volatile u32 *>(gpio_base_vaddr + gpio_reg_offset + 0x80)) = gpio_cnf_val;

    /* Do a dummy read from GPIO_CNF_x register (lower offset) */
    gpio_cnf_val = *(reinterpret_cast<volatile u32 *>(gpio_base_vaddr + gpio_reg_offset));

    return gpio_cnf_val;
}

u32 Boot::GpioSetDirection(u32 gpio_pad_name, GpioDirection dir) {
    uintptr_t gpio_base_vaddr = GetGpioBaseAddress();

    /* Fetch this GPIO's pad descriptor */
    const u32 gpio_pad_desc = GetGpioPadDescriptor(gpio_pad_name);

    /* Discard invalid GPIOs */
    if (gpio_pad_desc == GpioInvalid) {
        return GpioInvalid;
    }

    /* Convert the GPIO pad descriptor into its register offset */
    u32 gpio_reg_offset = (((gpio_pad_desc << 0x03) & 0xFFFFFF00) | ((gpio_pad_desc >> 0x01) & 0x0C));

    /* Set the direction bit and lock values */
    u32 gpio_oe_val = ((0x01 << ((gpio_pad_desc & 0x07) | 0x08)) | (static_cast<u32>(dir) << (gpio_pad_desc & 0x07)));

    /* Write to the appropriate GPIO_OE_x register (upper offset) */
    *(reinterpret_cast<volatile u32 *>(gpio_base_vaddr + gpio_reg_offset + 0x90)) = gpio_oe_val;

    /* Do a dummy read from GPIO_OE_x register (lower offset) */
    gpio_oe_val = *(reinterpret_cast<volatile u32 *>(gpio_base_vaddr + gpio_reg_offset + 0x10));

    return gpio_oe_val;
}

u32 Boot::GpioSetValue(u32 gpio_pad_name, GpioValue val) {
    uintptr_t gpio_base_vaddr = GetGpioBaseAddress();

    /* Fetch this GPIO's pad descriptor */
    const u32 gpio_pad_desc = GetGpioPadDescriptor(gpio_pad_name);

    /* Discard invalid GPIOs */
    if (gpio_pad_desc == GpioInvalid) {
        return GpioInvalid;
    }

    /* Convert the GPIO pad descriptor into its register offset */
    u32 gpio_reg_offset = (((gpio_pad_desc << 0x03) & 0xFFFFFF00) | ((gpio_pad_desc >> 0x01) & 0x0C));

    /* Set the output bit and lock values */
    u32 gpio_out_val = ((0x01 << ((gpio_pad_desc & 0x07) | 0x08)) | (static_cast<u32>(val) << (gpio_pad_desc & 0x07)));

    /* Write to the appropriate GPIO_OUT_x register (upper offset) */
    *(reinterpret_cast<volatile u32 *>(gpio_base_vaddr + gpio_reg_offset + 0xA0)) = gpio_out_val;

    /* Do a dummy read from GPIO_OUT_x register (lower offset) */
    gpio_out_val = *(reinterpret_cast<volatile u32 *>(gpio_base_vaddr + gpio_reg_offset + 0x20));

    return gpio_out_val;
}
