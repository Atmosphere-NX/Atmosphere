/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License
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
#include <vapours/svc/svc_types_common.hpp>

namespace ams::svc::board::nintendo::nx {

    enum DeviceName {
        DeviceName_Afi        = 0,
        DeviceName_Avpc       = 1,
        DeviceName_Dc         = 2,
        DeviceName_Dcb        = 3,
        DeviceName_Hc         = 4,
        DeviceName_Hda        = 5,
        DeviceName_Isp2       = 6,
        DeviceName_MsencNvenc = 7,
        DeviceName_Nv         = 8,
        DeviceName_Nv2        = 9,
        DeviceName_Ppcs       = 10,
        DeviceName_Sata       = 11,
        DeviceName_Vi         = 12,
        DeviceName_Vic        = 13,
        DeviceName_XusbHost   = 14,
        DeviceName_XusbDev    = 15,
        DeviceName_Tsec       = 16,
        DeviceName_Ppcs1      = 17,
        DeviceName_Dc1        = 18,
        DeviceName_Sdmmc1a    = 19,
        DeviceName_Sdmmc2a    = 20,
        DeviceName_Sdmmc3a    = 21,
        DeviceName_Sdmmc4a    = 22,
        DeviceName_Isp2b      = 23,
        DeviceName_Gpu        = 24,
        DeviceName_Gpub       = 25,
        DeviceName_Ppcs2      = 26,
        DeviceName_Nvdec      = 27,
        DeviceName_Ape        = 28,
        DeviceName_Se         = 29,
        DeviceName_Nvjpg      = 30,
        DeviceName_Hc1        = 31,
        DeviceName_Se1        = 32,
        DeviceName_Axiap      = 33,
        DeviceName_Etr        = 34,
        DeviceName_Tsecb      = 35,
        DeviceName_Tsec1      = 36,
        DeviceName_Tsecb1     = 37,
        DeviceName_Nvdec1     = 38,

        DeviceName_Count,
    };

    namespace impl {

        constexpr inline const size_t RequiredNonSecureSystemMemorySizeVi         = 0x2238 * 4_KB;
        constexpr inline const size_t RequiredNonSecureSystemMemorySizeNvservices = 0x710  * 4_KB;
        constexpr inline const size_t RequiredNonSecureSystemMemorySizeMisc       = 0x80   * 4_KB;

    }

    constexpr inline const size_t RequiredNonSecureSystemMemorySize = impl::RequiredNonSecureSystemMemorySizeVi         +
                                                                      impl::RequiredNonSecureSystemMemorySizeNvservices +
                                                                      impl::RequiredNonSecureSystemMemorySizeMisc;

}
