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
#include "settings_firmware_version_impl.hpp"

namespace ams::settings::impl {

    Result GetFirmwareVersion(settings::system::FirmwareVersion *out) {
        #if defined(ATMOSPHERE_OS_HORIZON)
        static_assert(sizeof(*out) == sizeof(::SetSysFirmwareVersion));
        R_RETURN(::setsysGetFirmwareVersion(reinterpret_cast<::SetSysFirmwareVersion *>(out)));
        #else
        AMS_UNUSED(out);
        AMS_ABORT("TODO");
        #endif
    }

}
