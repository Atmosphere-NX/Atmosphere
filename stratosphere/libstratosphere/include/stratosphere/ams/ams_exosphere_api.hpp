/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

namespace sts::ams {

    ApiInfo GetApiInfo();

    void ForceRebootToRcm();
    void ForceRebootToIramPayload();
    void ForceShutdown();

    bool IsRcmBugPatched();

    void CopyToIram(uintptr_t iram_dst, const void *dram_src, size_t size);
    void CopyFromIram(void *dram_dst, uintptr_t iram_src, size_t size);

    /* Version checking utility. */
#ifdef ATMOSPHERE_RELEASE_VERSION_MAJOR

#define ATMOSPHERE_RELEASE_VERSION ATMOSPHERE_RELEASE_VERSION_MAJOR, ATMOSPHERE_RELEASE_VERSION_MINOR, ATMOSPHERE_RELEASE_VERSION_MICRO

    inline void CheckApiVersion() {
        const u32 runtime_version = GetApiInfo().GetVersion();
        const u32 build_version = GetVersion(ATMOSPHERE_RELEASE_VERSION);

        if (runtime_version < build_version) {
            R_ASSERT(ResultAtmosphereVersionMismatch);
        }
    }

#endif

#ifdef ATMOSPHERE_GIT_BRANCH
    NX_CONSTEXPR const char *GetGitBranch() {
        return ATMOSPHERE_GIT_BRANCH;
    }
#endif

#ifdef ATMOSPHERE_GIT_REV
    NX_CONSTEXPR const char *GetGitRevision() {
        return ATMOSPHERE_GIT_REV;
    }
#endif

}
