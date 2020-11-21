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

namespace ams::xusb {

    /* Please note: These results are all custom, and not official. */
    R_DEFINE_NAMESPACE_RESULT_MODULE(445);


    /* Result 1-1000 reserved for Atmosphere. */

    /* USB protocol-level results. */
    R_DEFINE_ERROR_RESULT(InvalidDeviceState,               1);
    R_DEFINE_ERROR_RESULT(MalformedSetupRequest,            2);
    R_DEFINE_ERROR_RESULT(UnknownSetupRequest,              3);
    R_DEFINE_ERROR_RESULT(UnknownDescriptorType,            4);
    R_DEFINE_ERROR_RESULT(InvalidDescriptorIndex,           5);
    R_DEFINE_ERROR_RESULT(InvalidAddress,                   6);
    R_DEFINE_ERROR_RESULT(ControlEndpointBusy,              7);
    R_DEFINE_ERROR_RESULT(InvalidSetupPacketSequenceNumber, 8);
    R_DEFINE_ERROR_RESULT(InvalidControlEndpointState,      9);
    R_DEFINE_ERROR_RESULT(InvalidConfiguration,             10);
    R_DEFINE_ERROR_RESULT(TransferRingFull,                 11);

    /* Gadget-level results. */
    R_DEFINE_ERROR_RESULT(UnexpectedCompletionCode, 101);
    R_DEFINE_ERROR_RESULT(UnexpectedEndpoint,       102);
    R_DEFINE_ERROR_RESULT(UnexpectedTRB,            103);
    R_DEFINE_ERROR_RESULT(DownloadTooLarge,         104);
    
}
