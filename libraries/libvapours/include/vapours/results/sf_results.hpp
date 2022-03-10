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

R_DEFINE_NAMESPACE_RESULT_MODULE(ams::sf, 10);

namespace ams::sf {

    R_DEFINE_ERROR_RESULT(NotSupported,             1);
    R_DEFINE_ERROR_RESULT(PreconditionViolation,    3);

    //namespace cmif {

        R_DEFINE_ERROR_RESULT_NS(cmif, InvalidHeaderSize,    202);
        R_DEFINE_ERROR_RESULT_NS(cmif, InvalidInHeader,      211);
        R_DEFINE_ERROR_RESULT_NS(cmif, UnknownCommandId,     221);
        R_DEFINE_ERROR_RESULT_NS(cmif, InvalidOutRawSize,    232);
        R_DEFINE_ERROR_RESULT_NS(cmif, InvalidNumInObjects,  235);
        R_DEFINE_ERROR_RESULT_NS(cmif, InvalidNumOutObjects, 236);
        R_DEFINE_ERROR_RESULT_NS(cmif, InvalidInObject,      239);

        R_DEFINE_ERROR_RESULT_NS(cmif, TargetNotFound,       261);

        R_DEFINE_ERROR_RESULT_NS(cmif, OutOfDomainEntries,   301);

    //}

    //namespace impl {

        R_DEFINE_ABSTRACT_ERROR_RANGE_NS(impl, RequestContextChanged, 800, 899);
            R_DEFINE_ABSTRACT_ERROR_RANGE_NS(impl, RequestInvalidated, 801, 809);
                R_DEFINE_ERROR_RESULT_NS(impl, RequestInvalidatedByUser, 802);

    //}

    R_DEFINE_ABSTRACT_ERROR_RANGE(RequestDeferred, 811, 819);
        R_DEFINE_ERROR_RESULT(RequestDeferredByUser, 812);

}
