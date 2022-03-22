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

namespace ams::time {

    R_DEFINE_NAMESPACE_RESULT_MODULE(116);

    R_DEFINE_ERROR_RESULT(NotInitialized, 0);

    R_DEFINE_ERROR_RESULT(NotComparable, 200);
    R_DEFINE_ERROR_RESULT(Overflowed,    201);

    R_DEFINE_ABSTRACT_ERROR_RANGE(InvalidArgument, 900, 919);
        R_DEFINE_ERROR_RESULT(InvalidPointer,   901);

}
