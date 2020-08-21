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
#include <stratosphere.hpp>

namespace ams::ldr::caps {

    /* Capabilities API. */
    Result ValidateCapabilities(const void *acid_kac, size_t acid_kac_size, const void *aci_kac, size_t aci_kac_size);
    u16    GetProgramInfoFlags(const void *kac, size_t kac_size);
    void   SetProgramInfoFlags(u16 flags, void *kac, size_t kac_size);

    void   ProcessCapabilities(void *kac, size_t kac_size);

}
