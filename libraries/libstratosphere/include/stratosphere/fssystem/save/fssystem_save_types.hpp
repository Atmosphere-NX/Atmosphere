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
#include <stratosphere/lmem.hpp>
#include <stratosphere/fs/fs_directory.hpp>
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/fssystem/dbm/fssystem_dbm_utils.hpp>

namespace ams::fssystem::save {

   constexpr inline bool IsPowerOfTwo(s32 val) {
       return util::IsPowerOfTwo(val);
   }

   constexpr inline u32 ILog2(u32 val) {
       AMS_ASSERT(val > 0);
       return (BITSIZEOF(u32) - 1 - dbm::CountLeadingZeros(val));
   }

   constexpr inline u32 CeilPowerOfTwo(u32 val) {
       if (val == 0) {
           return 1;
       }
       return ((1u << (BITSIZEOF(u32) - 1)) >> (dbm::CountLeadingZeros(val - 1) - 1));
   }

}
