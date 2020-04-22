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
#include "spl_types.hpp"

namespace ams::spl {

    HardwareType GetHardwareType();
    MemoryArrangement GetMemoryArrangement();
    bool IsDisabledProgramVerification();
    bool IsDevelopmentHardware();
    bool IsDevelopmentFunctionEnabled();
    bool IsMariko();
    bool IsRecoveryBoot();

    Result GenerateAesKek(AccessKey *access_key, const void *key_source, size_t key_source_size, u32 generation, u32 option);
    Result GenerateAesKey(void *dst, size_t dst_size, const AccessKey &access_key, const void *key_source, size_t key_source_size);

}
