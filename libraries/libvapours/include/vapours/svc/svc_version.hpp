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
#include <vapours/svc/svc_common.hpp>
#include <vapours/svc/svc_select_hardware_constants.hpp>
#include <vapours/util/util_bitpack.hpp>

namespace ams::svc {

    constexpr inline u32 ConvertToSvcMajorVersion(u32 sdk) { return sdk + 4; }
    constexpr inline u32 ConvertToSdkMajorVersion(u32 svc) { return svc - 4; }

    constexpr inline u32 ConvertToSvcMinorVersion(u32 sdk) { return sdk; }
    constexpr inline u32 ConvertToSdkMinorVersion(u32 svc) { return svc; }

    struct KernelVersion {
        using MinorVersion = util::BitPack32::Field<0, 4>;
        using MajorVersion = util::BitPack32::Field<MinorVersion::Next, 13>;
    };

    constexpr inline u32 EncodeKernelVersion(u32 major, u32 minor) {
        util::BitPack32 pack = {};
        pack.Set<KernelVersion::MinorVersion>(minor);
        pack.Set<KernelVersion::MajorVersion>(major);
        return pack.value;
    }

    constexpr inline u32 GetKernelMajorVersion(u32 encoded) {
        const util::BitPack32 pack = { encoded };
        return pack.Get<KernelVersion::MajorVersion>();
    }

    constexpr inline u32 GetKernelMinorVersion(u32 encoded) {
        const util::BitPack32 pack = { encoded };
        return pack.Get<KernelVersion::MinorVersion>();
    }

    /* Nintendo doesn't support programs targeting SVC versions < 3.0. */
    constexpr inline u32 RequiredKernelMajorVersion = 3;
    constexpr inline u32 RequiredKernelMinorVersion = 0;

    constexpr inline u32 RequiredKernelVersion = EncodeKernelVersion(RequiredKernelMajorVersion, RequiredKernelMinorVersion);

    /* This is the highest SVC version supported by Atmosphere, to be updated on new kernel releases. */
    /* NOTE: Official kernel versions have SVC major = SDK major + 4, SVC minor = SDK minor. */
    constexpr inline u32 SupportedKernelMajorVersion = ConvertToSvcMajorVersion(20);
    constexpr inline u32 SupportedKernelMinorVersion = ConvertToSvcMinorVersion( 5);

    constexpr inline u32 SupportedKernelVersion = EncodeKernelVersion(SupportedKernelMajorVersion, SupportedKernelMinorVersion);

}
