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

R_DEFINE_NAMESPACE_RESULT_MODULE(ams::ldr, 9);

namespace ams::ldr {

    R_DEFINE_ERROR_RESULT(ArgumentOverflow,          1);
    R_DEFINE_ERROR_RESULT(ArgumentCountOverflow,     2);
    R_DEFINE_ERROR_RESULT(MetaOverflow,              3);
    R_DEFINE_ERROR_RESULT(InvalidMeta,               4);
    R_DEFINE_ERROR_RESULT(InvalidNso,                5);
    R_DEFINE_ERROR_RESULT(InvalidPath,               6);
    R_DEFINE_ERROR_RESULT(MaxProcess,                7);
    R_DEFINE_ERROR_RESULT(NotPinned,                 8);
    R_DEFINE_ERROR_RESULT(InvalidProgramId,          9);
    R_DEFINE_ERROR_RESULT(InvalidVersion,           10);
    R_DEFINE_ERROR_RESULT(InvalidAcidSignature,     11);
    R_DEFINE_ERROR_RESULT(InvalidNcaSignature,      12);

    R_DEFINE_ERROR_RESULT(InvalidProgramAttributes, 14);

    R_DEFINE_ERROR_RESULT(OutOfAddressSpace,            51);
    R_DEFINE_ERROR_RESULT(InvalidNroImage,              52);
    R_DEFINE_ERROR_RESULT(InvalidNrrImage,              53);
    R_DEFINE_ERROR_RESULT(NotAuthorized,                54);
    R_DEFINE_ERROR_RESULT(MaxModule,                    55);
    R_DEFINE_ERROR_RESULT(MaxRegistration,              56);
    R_DEFINE_ERROR_RESULT(NroAlreadyLoaded,             57);

    R_DEFINE_ERROR_RESULT(InvalidAddress,        81);
    R_DEFINE_ERROR_RESULT(InvalidSize,           82);
    R_DEFINE_ERROR_RESULT(InvalidCurrentMemory,  83);
    R_DEFINE_ERROR_RESULT(NotLoaded,             84);
    R_DEFINE_ERROR_RESULT(NotRegistered,         85);
    R_DEFINE_ERROR_RESULT(InvalidSession,        86);
    R_DEFINE_ERROR_RESULT(InvalidProcess,        87);

    R_DEFINE_ERROR_RESULT(UnknownCapability,                100);
    R_DEFINE_ERROR_RESULT(InvalidCapabilityKernelFlags,     103);
    R_DEFINE_ERROR_RESULT(InvalidCapabilitySyscallMask,     104);
    R_DEFINE_ERROR_RESULT(InvalidCapabilityMapRange,        106);
    R_DEFINE_ERROR_RESULT(InvalidCapabilityMapPage,         107);
    R_DEFINE_ERROR_RESULT(InvalidCapabilityMapRegion,       110);
    R_DEFINE_ERROR_RESULT(InvalidCapabilityInterruptPair,   111);
    R_DEFINE_ERROR_RESULT(InvalidCapabilityApplicationType, 113);
    R_DEFINE_ERROR_RESULT(InvalidCapabilityKernelVersion,   114);
    R_DEFINE_ERROR_RESULT(InvalidCapabilityHandleTable,     115);
    R_DEFINE_ERROR_RESULT(InvalidCapabilityDebugFlags,      116);

    R_DEFINE_ERROR_RESULT(InternalError, 200);

}
