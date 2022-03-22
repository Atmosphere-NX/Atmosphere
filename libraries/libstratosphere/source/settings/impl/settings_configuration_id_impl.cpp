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
#include "settings_serial_number_impl.hpp"

namespace ams::settings::impl {

    Result GetConfigurationId1(settings::factory::ConfigurationId1 *out) {
        static_assert(sizeof(*out) == sizeof(::SetCalConfigurationId1));
        return ::setcalGetConfigurationId1(reinterpret_cast<::SetCalConfigurationId1 *>(out));
    }

}
