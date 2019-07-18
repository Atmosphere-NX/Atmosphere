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
#include "../ncm/ncm_types.hpp"

namespace sts::cfg {

    /* Privileged Process configuration. */
    bool IsInitialProcess();
    void GetInitialProcessRange(u64 *out_min, u64 *out_max);

    /* SD card configuration. */
    bool IsSdCardInitialized();
    void WaitSdCardInitialized();

    /* Override key utilities. */
    bool IsTitleOverrideKeyHeld(ncm::TitleId title_id);
    bool IsHblOverrideKeyHeld(ncm::TitleId title_id);
    void GetOverrideKeyHeldStatus(bool *out_hbl, bool *out_title, ncm::TitleId title_id);
    bool IsCheatEnableKeyHeld(ncm::TitleId title_id);

    /* Flag utilities. */
    bool HasFlag(ncm::TitleId title_id, const char *flag);
    bool HasTitleSpecificFlag(ncm::TitleId title_id, const char *flag);
    bool HasGlobalFlag(const char *flag);

    /* HBL Configuration utilities. */
    bool IsHblTitleId(ncm::TitleId title_id);
    bool HasHblFlag(const char *flag);
    const char *GetHblPath();

}
