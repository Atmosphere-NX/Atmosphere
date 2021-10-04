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

namespace ams::sm {

    R_DEFINE_NAMESPACE_RESULT_MODULE(21);

    R_DEFINE_ERROR_RESULT(OutOfProcesses,        1);
    R_DEFINE_ERROR_RESULT(InvalidClient,         2);
    R_DEFINE_ERROR_RESULT(OutOfSessions,         3);
    R_DEFINE_ERROR_RESULT(AlreadyRegistered,     4);
    R_DEFINE_ERROR_RESULT(OutOfServices,         5);
    R_DEFINE_ERROR_RESULT(InvalidServiceName,    6);
    R_DEFINE_ERROR_RESULT(NotRegistered,         7);
    R_DEFINE_ERROR_RESULT(NotAllowed,            8);
    R_DEFINE_ERROR_RESULT(TooLargeAccessControl, 9);

    /* Results 1000-2000 used as extension for Atmosphere Mitm. */
    namespace mitm {

        R_DEFINE_ERROR_RESULT(ShouldForwardToSession, 1000);
        R_DEFINE_ERROR_RESULT(ProcessNotAssociated,   1100);

    }

}
