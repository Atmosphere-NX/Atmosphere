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
#include "settings_error_report_impl.hpp"

namespace ams::settings::impl {

    Result GetErrorReportSharePermission(s32 *out) {
        static_assert(sizeof(*out) == sizeof(::SetSysErrorReportSharePermission));
        return ::setsysGetErrorReportSharePermission(reinterpret_cast<::SetSysErrorReportSharePermission *>(out));
    }

}
