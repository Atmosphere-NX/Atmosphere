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

namespace ams::erpt {

    R_DEFINE_NAMESPACE_RESULT_MODULE(147);

    R_DEFINE_ERROR_RESULT(NotInitialized,            1);
    R_DEFINE_ERROR_RESULT(AlreadyInitialized,        2);
    R_DEFINE_ERROR_RESULT(OutOfArraySpace,           3);
    R_DEFINE_ERROR_RESULT(OutOfFieldSpace,           4);
    R_DEFINE_ERROR_RESULT(OutOfMemory,               5);
    R_DEFINE_ERROR_RESULT(InvalidArgument,           7);
    R_DEFINE_ERROR_RESULT(NotFound,                  8);
    R_DEFINE_ERROR_RESULT(FieldCategoryMismatch,     9);
    R_DEFINE_ERROR_RESULT(FieldTypeMismatch,        10);
    R_DEFINE_ERROR_RESULT(AlreadyExists,            11);
    R_DEFINE_ERROR_RESULT(CorruptJournal,           12);
    R_DEFINE_ERROR_RESULT(CategoryNotFound,         13);
    R_DEFINE_ERROR_RESULT(RequiredContextMissing,   14);
    R_DEFINE_ERROR_RESULT(RequiredFieldMissing,     15);
    R_DEFINE_ERROR_RESULT(FormatterError,           16);
    R_DEFINE_ERROR_RESULT(InvalidPowerState,        17);
    R_DEFINE_ERROR_RESULT(ArrayFieldTooLarge,       18);
    R_DEFINE_ERROR_RESULT(AlreadyOwned,             19);

}
