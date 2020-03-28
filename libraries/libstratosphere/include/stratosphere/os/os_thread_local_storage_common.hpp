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
#include <stratosphere/os/os_common_types.hpp>
#include <stratosphere/os/os_memory_common.hpp>

namespace ams::os {

    struct TlsSlot {
        u32 _value;
    };

    using TlsDestructor = void (*)(uintptr_t arg);

    constexpr inline size_t TlsSlotCountMax = 16;
    constexpr inline size_t SdkTlsSlotCountMax = 16;

}
