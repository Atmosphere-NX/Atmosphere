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
#include <stratosphere.hpp>

namespace ams::ldr {

    Result TestCapability(const util::BitPack32 *kacd, size_t kacd_count, const util::BitPack32 *kac, size_t kac_count);

    u16 MakeProgramInfoFlag(const util::BitPack32 *kac, size_t count);
    void UpdateProgramInfoFlag(u16 flags, util::BitPack32 *kac, size_t count);

    void PreProcessCapability(util::BitPack32 *kac, size_t count);

}
