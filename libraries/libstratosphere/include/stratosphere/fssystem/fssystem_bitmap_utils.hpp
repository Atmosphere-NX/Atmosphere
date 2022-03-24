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
#include <vapours.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */

    constexpr inline s32 CountLeadingZeros(u32 val) {
        return util::CountLeadingZeros(val);
    }

    constexpr inline s32 CountLeadingOnes(u32 val) {
        return CountLeadingZeros(~val);
    }

    inline u32 ReadU32(const u8 *buf, size_t index) {
        u32 val;
        std::memcpy(std::addressof(val), buf + index, sizeof(u32));
        return val;
    }

    inline void WriteU32(u8 *buf, size_t index, u32 val) {
        std::memcpy(buf + index, std::addressof(val), sizeof(u32));
    }

   constexpr inline bool IsPowerOfTwo(s32 val) {
       return util::IsPowerOfTwo(val);
   }

   constexpr inline u32 ILog2(u32 val) {
       AMS_ASSERT(val > 0);
       return (BITSIZEOF(u32) - 1 - util::CountLeadingZeros<u32>(val));
   }

   constexpr inline u32 CeilingPowerOfTwo(u32 val) {
       if (val == 0) {
           return 1;
       }

       return util::CeilingPowerOfTwo<u32>(val);
   }

}
