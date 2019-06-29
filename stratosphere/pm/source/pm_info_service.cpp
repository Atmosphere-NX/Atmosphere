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

#include <stratosphere/ldr/ldr_pm_api.hpp>
#include <stratosphere/pm.hpp>

#include "pm_info_service.hpp"
#include "impl/pm_process_manager.hpp"

namespace sts::pm::info {

    /* Overrides for libstratosphere pm::info commands. */
    Result HasLaunchedTitle(bool *out, ncm::TitleId title_id) {
        return ldr::pm::HasLaunchedTitle(out, title_id);
    }

    /* Actual command implementations. */
    Result InformationService::GetTitleId(Out<ncm::TitleId> out, u64 process_id) {
        return impl::GetTitleId(out.GetPointer(), process_id);
    }

    /* Atmosphere extension commands. */
    Result InformationService::AtmosphereGetProcessId(Out<u64> out, ncm::TitleId title_id) {
        return impl::GetProcessId(out.GetPointer(), title_id);
    }

    Result InformationService::AtmosphereHasLaunchedTitle(Out<bool> out, ncm::TitleId title_id) {
        return pm::info::HasLaunchedTitle(out.GetPointer(), title_id);
    }

}
