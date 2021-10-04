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

namespace ams::htc {

    R_DEFINE_NAMESPACE_RESULT_MODULE(18);

    R_DEFINE_ERROR_RESULT(ConnectionFailure,  1);
    R_DEFINE_ERROR_RESULT(NotFound,           2);
    R_DEFINE_ERROR_RESULT(NotEnoughBuffer,    3);

    R_DEFINE_ERROR_RESULT(Cancelled,        101);

    R_DEFINE_ERROR_RESULT(Unknown,         1023);

    R_DEFINE_ERROR_RESULT(Unknown2001,           2001);
    R_DEFINE_ERROR_RESULT(InvalidTaskId,         2003);
    R_DEFINE_ERROR_RESULT(InvalidSize,           2011);
    R_DEFINE_ERROR_RESULT(TaskCancelled,         2021);
    R_DEFINE_ERROR_RESULT(TaskNotCompleted,      2022);
    R_DEFINE_ERROR_RESULT(TaskQueueNotAvailable, 2033);

    R_DEFINE_ERROR_RESULT(Unknown2101,     2101);
    R_DEFINE_ERROR_RESULT(OutOfRpcTask,    2102);
    R_DEFINE_ERROR_RESULT(InvalidCategory, 2123);

}
