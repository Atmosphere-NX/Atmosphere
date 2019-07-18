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

#include "pm_types.hpp"
#include "../ncm/ncm_types.hpp"

namespace sts::pm::info {

    /* Information API. */
    Result GetTitleId(ncm::TitleId *out_title_id, u64 process_id);
    Result GetProcessId(u64 *out_process_id, ncm::TitleId title_id);
    Result HasLaunchedTitle(bool *out, ncm::TitleId title_id);

    /* Information convenience API. */
    bool HasLaunchedTitle(ncm::TitleId title_id);

}
