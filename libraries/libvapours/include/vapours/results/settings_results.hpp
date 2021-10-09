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

#pragma once
#include <vapours/results/results_common.hpp>

namespace ams::settings {

    R_DEFINE_NAMESPACE_RESULT_MODULE(105);

    R_DEFINE_ERROR_RESULT(SettingsItemNotFound,                 11);
    R_DEFINE_ERROR_RESULT(StopIteration,                        21);

    R_DEFINE_ERROR_RANGE(InternalError, 100, 149);
        R_DEFINE_ERROR_RESULT(SettingsItemKeyAllocationFailed,         101);
        R_DEFINE_ERROR_RESULT(SettingsItemValueAllocationFailed,       102);
        R_DEFINE_ERROR_RESULT(SettingsItemKeyIteratorAllocationFailed, 111);
        R_DEFINE_ERROR_RESULT(TooLargeSystemSaveData,                  141);

    R_DEFINE_ERROR_RANGE(InvalidArgument, 200, 399);
        R_DEFINE_ERROR_RESULT(NullSettingsName,                             201);
        R_DEFINE_ERROR_RESULT(NullSettingsItemKey,                          202);
        R_DEFINE_ERROR_RESULT(NullSettingsItemValue,                        203);
        R_DEFINE_ERROR_RESULT(NullSettingsItemKeyBuffer,                    204);
        R_DEFINE_ERROR_RESULT(NullSettingsItemValueBuffer,                  205);

        R_DEFINE_ERROR_RESULT(EmptySettingsName,                            221);
        R_DEFINE_ERROR_RESULT(EmptySettingsItemKey,                         222);

        R_DEFINE_ERROR_RESULT(TooLongSettingsName,                          241);
        R_DEFINE_ERROR_RESULT(TooLongSettingsItemKey,                       242);

        R_DEFINE_ERROR_RESULT(InvalidFormatSettingsName,                    261);
        R_DEFINE_ERROR_RESULT(InvalidFormatSettingsItemKey,                 262);
        R_DEFINE_ERROR_RESULT(InvalidFormatSettingsItemValue,               263);

        R_DEFINE_ERROR_RESULT(NotFoundSettingsItemKeyIterator,              281);

    R_DEFINE_ERROR_RANGE(CalibrationDataError, 580, 599);
        R_DEFINE_ERROR_RESULT(CalibrationDataFileSystemCorrupted, 581);
        R_DEFINE_ERROR_RESULT(CalibrationDataCrcError,            582);
        R_DEFINE_ERROR_RESULT(CalibrationDataShaError,            583);

}
