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

namespace ams::htcfs {

    R_DEFINE_NAMESPACE_RESULT_MODULE(31);

    R_DEFINE_ERROR_RESULT(InvalidArgument,   3);


    R_DEFINE_ERROR_RANGE(ConnectionFailure, 100, 199);
        R_DEFINE_ERROR_RESULT(HtclowChannelClosed, 101);

        R_DEFINE_ERROR_RANGE(UnexpectedResponse, 110, 119);
            R_DEFINE_ERROR_RESULT(UnexpectedResponseProtocolId,      111);
            R_DEFINE_ERROR_RESULT(UnexpectedResponseProtocolVersion, 112);
            R_DEFINE_ERROR_RESULT(UnexpectedResponsePacketCategory,  113);
            R_DEFINE_ERROR_RESULT(UnexpectedResponsePacketType,      114);
            R_DEFINE_ERROR_RESULT(UnexpectedResponseBodySize,        115);
            R_DEFINE_ERROR_RESULT(UnexpectedResponseBody,            116);

    R_DEFINE_ERROR_RANGE(InternalError, 200, 299);
        R_DEFINE_ERROR_RESULT(InvalidSize,                201);
        R_DEFINE_ERROR_RESULT(UnknownError,               211);
        R_DEFINE_ERROR_RESULT(UnsupportedProtocolVersion, 212);
        R_DEFINE_ERROR_RESULT(InvalidRequest,             213);
        R_DEFINE_ERROR_RESULT(InvalidHandle,              214);
        R_DEFINE_ERROR_RESULT(OutOfHandle,                215);

}
