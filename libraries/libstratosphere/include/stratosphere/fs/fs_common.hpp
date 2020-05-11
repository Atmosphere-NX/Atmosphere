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
#include <stratosphere/os.hpp>
#include <stratosphere/ncm/ncm_ids.hpp>
#include <stratosphere/sf.hpp>

namespace ams::fs {

    struct Int64 {
        u32 low;
        u32 high;

        constexpr ALWAYS_INLINE void Set(s64 v) {
            this->low  = static_cast<u32>((v & static_cast<u64>(0x00000000FFFFFFFFul)) >>  0);
            this->high = static_cast<u32>((v & static_cast<u64>(0xFFFFFFFF00000000ul)) >> 32);
        }

        constexpr ALWAYS_INLINE s64 Get() const {
            return (static_cast<s64>(this->high) << 32) | (static_cast<s64>(this->low));
        }

        constexpr ALWAYS_INLINE Int64 &operator=(s64 v) {
            this->Set(v);
            return *this;
        }

        constexpr ALWAYS_INLINE operator s64() const {
            return this->Get();
        }
    };
    static_assert(util::is_pod<Int64>::value);

}