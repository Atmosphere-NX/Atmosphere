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

namespace ams::sdmmc {

    R_DEFINE_NAMESPACE_RESULT_MODULE(24);

    R_DEFINE_ERROR_RESULT(NotActivated,  2);
    R_DEFINE_ERROR_RESULT(DeviceRemoved, 3);
    R_DEFINE_ERROR_RESULT(NotAwakened,   4);

    R_DEFINE_ERROR_RANGE(CommunicationError, 32, 126);
        R_DEFINE_ERROR_RANGE(CommunicationNotAttained, 33, 46);
            R_DEFINE_ERROR_RESULT(ResponseIndexError,               34);
            R_DEFINE_ERROR_RESULT(ResponseEndBitError,              35);
            R_DEFINE_ERROR_RESULT(ResponseCrcError,                 36);
            R_DEFINE_ERROR_RESULT(ResponseTimeoutError,             37);
            R_DEFINE_ERROR_RESULT(DataEndBitError,                  38);
            R_DEFINE_ERROR_RESULT(DataCrcError,                     39);
            R_DEFINE_ERROR_RESULT(DataTimeoutError,                 40);
            R_DEFINE_ERROR_RESULT(AutoCommandResponseIndexError,    41);
            R_DEFINE_ERROR_RESULT(AutoCommandResponseEndBitError,   42);
            R_DEFINE_ERROR_RESULT(AutoCommandResponseCrcError,      43);
            R_DEFINE_ERROR_RESULT(AutoCommandResponseTimeoutError,  44);
            R_DEFINE_ERROR_RESULT(CommandCompleteSoftwareTimeout,   45);
            R_DEFINE_ERROR_RESULT(TransferCompleteSoftwareTimeout,  46);
        R_DEFINE_ERROR_RESULT(AbortTransactionSoftwareTimeout,  74);
        R_DEFINE_ERROR_RESULT(CommandInhibitCmdSoftwareTimeout, 75);
        R_DEFINE_ERROR_RESULT(CommandInhibitDatSoftwareTimeout, 76);
        R_DEFINE_ERROR_RESULT(BusySoftwareTimeout,              77);

    R_DEFINE_ERROR_RANGE(HostControllerUnexpected, 128, 158);
        R_DEFINE_ERROR_RESULT(SdHostStandardUnknownAutoCmdError, 130);
        R_DEFINE_ERROR_RESULT(SdHostStandardUnknownError, 131);

    R_DEFINE_ERROR_RANGE(InternalError, 160, 190);
        R_DEFINE_ERROR_RESULT(NoWaitedInterrupt,            161);
        R_DEFINE_ERROR_RESULT(WaitInterruptSoftwareTimeout, 162);

    R_DEFINE_ERROR_RESULT(NotImplemented, 201);
}
