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

namespace ams::ns {

    R_DEFINE_NAMESPACE_RESULT_MODULE(16);

    R_DEFINE_ERROR_RESULT(Canceled,                           90);
    R_DEFINE_ERROR_RESULT(OutOfMaxRunningTask,               110);
    R_DEFINE_ERROR_RESULT(CardUpdateNotSetup,                270);
    R_DEFINE_ERROR_RESULT(CardUpdateNotPrepared,             280);
    R_DEFINE_ERROR_RESULT(CardUpdateAlreadySetup,            290);
    R_DEFINE_ERROR_RESULT(PrepareCardUpdateAlreadyRequested, 460);

}
