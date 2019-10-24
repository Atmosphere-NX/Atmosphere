/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "results_common.hpp"

namespace ams::sf {

    R_DEFINE_NAMESPACE_RESULT_MODULE(10);

    R_DEFINE_ERROR_RESULT(NotSupported,             1);
    R_DEFINE_ERROR_RESULT(PreconditionViolation,    3);

    namespace cmif {

        R_DEFINE_ERROR_RESULT(InvalidHeaderSize,    202);
        R_DEFINE_ERROR_RESULT(InvalidInHeader,      211);
        R_DEFINE_ERROR_RESULT(UnknownCommandId,     221);
        R_DEFINE_ERROR_RESULT(InvalidOutRawSize,    232);
        R_DEFINE_ERROR_RESULT(InvalidNumInObjects,  235);
        R_DEFINE_ERROR_RESULT(InvalidNumOutObjects, 236);
        R_DEFINE_ERROR_RESULT(InvalidInObject,      239);

        R_DEFINE_ERROR_RESULT(TargetNotFound,       261);

        R_DEFINE_ERROR_RESULT(OutOfDomainEntries,   301);

    }

    namespace impl {

        R_DEFINE_ABSTRACT_ERROR_RANGE(RequestContextChanged, 800, 899);
            R_DEFINE_ABSTRACT_ERROR_RANGE(RequestInvalidated, 801, 809);
                R_DEFINE_ERROR_RESULT(RequestInvalidatedByUser, 802);

    }

    R_DEFINE_ABSTRACT_ERROR_RANGE(RequestDeferred, 811, 819);
        R_DEFINE_ERROR_RESULT(RequestDeferredByUser, 812);

}
