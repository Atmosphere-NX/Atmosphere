/*
 * Copyright (c) Atmosphère-NX
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
#include <mesosphere/kern_common.hpp>

#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
    #include <mesosphere/board/nintendo/nx/kern_k_device_page_table.hpp>

    namespace ams::kern {
        using ams::kern::board::nintendo::nx::KDevicePageTable;
    }

#else
    #include <mesosphere/board/generic/kern_k_device_page_table.hpp>

    namespace ams::kern {
        using ams::kern::board::generic::KDevicePageTable;
    }

#endif
