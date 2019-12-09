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

namespace ams::fs {

    R_DEFINE_NAMESPACE_RESULT_MODULE(2);

    R_DEFINE_ERROR_RESULT(PathNotFound,      1);
    R_DEFINE_ERROR_RESULT(PathAlreadyExists, 2);

    R_DEFINE_ERROR_RESULT(TargetLocked,      7);
    R_DEFINE_ERROR_RESULT(DirectoryNotEmpty, 8);

    R_DEFINE_ERROR_RANGE (NotEnoughFreeSpace, 30, 45);
        R_DEFINE_ERROR_RANGE(NotEnoughFreeSpaceBis, 34, 38);
            R_DEFINE_ERROR_RESULT(NotEnoughFreeSpaceBisCalibration, 35);
            R_DEFINE_ERROR_RESULT(NotEnoughFreeSpaceBisSafe,        36);
            R_DEFINE_ERROR_RESULT(NotEnoughFreeSpaceBisUser,        37);
            R_DEFINE_ERROR_RESULT(NotEnoughFreeSpaceBisSystem,      38);
        R_DEFINE_ERROR_RESULT(NotEnoughFreeSpaceSdCard, 39);

    R_DEFINE_ERROR_RESULT(MountNameAlreadyExists, 60);

    R_DEFINE_ERROR_RESULT(TargetNotFound,   1002);

    R_DEFINE_ERROR_RANGE(SdCardAccessFailed, 2000, 2499);
        R_DEFINE_ERROR_RESULT(SdCardNotPresent, 2001);

    R_DEFINE_ERROR_RANGE(GameCardAccessFailed, 2500, 2999);

    R_DEFINE_ERROR_RESULT(NotImplemented, 3001);
    R_DEFINE_ERROR_RESULT(OutOfRange,     3005);

    R_DEFINE_ERROR_RANGE(AllocationFailure, 3200, 3499);
        R_DEFINE_ERROR_RESULT(AllocationFailureInDirectorySaveDataFileSystem, 3321);
        R_DEFINE_ERROR_RESULT(AllocationFailureInSubDirectoryFileSystem,      3355);
        R_DEFINE_ERROR_RESULT(AllocationFailureInPathNormalizer,              3367);
        R_DEFINE_ERROR_RESULT(AllocationFailureInFileSystemInterfaceAdapter,  3407);

    R_DEFINE_ERROR_RANGE(MmcAccessFailed, 3500, 3999);

    R_DEFINE_ERROR_RANGE(DataCorrupted, 4000, 4999);
        R_DEFINE_ERROR_RANGE(RomCorrupted, 4001, 4299);
        R_DEFINE_ERROR_RANGE(SaveDataCorrupted, 4301, 4499);
        R_DEFINE_ERROR_RANGE(NcaCorrupted, 4501, 4599);
        R_DEFINE_ERROR_RANGE(IntegrityVerificationStorageCorrupted, 4601, 4639);
        R_DEFINE_ERROR_RANGE(PartitionFileSystemCorrupted, 4641, 4659);
        R_DEFINE_ERROR_RANGE(BuiltInStorageCorrupted, 4661, 4679);
        R_DEFINE_ERROR_RANGE(HostFileSystemCorrupted, 4701, 4719);
        R_DEFINE_ERROR_RANGE(DatabaseCorrupted, 4721, 4739);
        R_DEFINE_ERROR_RANGE(AesXtsFileSystemCorrupted, 4741, 4759);
        R_DEFINE_ERROR_RANGE(SaveDataTransferDataCorrupted, 4761, 4769);
        R_DEFINE_ERROR_RANGE(SignedSystemPartitionDataCorrupted, 4771, 4779);

        R_DEFINE_ERROR_RESULT(GameCardLogoDataCorrupted, 4781);

    R_DEFINE_ERROR_RANGE(Unexpected, 5000, 5999);

    R_DEFINE_ERROR_RANGE(PreconditionViolation, 6000, 6499);
        R_DEFINE_ERROR_RANGE(InvalidArgument, 6001, 6199);
            R_DEFINE_ERROR_RANGE(InvalidPath, 6002, 6029);
                R_DEFINE_ERROR_RESULT(TooLongPath,           6003);
                R_DEFINE_ERROR_RESULT(InvalidCharacter,      6004);
                R_DEFINE_ERROR_RESULT(InvalidPathFormat,     6005);
                R_DEFINE_ERROR_RESULT(DirectoryUnobtainable, 6006);
                R_DEFINE_ERROR_RESULT(NotNormalized,         6007);

            R_DEFINE_ERROR_RANGE(InvalidPathForOperation, 6030, 6059);
                R_DEFINE_ERROR_RESULT(DirectoryNotDeletable,   6031);
                R_DEFINE_ERROR_RESULT(DirectoryNotRenamable,   6032);
                R_DEFINE_ERROR_RESULT(IncompatiblePath,        6033);
                R_DEFINE_ERROR_RESULT(RenameToOtherFileSystem, 6034);

            R_DEFINE_ERROR_RESULT(InvalidOffset,    6061);
            R_DEFINE_ERROR_RESULT(InvalidSize,      6062);
            R_DEFINE_ERROR_RESULT(NullptrArgument,  6063);
            R_DEFINE_ERROR_RESULT(InvalidAlignment, 6064);
            R_DEFINE_ERROR_RESULT(InvalidMountName, 6065);

            R_DEFINE_ERROR_RESULT(ExtensionSizeTooLarge, 6066);
            R_DEFINE_ERROR_RESULT(ExtensionSizeInvalid,  6067);

            R_DEFINE_ERROR_RANGE(InvalidEnumValue,  6080, 6099);
                R_DEFINE_ERROR_RESULT(InvalidSaveDataState, 6081);
                R_DEFINE_ERROR_RESULT(InvalidSaveDataSpaceId, 6082);

            R_DEFINE_ERROR_RANGE(InvalidOperationForOpenMode, 6200, 6299);
                R_DEFINE_ERROR_RESULT(FileExtensionWithoutOpenModeAllowAppend, 6201);

            R_DEFINE_ERROR_RANGE(UnsupportedOperation, 6300, 6399);

            R_DEFINE_ERROR_RANGE(PermissionDenied, 6400, 6449);

            R_DEFINE_ERROR_RESULT(WriteModeFileNotClosed, 6457);
            R_DEFINE_ERROR_RESULT(AllocatorAlignmentViolation,  6461);
            R_DEFINE_ERROR_RESULT(UserNotExist,           6465);

    R_DEFINE_ERROR_RANGE(OutOfResource, 6700, 6799);
        R_DEFINE_ERROR_RESULT(MappingTableFull, 6706);
        R_DEFINE_ERROR_RESULT(OpenCountLimit,   6709);

    R_DEFINE_ERROR_RANGE(MappingFailed, 6800, 6899);
        R_DEFINE_ERROR_RESULT(MapFull,  6811);

    R_DEFINE_ERROR_RANGE(BadState, 6900, 6999);
        R_DEFINE_ERROR_RESULT(NotMounted,   6905);

}
