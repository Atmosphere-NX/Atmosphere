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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fs_istorage.hpp>

namespace ams::fs {

   enum class BisPartitionId {
       /* Boot0 */
       BootPartition1Root =  0,

       /* Boot1 */
       BootPartition2Root = 10,

       /* Non-Boot */
       UserDataRoot                     = 20,
       BootConfigAndPackage2Part1       = 21,
       BootConfigAndPackage2Part2       = 22,
       BootConfigAndPackage2Part3       = 23,
       BootConfigAndPackage2Part4       = 24,
       BootConfigAndPackage2Part5       = 25,
       BootConfigAndPackage2Part6       = 26,
       CalibrationBinary                = 27,
       CalibrationFile                  = 28,
       SafeMode                         = 29,
       User                             = 30,
       System                           = 31,
       SystemProperEncryption           = 32,
       SystemProperPartition            = 33,
       SignedSystemPartitionOnSafeMode  = 34,
   };

   const char *GetBisMountName(BisPartitionId id);

   Result MountBis(BisPartitionId id, const char *root_path);
   Result MountBis(const char *name, BisPartitionId id);

   void SetBisRootForHost(BisPartitionId id, const char *root_path);

   Result OpenBisPartition(std::unique_ptr<fs::IStorage> *out, BisPartitionId id);

   Result InvalidateBisCache();

}
