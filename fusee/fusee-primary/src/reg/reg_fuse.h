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

    const struct FUSE {
        static const uintptr_t base_addr = 0x7000f800;
        using Peripheral = FUSE;

        BEGIN_DEFINE_REGISTER(SKU_USB_CALIB, uint32_t, 0x1f0)
            DEFINE_RO_FIELD(TERM_RANGE_ADJ, 7, 10) // TODO: check?
            DEFINE_RO_FIELD(HS_CURR_LEVEL, 0, 5) // TODO: check?
        END_DEFINE_REGISTER(SKU_USB_CALIB)

        BEGIN_DEFINE_REGISTER(USB_CALIB_EXT, uint32_t, 0x350)
            DEFINE_RO_FIELD(RPD_CTRL, 0, 5)
        END_DEFINE_REGISTER(USB_CALIB_EXT)

    } FUSE;
    
} // namespace t210
