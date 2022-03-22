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

namespace ams::err {

    R_DEFINE_NAMESPACE_RESULT_MODULE(162);

    R_DEFINE_ERROR_RESULT(ApplicationAbort,       1);
    R_DEFINE_ERROR_RESULT(SystemProgramAbort,     2);

    R_DEFINE_ERROR_RESULT(ForcedShutdownDetected, 4);

}
