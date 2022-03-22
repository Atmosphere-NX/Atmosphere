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
#include <stratosphere/cfg/cfg_types.hpp>
#include <stratosphere/cfg/cfg_locale_types.hpp>
#include <stratosphere/sm/sm_types.hpp>

namespace ams::cfg {

    /* SD card configuration. */
    bool IsSdCardRequiredServicesReady();
    void WaitSdCardRequiredServicesReady();
    bool IsSdCardInitialized();
    void WaitSdCardInitialized();

    /* Override key utilities. */
    OverrideStatus CaptureOverrideStatus(ncm::ProgramId program_id);

    /* Locale utilities. */
    OverrideLocale GetOverrideLocale(ncm::ProgramId program_id);

    /* Flag utilities. */
    bool HasFlag(const sm::MitmProcessInfo &process_info, const char *flag);
    bool HasContentSpecificFlag(ncm::ProgramId program_id, const char *flag);
    bool HasGlobalFlag(const char *flag);
    Result DeleteGlobalFlag(const char *flag);

    /* HBL Configuration utilities. */
    bool HasHblFlag(const char *flag);
    const char *GetHblPath();

}
