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
#include <switch.h>

#include "results.hpp"

static inline void GetAtmosphereApiVersion(u32 *major, u32 *minor, u32 *micro, u32 *target_fw, u32 *mkey_rev) {
    /* Check for exosphere API compatibility. */
    u64 exosphere_cfg;
    if (R_FAILED(SmcGetConfig((SplConfigItem)65000, &exosphere_cfg))) {
        fatalSimple(ResultAtmosphereExosphereNotPresent);
    }

    if (mkey_rev) {
        *mkey_rev  = (u32)((exosphere_cfg >> 0x00) & 0xFF);
    }

    if (target_fw) {
        *target_fw = (u32)((exosphere_cfg >> 0x08) & 0xFF);
    }

    if (micro) {
        *micro     = (u32)((exosphere_cfg >> 0x10) & 0xFF);
    }

    if (minor) {
        *minor     = (u32)((exosphere_cfg >> 0x18) & 0xFF);
    }

    if (major) {
        *major     = (u32)((exosphere_cfg >> 0x20) & 0xFF);
    }
}

static inline u32 MakeAtmosphereVersion(u32 major, u32 minor, u32 micro) {
    return (major << 16) | (minor << 8) | micro;
}

static inline void CheckAtmosphereVersion(u32 expected_major, u32 expected_minor, u32 expected_micro) {
    u32 major, minor, micro;
    GetAtmosphereApiVersion(&major, &minor, &micro, nullptr, nullptr);

    if (MakeAtmosphereVersion(major, minor, micro) < MakeAtmosphereVersion(expected_major, expected_minor, expected_micro)) {
        fatalSimple(ResultAtmosphereVersionMismatch);
    }
}

#define CURRENT_ATMOSPHERE_VERSION ATMOSPHERE_RELEASE_VERSION_MAJOR, ATMOSPHERE_RELEASE_VERSION_MINOR, ATMOSPHERE_RELEASE_VERSION_MICRO

#ifdef ATMOSPHERE_GIT_BRANCH
static inline const char *GetAtmosphereGitBranch() {
    return ATMOSPHERE_GIT_BRANCH;
}
#endif

#ifdef ATMOSPHERE_GIT_REV
static inline const char *GetAtmosphereGitRevision() {
    return ATMOSPHERE_GIT_REV;
}
#endif
