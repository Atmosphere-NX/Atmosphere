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
#include "ams_types.hpp"

namespace ams::exosphere {

    ApiInfo GetApiInfo();

    void ForceRebootToRcm();
    void ForceRebootToIramPayload();
    void ForceRebootToFatalError();
    void ForceShutdown();

    bool IsRcmBugPatched();

    bool ShouldBlankProdInfo();
    bool ShouldAllowWritesToProdInfo();

    u64 GetDeviceId();

    void CopyToIram(uintptr_t iram_dst, const void *dram_src, size_t size);
    void CopyFromIram(void *dram_dst, uintptr_t iram_src, size_t size);

}

namespace ams {

    /* Version checking utility. */
    inline void CheckApiVersion() {
        const u32 runtime_version = exosphere::GetApiInfo().GetVersion();
        const u32 build_version   = exosphere::GetVersion(ATMOSPHERE_RELEASE_VERSION);

        if (runtime_version < build_version) {
            R_ABORT_UNLESS(exosphere::ResultVersionMismatch());
        }
    }

}
