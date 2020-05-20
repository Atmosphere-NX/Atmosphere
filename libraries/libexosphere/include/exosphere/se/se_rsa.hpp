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
#include <vapours.hpp>
#include <exosphere/se/se_common.hpp>

namespace ams::se {

    constexpr inline int RsaKeySlotCount = 2;
    constexpr inline int RsaSize         = 0x100;

    void ClearRsaKeySlot(int slot);
    void LockRsaKeySlot(int slot, u32 flags);

    void SetRsaKey(int slot, const void *mod, size_t mod_size, const void *exp, size_t exp_size);

    void ModularExponentiate(void *dst, size_t dst_size, int slot, const void *src, size_t src_size);
    void ModularExponentiateAsync(int slot, const void *src, size_t src_size, DoneHandler handler);

    void GetRsaResult(void *dst, size_t dst_size);

}
