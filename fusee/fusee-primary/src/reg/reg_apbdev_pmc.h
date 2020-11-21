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

#include "reg_util.h"

namespace t210 {

    const struct APBDEV_PMC {
        static const uintptr_t base_addr = 0x7000e400;
        using Peripheral = APBDEV_PMC;

        BEGIN_DEFINE_REGISTER(USB_AO_0, uint32_t, 0xf0)
            DEFINE_RW_FIELD(ID_PD_P0, 3)
            DEFINE_RW_FIELD(VBUS_WAKEUP_PD_P0, 2)
        END_DEFINE_REGISTER(USB_AO_0)
    } APBDEV_PMC;
    
} // namespace t210
