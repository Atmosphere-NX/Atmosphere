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
#include <stratosphere/pm.hpp>

#include "ldr_launch_record.hpp"

namespace sts::ldr {

    namespace {

        /* Global cache. */
        std::set<u64> g_launched_titles;

    }

    /* Launch Record API. */
    bool HasLaunchedTitle(ncm::TitleId title_id) {
        return g_launched_titles.find(static_cast<u64>(title_id)) != g_launched_titles.end();
    }

    void SetLaunchedTitle(ncm::TitleId title_id) {
        g_launched_titles.insert(static_cast<u64>(title_id));
    }

}

/* Loader wants to override this libstratosphere function, which is weakly linked. */
/* This is necessary to prevent circular dependencies. */
namespace sts::pm::info {

    Result HasLaunchedTitle(bool *out, ncm::TitleId title_id) {
        *out = ldr::HasLaunchedTitle(title_id);
        return ResultSuccess;
    }

}
