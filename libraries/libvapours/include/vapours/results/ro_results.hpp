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

namespace ams::ro {

    R_DEFINE_NAMESPACE_RESULT_MODULE(22);

    R_DEFINE_ERROR_RANGE(RoError, 1, 1023);
        R_DEFINE_ERROR_RESULT(OutOfAddressSpace,        2);
        R_DEFINE_ERROR_RESULT(AlreadyLoaded,            3);
        R_DEFINE_ERROR_RESULT(InvalidNro,               4);

        R_DEFINE_ERROR_RESULT(InvalidNrr,               6);
        R_DEFINE_ERROR_RESULT(TooManyNro,               7);
        R_DEFINE_ERROR_RESULT(TooManyNrr,               8);
        R_DEFINE_ERROR_RESULT(NotAuthorized,            9);
        R_DEFINE_ERROR_RESULT(InvalidNrrKind,           10);

        R_DEFINE_ERROR_RESULT(InternalError,            1023);

    R_DEFINE_ERROR_RESULT(InvalidAddress,           1025);
    R_DEFINE_ERROR_RESULT(InvalidSize,              1026);

    R_DEFINE_ERROR_RESULT(NotLoaded,                1028);
    R_DEFINE_ERROR_RESULT(NotRegistered,            1029);
    R_DEFINE_ERROR_RESULT(InvalidSession,           1030);
    R_DEFINE_ERROR_RESULT(InvalidProcess,           1031);

}
