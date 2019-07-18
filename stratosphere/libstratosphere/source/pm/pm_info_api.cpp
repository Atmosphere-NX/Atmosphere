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
#include <set>
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/pm.hpp>

#include "pm_ams.h"

namespace sts::pm::info {

    namespace {

        /* Global lock. */
        HosMutex g_info_lock;
        std::set<u64> g_cached_launched_titles;

    }

    /* Information API. */
    Result GetTitleId(ncm::TitleId *out_title_id, u64 process_id) {
        std::scoped_lock<HosMutex> lk(g_info_lock);

        return pminfoGetTitleId(reinterpret_cast<u64 *>(out_title_id), process_id);
    }

    Result GetProcessId(u64 *out_process_id, ncm::TitleId title_id) {
        std::scoped_lock<HosMutex> lk(g_info_lock);

        return pminfoAtmosphereGetProcessId(out_process_id, static_cast<u64>(title_id));
    }

    Result WEAK HasLaunchedTitle(bool *out, ncm::TitleId title_id) {
        std::scoped_lock<HosMutex> lk(g_info_lock);

        if (g_cached_launched_titles.find(static_cast<u64>(title_id)) != g_cached_launched_titles.end()) {
            *out = true;
            return ResultSuccess;
        }

        bool has_launched = false;
        R_TRY(pminfoAtmosphereHasLaunchedTitle(&has_launched, static_cast<u64>(title_id)));
        if (has_launched) {
            g_cached_launched_titles.insert(static_cast<u64>(title_id));
        }

        *out = has_launched;
        return ResultSuccess;
    }

    bool HasLaunchedTitle(ncm::TitleId title_id) {
        bool has_launched = false;
        R_ASSERT(HasLaunchedTitle(&has_launched, title_id));
        return has_launched;
    }

}
