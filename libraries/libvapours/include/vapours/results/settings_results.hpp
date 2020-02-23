/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <vapours/results/results_common.hpp>

namespace ams::settings {

    R_DEFINE_NAMESPACE_RESULT_MODULE(105);

    R_DEFINE_ERROR_RESULT(SettingsItemNotFound,                 11);

    R_DEFINE_ERROR_RANGE(InternalError, 100, 149);
        R_DEFINE_ERROR_RESULT(SettingsItemKeyAllocationFailed,   101);
        R_DEFINE_ERROR_RESULT(SettingsItemValueAllocationFailed, 102);

    R_DEFINE_ERROR_RANGE(InvalidArgument, 200, 399);
        R_DEFINE_ERROR_RESULT(SettingsNameNull,                 201);
        R_DEFINE_ERROR_RESULT(SettingsItemKeyNull,              202);
        R_DEFINE_ERROR_RESULT(SettingsItemValueNull,            203);
        R_DEFINE_ERROR_RESULT(SettingsItemKeyBufferNull,        204);
        R_DEFINE_ERROR_RESULT(SettingsItemValueBufferNull,      205);

        R_DEFINE_ERROR_RESULT(SettingsNameEmpty,                221);
        R_DEFINE_ERROR_RESULT(SettingsItemKeyEmpty,             222);

        R_DEFINE_ERROR_RESULT(SettingsNameTooLong,              241);
        R_DEFINE_ERROR_RESULT(SettingsItemKeyTooLong,           242);

        R_DEFINE_ERROR_RESULT(SettingsNameInvalidFormat,        261);
        R_DEFINE_ERROR_RESULT(SettingsItemKeyInvalidFormat,     262);
        R_DEFINE_ERROR_RESULT(SettingsItemValueInvalidFormat,   263);

}
