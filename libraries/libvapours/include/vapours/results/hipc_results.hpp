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

namespace ams::sf::hipc {

    R_DEFINE_NAMESPACE_RESULT_MODULE(11);

    R_DEFINE_ABSTRACT_ERROR_RANGE(OutOfResource, 100, 299);
        R_DEFINE_ERROR_RESULT(OutOfSessionMemory,    102);
        R_DEFINE_ERROR_RANGE (OutOfSessions,         131, 139);
        R_DEFINE_ERROR_RESULT(PointerBufferTooSmall, 141);

        R_DEFINE_ERROR_RESULT(OutOfDomains,          200);

    R_DEFINE_ERROR_RESULT(SessionClosed,        301);

    R_DEFINE_ERROR_RESULT(InvalidRequestSize,   402);
    R_DEFINE_ERROR_RESULT(UnknownCommandType,   403);

    R_DEFINE_ERROR_RESULT(InvalidCmifRequest,   420);

    R_DEFINE_ERROR_RESULT(TargetNotDomain,      491);
    R_DEFINE_ERROR_RESULT(DomainObjectNotFound, 492);

}
