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

namespace ams::lr {

    R_DEFINE_NAMESPACE_RESULT_MODULE(8);

    R_DEFINE_ERROR_RESULT(ProgramNotFound,          2);
    R_DEFINE_ERROR_RESULT(DataNotFound,             3);
    R_DEFINE_ERROR_RESULT(UnknownStorageId,         4);
    R_DEFINE_ERROR_RESULT(HtmlDocumentNotFound,     6);
    R_DEFINE_ERROR_RESULT(AddOnContentNotFound,     7);
    R_DEFINE_ERROR_RESULT(ControlNotFound,          8);
    R_DEFINE_ERROR_RESULT(LegalInformationNotFound, 9);
    R_DEFINE_ERROR_RESULT(DebugProgramNotFound,    10);

    R_DEFINE_ERROR_RESULT(TooManyRegisteredPaths,   90);

}
