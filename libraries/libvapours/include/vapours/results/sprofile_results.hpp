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

namespace ams::sprofile {

    R_DEFINE_NAMESPACE_RESULT_MODULE(246);

    R_DEFINE_ERROR_RESULT(InvalidArgument,  100);
    R_DEFINE_ERROR_RESULT(InvalidState,     101);

    R_DEFINE_ERROR_RESULT(NotPermitted,     303);

    R_DEFINE_ERROR_RESULT(AllocationFailed, 401);

    R_DEFINE_ERROR_RESULT(KeyNotFound,      600);
    R_DEFINE_ERROR_RESULT(InvalidDataType,  601);

    R_DEFINE_ERROR_RESULT(MaxListeners,     620);
    R_DEFINE_ERROR_RESULT(AlreadyListening, 621);
    R_DEFINE_ERROR_RESULT(NotListening,     622);
    R_DEFINE_ERROR_RESULT(MaxObservers,     623);

    R_DEFINE_ERROR_RESULT(InvalidMetadataVersion, 3210);
    R_DEFINE_ERROR_RESULT(InvalidMetadataHash,    3211);
    R_DEFINE_ERROR_RESULT(InvalidDataVersion,     3230);
    R_DEFINE_ERROR_RESULT(InvalidDataHash,        3231);

}
