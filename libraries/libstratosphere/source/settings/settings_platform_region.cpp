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
#include <stratosphere.hpp>
#include "impl/settings_platform_region_impl.hpp"

namespace ams::settings::system {

    PlatformRegion GetPlatformRegion() {
        if (hos::GetVersion() >= hos::Version_9_0_0) {
            s32 region = 0;
            R_ABORT_UNLESS(settings::impl::GetPlatformRegion(std::addressof(region)));
            return static_cast<PlatformRegion>(region);
        } else {
            return PlatformRegion_Global;
        }
    }

}
