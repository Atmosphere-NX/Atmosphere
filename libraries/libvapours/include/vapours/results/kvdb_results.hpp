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

namespace ams::kvdb {

    R_DEFINE_NAMESPACE_RESULT_MODULE(20);

    R_DEFINE_ERROR_RESULT(OutOfKeyResource,        1);
    R_DEFINE_ERROR_RESULT(KeyNotFound,             2);
    R_DEFINE_ERROR_RESULT(AllocationFailed,        4);
    R_DEFINE_ERROR_RESULT(InvalidKeyValue,         5);
    R_DEFINE_ERROR_RESULT(BufferInsufficient,      6);

    R_DEFINE_ERROR_RESULT(InvalidFilesystemState,  8);
    R_DEFINE_ERROR_RESULT(NotCreated,              9);

}
