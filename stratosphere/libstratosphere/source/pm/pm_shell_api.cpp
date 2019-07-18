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

#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/pm.hpp>

namespace sts::pm::shell {

    /* Shell API. */
    Result WEAK LaunchTitle(u64 *out_process_id, const ncm::TitleLocation &loc, u32 launch_flags) {
        return pmshellLaunchProcess(launch_flags, static_cast<u64>(loc.title_id), loc.storage_id, out_process_id);
    }

}
