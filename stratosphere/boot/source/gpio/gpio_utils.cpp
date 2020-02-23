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

#include <stratosphere/reg.hpp>

#include "gpio_utils.hpp"

namespace ams::gpio {

    namespace {

        /* Pull in GPIO map definitions. */
#include "gpio_map.inc"

        constexpr u32 PhysicalBase = 0x6000D000;

        /* Globals. */
        bool g_initialized_gpio_vaddr = false;
        uintptr_t g_gpio_vaddr = 0;

        /* Helpers. */
        inline u32 GetPadDescriptor(u32 gpio_pad_name) {
            AMS_ABORT_UNLESS(gpio_pad_name < PadNameMax);
            return Map[gpio_pad_name];
        }

        uintptr_t GetBaseAddress() {
            if (!g_initialized_gpio_vaddr) {
                g_gpio_vaddr = dd::GetIoMapping(PhysicalBase, os::MemoryPageSize);
                g_initialized_gpio_vaddr = true;
            }
            return g_gpio_vaddr;
        }


    }

    u32 Configure(u32 gpio_pad_name) {
        uintptr_t gpio_base_vaddr = GetBaseAddress();

        /* Fetch this GPIO's pad descriptor */
        const u32 gpio_pad_desc = GetPadDescriptor(gpio_pad_name);

        /* Discard invalid GPIOs */
        if (gpio_pad_desc == InvalidPadName) {
            return InvalidPadName;
        }

        /* Convert the GPIO pad descriptor into its register offset */
        u32 gpio_reg_offset = (((gpio_pad_desc << 0x03) & 0xFFFFFF00) | ((gpio_pad_desc >> 0x01) & 0x0C));

        /* Extract the bit and lock values from the GPIO pad descriptor */
        u32 gpio_cnf_val = ((0x01 << ((gpio_pad_desc & 0x07) | 0x08)) | (0x01 << (gpio_pad_desc & 0x07)));

        /* Write to the appropriate GPIO_CNF_x register (upper offset) */
        reg::Write(gpio_base_vaddr + gpio_reg_offset + 0x80, gpio_cnf_val);

        /* Do a dummy read from GPIO_CNF_x register (lower offset) */
        gpio_cnf_val = reg::Read(gpio_base_vaddr + gpio_reg_offset + 0x00);

        return gpio_cnf_val;
    }

    u32 SetDirection(u32 gpio_pad_name, GpioDirection dir) {
        uintptr_t gpio_base_vaddr = GetBaseAddress();

        /* Fetch this GPIO's pad descriptor */
        const u32 gpio_pad_desc = GetPadDescriptor(gpio_pad_name);

        /* Discard invalid GPIOs */
        if (gpio_pad_desc == InvalidPadName) {
            return InvalidPadName;
        }

        /* Convert the GPIO pad descriptor into its register offset */
        u32 gpio_reg_offset = (((gpio_pad_desc << 0x03) & 0xFFFFFF00) | ((gpio_pad_desc >> 0x01) & 0x0C));

        /* Set the direction bit and lock values */
        u32 gpio_oe_val = ((0x01 << ((gpio_pad_desc & 0x07) | 0x08)) | (static_cast<u32>(dir) << (gpio_pad_desc & 0x07)));

        /* Write to the appropriate GPIO_OE_x register (upper offset) */
        reg::Write(gpio_base_vaddr + gpio_reg_offset + 0x90, gpio_oe_val);

        /* Do a dummy read from GPIO_OE_x register (lower offset) */
        gpio_oe_val = reg::Read(gpio_base_vaddr + gpio_reg_offset + 0x10);

        return gpio_oe_val;
    }

    u32 SetValue(u32 gpio_pad_name, GpioValue val) {
        uintptr_t gpio_base_vaddr = GetBaseAddress();

        /* Fetch this GPIO's pad descriptor */
        const u32 gpio_pad_desc = GetPadDescriptor(gpio_pad_name);

        /* Discard invalid GPIOs */
        if (gpio_pad_desc == InvalidPadName) {
            return InvalidPadName;
        }

        /* Convert the GPIO pad descriptor into its register offset */
        u32 gpio_reg_offset = (((gpio_pad_desc << 0x03) & 0xFFFFFF00) | ((gpio_pad_desc >> 0x01) & 0x0C));

        /* Set the output bit and lock values */
        u32 gpio_out_val = ((0x01 << ((gpio_pad_desc & 0x07) | 0x08)) | (static_cast<u32>(val) << (gpio_pad_desc & 0x07)));

        /* Write to the appropriate GPIO_OUT_x register (upper offset) */
        reg::Write(gpio_base_vaddr + gpio_reg_offset + 0xA0, gpio_out_val);

        /* Do a dummy read from GPIO_OUT_x register (lower offset) */
        gpio_out_val = reg::Read(gpio_base_vaddr + gpio_reg_offset + 0x20);

        return gpio_out_val;
    }

}
