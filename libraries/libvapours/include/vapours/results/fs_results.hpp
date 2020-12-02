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

namespace ams::fs {

    R_DEFINE_NAMESPACE_RESULT_MODULE(2);

    R_DEFINE_ERROR_RANGE(HandledByAllProcess, 0, 999);
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

        R_DEFINE_ERROR_RESULT(UnsupportedSdkVersion, 50);

        R_DEFINE_ERROR_RESULT(MountNameAlreadyExists, 60);

    R_DEFINE_ERROR_RANGE(HandledBySystemProcess, 1000, 2999);
        R_DEFINE_ERROR_RESULT(PartitionNotFound,    1001);
        R_DEFINE_ERROR_RESULT(TargetNotFound,       1002);

        R_DEFINE_ERROR_RANGE(SdCardAccessFailed, 2000, 2499);
            R_DEFINE_ERROR_RESULT(SdCardNotPresent, 2001);

        R_DEFINE_ERROR_RANGE(GameCardAccessFailed, 2500, 2999);

        R_DEFINE_ERROR_RESULT(NotImplemented,     3001);
        R_DEFINE_ERROR_RESULT(UnsupportedVersion, 3002);
        R_DEFINE_ERROR_RESULT(OutOfRange,         3005);

        R_DEFINE_ERROR_RESULT(SystemPartitionNotReady, 3100);

        R_DEFINE_ERROR_RANGE(AllocationFailure, 3200, 3499);
            R_DEFINE_ERROR_RESULT(AllocationFailureInFileSystemAccessorA,           3211);
            R_DEFINE_ERROR_RESULT(AllocationFailureInFileSystemAccessorB,           3212);
            R_DEFINE_ERROR_RESULT(AllocationFailureInApplicationA,                  3213);
            R_DEFINE_ERROR_RESULT(AllocationFailureInBisA,                          3215);
            R_DEFINE_ERROR_RESULT(AllocationFailureInBisB,                          3216);
            R_DEFINE_ERROR_RESULT(AllocationFailureInBisC,                          3217);
            R_DEFINE_ERROR_RESULT(AllocationFailureInCodeA,                         3218);
            R_DEFINE_ERROR_RESULT(AllocationFailureInContentA,                      3219);
            R_DEFINE_ERROR_RESULT(AllocationFailureInContentStorageA,               3220);
            R_DEFINE_ERROR_RESULT(AllocationFailureInContentStorageB,               3221);
            R_DEFINE_ERROR_RESULT(AllocationFailureInDataA,                         3222);
            R_DEFINE_ERROR_RESULT(AllocationFailureInDataB,                         3223);
            R_DEFINE_ERROR_RESULT(AllocationFailureInDeviceSaveDataA,               3224);
            R_DEFINE_ERROR_RESULT(AllocationFailureInGameCardA,                     3225);
            R_DEFINE_ERROR_RESULT(AllocationFailureInGameCardB,                     3226);
            R_DEFINE_ERROR_RESULT(AllocationFailureInGameCardC,                     3227);
            R_DEFINE_ERROR_RESULT(AllocationFailureInGameCardD,                     3228);
            R_DEFINE_ERROR_RESULT(AllocationFailureInImageDirectoryA,               3232);
            R_DEFINE_ERROR_RESULT(AllocationFailureInSdCardA,                       3244);
            R_DEFINE_ERROR_RESULT(AllocationFailureInSdCardB,                       3245);
            R_DEFINE_ERROR_RESULT(AllocationFailureInSystemSaveDataA,               3246);
            R_DEFINE_ERROR_RESULT(AllocationFailureInRomFsFileSystemA,              3247);
            R_DEFINE_ERROR_RESULT(AllocationFailureInRomFsFileSystemB,              3248);
            R_DEFINE_ERROR_RESULT(AllocationFailureInRomFsFileSystemC,              3249);
            R_DEFINE_ERROR_RESULT(AllocationFailureInFileSystemProxyCoreImplD,      3256);
            R_DEFINE_ERROR_RESULT(AllocationFailureInFileSystemProxyCoreImplE,      3257);
            R_DEFINE_ERROR_RESULT(AllocationFailureInPartitionFileSystemCreatorA,   3280);
            R_DEFINE_ERROR_RESULT(AllocationFailureInRomFileSystemCreatorA,         3281);
            R_DEFINE_ERROR_RESULT(AllocationFailureInStorageOnNcaCreatorA,          3288);
            R_DEFINE_ERROR_RESULT(AllocationFailureInStorageOnNcaCreatorB,          3289);
            R_DEFINE_ERROR_RESULT(AllocationFailureInFileSystemBuddyHeapA,          3294);
            R_DEFINE_ERROR_RESULT(AllocationFailureInFileSystemBufferManagerA,      3295);
            R_DEFINE_ERROR_RESULT(AllocationFailureInBlockCacheBufferedStorageA,    3296);
            R_DEFINE_ERROR_RESULT(AllocationFailureInBlockCacheBufferedStorageB,    3297);
            R_DEFINE_ERROR_RESULT(AllocationFailureInIntegrityVerificationStorageA, 3304);
            R_DEFINE_ERROR_RESULT(AllocationFailureInIntegrityVerificationStorageB, 3305);
            R_DEFINE_ERROR_RESULT(AllocationFailureInDirectorySaveDataFileSystem,   3321);
            R_DEFINE_ERROR_RESULT(AllocationFailureInNcaFileSystemDriverI,          3341);
            R_DEFINE_ERROR_RESULT(AllocationFailureInPartitionFileSystemA,          3347);
            R_DEFINE_ERROR_RESULT(AllocationFailureInPartitionFileSystemB,          3348);
            R_DEFINE_ERROR_RESULT(AllocationFailureInPartitionFileSystemC,          3349);
            R_DEFINE_ERROR_RESULT(AllocationFailureInPartitionFileSystemMetaA,      3350);
            R_DEFINE_ERROR_RESULT(AllocationFailureInPartitionFileSystemMetaB,      3351);
            R_DEFINE_ERROR_RESULT(AllocationFailureInRomFsFileSystemD,              3352);
            R_DEFINE_ERROR_RESULT(AllocationFailureInSubDirectoryFileSystem,        3355);
            R_DEFINE_ERROR_RESULT(AllocationFailureInNcaReaderA,                    3363);
            R_DEFINE_ERROR_RESULT(AllocationFailureInRegisterA,                     3365);
            R_DEFINE_ERROR_RESULT(AllocationFailureInRegisterB,                     3366);
            R_DEFINE_ERROR_RESULT(AllocationFailureInPathNormalizer,                3367);
            R_DEFINE_ERROR_RESULT(AllocationFailureInDbmRomKeyValueStorage,         3375);
            R_DEFINE_ERROR_RESULT(AllocationFailureInRomFsFileSystemE,              3377);
            R_DEFINE_ERROR_RESULT(AllocationFailureInReadOnlyFileSystemA,           3386);
            R_DEFINE_ERROR_RESULT(AllocationFailureInAesCtrCounterExtendedStorageA, 3399);
            R_DEFINE_ERROR_RESULT(AllocationFailureInAesCtrCounterExtendedStorageB, 3400);
            R_DEFINE_ERROR_RESULT(AllocationFailureInFileSystemInterfaceAdapter,    3407);
            R_DEFINE_ERROR_RESULT(AllocationFailureInBufferedStorageA,              3411);
            R_DEFINE_ERROR_RESULT(AllocationFailureInIntegrityRomFsStorageA,        3412);
            R_DEFINE_ERROR_RESULT(AllocationFailureInNew,                           3420);
            R_DEFINE_ERROR_RESULT(AllocationFailureInMakeUnique,                    3422);
            R_DEFINE_ERROR_RESULT(AllocationFailureInAllocateShared,                3423);
            R_DEFINE_ERROR_RESULT(AllocationFailurePooledBufferNotEnoughSize,       3424);

    R_DEFINE_ERROR_RANGE(Internal, 3000, 7999);
        R_DEFINE_ERROR_RANGE(MmcAccessFailed, 3500, 3999);

        R_DEFINE_ERROR_RANGE(DataCorrupted, 4000, 4999);
            R_DEFINE_ERROR_RANGE(RomCorrupted, 4001, 4299);
                R_DEFINE_ERROR_RESULT(UnsupportedRomVersion, 4002);

                R_DEFINE_ERROR_RANGE(AesCtrCounterExtendedStorageCorrupted, 4011, 4019);
                    R_DEFINE_ERROR_RESULT(InvalidAesCtrCounterExtendedEntryOffset,  4012);
                    R_DEFINE_ERROR_RESULT(InvalidAesCtrCounterExtendedTableSize,    4013);
                    R_DEFINE_ERROR_RESULT(InvalidAesCtrCounterExtendedGeneration,   4014);
                    R_DEFINE_ERROR_RESULT(InvalidAesCtrCounterExtendedOffset,       4015);

                R_DEFINE_ERROR_RANGE(IndirectStorageCorrupted, 4021, 4029);
                    R_DEFINE_ERROR_RESULT(InvalidIndirectEntryOffset,       4022);
                    R_DEFINE_ERROR_RESULT(InvalidIndirectEntryStorageIndex, 4023);
                    R_DEFINE_ERROR_RESULT(InvalidIndirectStorageSize,       4024);
                    R_DEFINE_ERROR_RESULT(InvalidIndirectVirtualOffset,     4025);
                    R_DEFINE_ERROR_RESULT(InvalidIndirectPhysicalOffset,    4026);
                    R_DEFINE_ERROR_RESULT(InvalidIndirectStorageIndex,      4027);

                R_DEFINE_ERROR_RANGE(BucketTreeCorrupted, 4031, 4039);
                    R_DEFINE_ERROR_RESULT(InvalidBucketTreeSignature,       4032);
                    R_DEFINE_ERROR_RESULT(InvalidBucketTreeEntryCount,      4033);
                    R_DEFINE_ERROR_RESULT(InvalidBucketTreeNodeEntryCount,  4034);
                    R_DEFINE_ERROR_RESULT(InvalidBucketTreeNodeOffset,      4035);
                    R_DEFINE_ERROR_RESULT(InvalidBucketTreeEntryOffset,     4036);
                    R_DEFINE_ERROR_RESULT(InvalidBucketTreeEntrySetOffset,  4037);
                    R_DEFINE_ERROR_RESULT(InvalidBucketTreeNodeIndex,       4038);
                    R_DEFINE_ERROR_RESULT(InvalidBucketTreeVirtualOffset,   4039);

                R_DEFINE_ERROR_RANGE(RomNcaCorrupted, 4041, 4139);
                    R_DEFINE_ERROR_RANGE(RomNcaFileSystemCorrupted, 4051, 4069);
                        R_DEFINE_ERROR_RESULT(InvalidRomNcaFileSystemType,                 4052);
                        R_DEFINE_ERROR_RESULT(InvalidRomAcidFileSize,                      4053);
                        R_DEFINE_ERROR_RESULT(InvalidRomAcidSize,                          4054);
                        R_DEFINE_ERROR_RESULT(InvalidRomAcid,                              4055);
                        R_DEFINE_ERROR_RESULT(RomAcidVerificationFailed,                   4056);
                        R_DEFINE_ERROR_RESULT(InvalidRomNcaSignature,                      4057);
                        R_DEFINE_ERROR_RESULT(RomNcaHeaderSignature1VerificationFailed,    4058);
                        R_DEFINE_ERROR_RESULT(RomNcaHeaderSignature2VerificationFailed,    4059);
                        R_DEFINE_ERROR_RESULT(RomNcaFsHeaderHashVerificationFailed,        4060);
                        R_DEFINE_ERROR_RESULT(InvalidRomNcaKeyIndex,                       4061);
                        R_DEFINE_ERROR_RESULT(InvalidRomNcaFsHeaderHashType,               4062);
                        R_DEFINE_ERROR_RESULT(InvalidRomNcaFsHeaderEncryptionType,         4063);

                    R_DEFINE_ERROR_RANGE(RomNcaHierarchicalSha256StorageCorrupted, 4071, 4079);
                        R_DEFINE_ERROR_RESULT(InvalidRomHierarchicalSha256BlockSize,       4072);
                        R_DEFINE_ERROR_RESULT(InvalidRomHierarchicalSha256LayerCount,      4073);
                        R_DEFINE_ERROR_RESULT(RomHierarchicalSha256BaseStorageTooLarge,    4074);
                        R_DEFINE_ERROR_RESULT(RomHierarchicalSha256HashVerificationFailed, 4075);

                R_DEFINE_ERROR_RANGE(RomIntegrityVerificationStorageCorrupted, 4141, 4179);
                     R_DEFINE_ERROR_RESULT(IncorrectRomIntegrityVerificationMagic,                4142);
                     R_DEFINE_ERROR_RESULT(InvalidRomZeroHash,                                    4143);
                     R_DEFINE_ERROR_RESULT(RomNonRealDataVerificationFailed,                      4144);
                     R_DEFINE_ERROR_RESULT(InvalidRomHierarchicalIntegrityVerificationLayerCount, 4145);

                     R_DEFINE_ERROR_RANGE(RomRealDataVerificationFailed, 4151, 4159);
                         R_DEFINE_ERROR_RESULT(ClearedRomRealDataVerificationFailed,   4152);
                         R_DEFINE_ERROR_RESULT(UnclearedRomRealDataVerificationFailed, 4153);

                R_DEFINE_ERROR_RANGE(RomPartitionFileSystemCorrupted, 4181, 4199);
                    R_DEFINE_ERROR_RESULT(InvalidRomSha256PartitionHashTarget,           4182);
                    R_DEFINE_ERROR_RESULT(RomSha256PartitionHashVerificationFailed,      4183);
                    R_DEFINE_ERROR_RESULT(RomPartitionSignatureVerificationFailed,       4184);
                    R_DEFINE_ERROR_RESULT(RomSha256PartitionSignatureVerificationFailed, 4185);
                    R_DEFINE_ERROR_RESULT(InvalidRomPartitionEntryOffset,                4186);
                    R_DEFINE_ERROR_RESULT(InvalidRomSha256PartitionMetaDataSize,         4187);

                R_DEFINE_ERROR_RANGE(RomBuiltInStorageCorrupted, 4201, 4219);
                    R_DEFINE_ERROR_RESULT(RomGptHeaderVerificationFailed, 4202);

                R_DEFINE_ERROR_RANGE(RomHostFileSystemCorrupted, 4241, 4259);
                    R_DEFINE_ERROR_RESULT(RomHostEntryCorrupted,    4242);
                    R_DEFINE_ERROR_RESULT(RomHostFileDataCorrupted, 4243);
                    R_DEFINE_ERROR_RESULT(RomHostFileCorrupted,     4244);
                    R_DEFINE_ERROR_RESULT(InvalidRomHostHandle,     4245);

                R_DEFINE_ERROR_RANGE(RomDatabaseCorrupted, 4261, 4279);
                    R_DEFINE_ERROR_RESULT(InvalidRomAllocationTableBlock,     4262);
                    R_DEFINE_ERROR_RESULT(InvalidRomKeyValueListElementIndex, 4263);

            R_DEFINE_ERROR_RANGE(SaveDataCorrupted, 4301, 4499);
            R_DEFINE_ERROR_RANGE(NcaCorrupted, 4501, 4599);
                R_DEFINE_ERROR_RESULT(NcaBaseStorageOutOfRangeA, 4508);
                R_DEFINE_ERROR_RESULT(NcaBaseStorageOutOfRangeB, 4509);

                R_DEFINE_ERROR_RANGE(NcaFileSystemCorrupted, 4511, 4529);
                    R_DEFINE_ERROR_RESULT(InvalidNcaFileSystemType,              4512);
                    R_DEFINE_ERROR_RESULT(InvalidAcidFileSize,                   4513);
                    R_DEFINE_ERROR_RESULT(InvalidAcidSize,                       4514);
                    R_DEFINE_ERROR_RESULT(InvalidAcid,                           4515);
                    R_DEFINE_ERROR_RESULT(AcidVerificationFailed,                4516);
                    R_DEFINE_ERROR_RESULT(InvalidNcaSignature,                   4517);
                    R_DEFINE_ERROR_RESULT(NcaHeaderSignature1VerificationFailed, 4518);
                    R_DEFINE_ERROR_RESULT(NcaHeaderSignature2VerificationFailed, 4519);
                    R_DEFINE_ERROR_RESULT(NcaFsHeaderHashVerificationFailed,     4520);
                    R_DEFINE_ERROR_RESULT(InvalidNcaKeyIndex,                    4521);
                    R_DEFINE_ERROR_RESULT(InvalidNcaFsHeaderHashType,            4522);
                    R_DEFINE_ERROR_RESULT(InvalidNcaFsHeaderEncryptionType,      4523);
                    R_DEFINE_ERROR_RESULT(InvalidNcaPatchInfoIndirectSize,       4524);
                    R_DEFINE_ERROR_RESULT(InvalidNcaPatchInfoAesCtrExSize,       4525);
                    R_DEFINE_ERROR_RESULT(InvalidNcaPatchInfoAesCtrExOffset,     4526);
                    R_DEFINE_ERROR_RESULT(InvalidNcaId,                          4527);
                    R_DEFINE_ERROR_RESULT(InvalidNcaHeader,                      4528);
                    R_DEFINE_ERROR_RESULT(InvalidNcaFsHeader,                    4529);

                R_DEFINE_ERROR_RANGE(NcaHierarchicalSha256StorageCorrupted, 4531, 4539);
                    R_DEFINE_ERROR_RESULT(InvalidHierarchicalSha256BlockSize,       4532);
                    R_DEFINE_ERROR_RESULT(InvalidHierarchicalSha256LayerCount,      4533);
                    R_DEFINE_ERROR_RESULT(HierarchicalSha256BaseStorageTooLarge,    4534);
                    R_DEFINE_ERROR_RESULT(HierarchicalSha256HashVerificationFailed, 4535);

                /* TODO: Range? */
                R_DEFINE_ERROR_RESULT(InvalidNcaHeader1SignatureKeyGeneration, 4543);

            R_DEFINE_ERROR_RANGE(IntegrityVerificationStorageCorrupted, 4601, 4639);
                 R_DEFINE_ERROR_RESULT(IncorrectIntegrityVerificationMagic,                4602);
                 R_DEFINE_ERROR_RESULT(InvalidZeroHash,                                    4603);
                 R_DEFINE_ERROR_RESULT(NonRealDataVerificationFailed,                      4604);
                 R_DEFINE_ERROR_RESULT(InvalidHierarchicalIntegrityVerificationLayerCount, 4605);

                 R_DEFINE_ERROR_RANGE(RealDataVerificationFailed, 4611, 4619);
                     R_DEFINE_ERROR_RESULT(ClearedRealDataVerificationFailed,   4612);
                     R_DEFINE_ERROR_RESULT(UnclearedRealDataVerificationFailed, 4613);

            R_DEFINE_ERROR_RANGE(PartitionFileSystemCorrupted, 4641, 4659);
                R_DEFINE_ERROR_RESULT(InvalidSha256PartitionHashTarget,           4642);
                R_DEFINE_ERROR_RESULT(Sha256PartitionHashVerificationFailed,      4643);
                R_DEFINE_ERROR_RESULT(PartitionSignatureVerificationFailed,       4644);
                R_DEFINE_ERROR_RESULT(Sha256PartitionSignatureVerificationFailed, 4645);
                R_DEFINE_ERROR_RESULT(InvalidPartitionEntryOffset,                4646);
                R_DEFINE_ERROR_RESULT(InvalidSha256PartitionMetaDataSize,         4647);

            R_DEFINE_ERROR_RANGE(BuiltInStorageCorrupted, 4661, 4679);
                R_DEFINE_ERROR_RESULT(GptHeaderVerificationFailed, 4662);

            R_DEFINE_ERROR_RANGE(FatFileSystemCorrupted, 4681, 4699);
                R_DEFINE_ERROR_RESULT(ExFatUnavailable, 4685);

            R_DEFINE_ERROR_RANGE(HostFileSystemCorrupted, 4701, 4719);
                R_DEFINE_ERROR_RESULT(HostEntryCorrupted,    4702);
                R_DEFINE_ERROR_RESULT(HostFileDataCorrupted, 4703);
                R_DEFINE_ERROR_RESULT(HostFileCorrupted,     4704);
                R_DEFINE_ERROR_RESULT(InvalidHostHandle,     4705);

            R_DEFINE_ERROR_RANGE(DatabaseCorrupted, 4721, 4739);
                R_DEFINE_ERROR_RESULT(InvalidAllocationTableBlock,     4722);
                R_DEFINE_ERROR_RESULT(InvalidKeyValueListElementIndex, 4723);

            R_DEFINE_ERROR_RANGE(AesXtsFileSystemCorrupted, 4741, 4759);
            R_DEFINE_ERROR_RANGE(SaveDataTransferDataCorrupted, 4761, 4769);
            R_DEFINE_ERROR_RANGE(SignedSystemPartitionDataCorrupted, 4771, 4779);

            R_DEFINE_ERROR_RESULT(GameCardLogoDataCorrupted, 4781);

        R_DEFINE_ERROR_RANGE(Unexpected, 5000, 5999);
            R_DEFINE_ERROR_RESULT(UnexpectedInAesCtrStorageA,  5315);
            R_DEFINE_ERROR_RESULT(UnexpectedInAesXtsStorageA,  5316);
            R_DEFINE_ERROR_RESULT(UnexpectedInFindFileSystemA, 5319);

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

                R_DEFINE_ERROR_RESULT(InvalidOpenMode, 6072);

                R_DEFINE_ERROR_RANGE(InvalidEnumValue,  6080, 6099);
                    R_DEFINE_ERROR_RESULT(InvalidSaveDataState, 6081);
                    R_DEFINE_ERROR_RESULT(InvalidSaveDataSpaceId, 6082);

                R_DEFINE_ERROR_RANGE(InvalidOperationForOpenMode, 6200, 6299);
                    R_DEFINE_ERROR_RESULT(FileExtensionWithoutOpenModeAllowAppend, 6201);
                    R_DEFINE_ERROR_RESULT(ReadNotPermitted,                        6202);
                    R_DEFINE_ERROR_RESULT(WriteNotPermitted,                       6203);

                R_DEFINE_ERROR_RANGE(UnsupportedOperation, 6300, 6399);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInSubStorageA,                                6302);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInSubStorageB,                                6303);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInMemoryStorageA,                             6304);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInMemoryStorageB,                             6305);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInFileStorageA,                               6306);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInFileStorageB,                               6307);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInSwitchStorageA,                             6308);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInAesCtrCounterExtendedStorageA,              6310);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInAesCtrCounterExtendedStorageB,              6311);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInAesCtrCounterExtendedStorageC,              6312);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInAesCtrStorageExternalA,                     6313);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInAesCtrStorageExternalB,                     6314);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInAesCtrStorageA,                             6315);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInHierarchicalIntegrityVerificationStorageA,  6316);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInHierarchicalIntegrityVerificationStorageB,  6317);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInIntegrityVerificationStorageA,              6318);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInIntegrityVerificationStorageB,              6319);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInIntegrityVerificationStorageC,              6320);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInBlockCacheBufferedStorageA,                 6321);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInBlockCacheBufferedStorageB,                 6322);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInBlockCacheBufferedStorageC,                 6323);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInIndirectStorageA,                           6324);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInIndirectStorageB,                           6325);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInIndirectStorageC,                           6326);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInZeroStorageA,                               6327);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInZeroStorageB,                               6328);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInHierarchicalSha256StorageA,                 6329);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInReadOnlyBlockCacheStorageA,                 6330);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInReadOnlyBlockCacheStorageB,                 6331);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInIntegrityRomFsStorageA    ,                 6332);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInFileServiceObjectAdapterA,                  6362);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInRomFsFileSystemA,                           6364);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInRomFsFileSystemB,                           6365);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInRomFsFileSystemC,                           6366);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInRomFsFileA,                                 6367);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInRomFsFileB,                                 6368);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInReadOnlyFileSystemTemplateA,                6369);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInReadOnlyFileSystemTemplateB,                6370);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInReadOnlyFileSystemTemplateC,                6371);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInReadOnlyFileA,                              6372);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInReadOnlyFileB,                              6373);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInPartitionFileSystemA,                       6374);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInPartitionFileSystemB,                       6375);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInPartitionFileA,                             6376);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperationInPartitionFileB,                             6377);

                R_DEFINE_ERROR_RANGE(PermissionDenied, 6400, 6449);

                R_DEFINE_ERROR_RESULT(NeedFlush, 6454);
                R_DEFINE_ERROR_RESULT(FileNotClosed, 6455);
                R_DEFINE_ERROR_RESULT(DirectoryNotClosed, 6456);
                R_DEFINE_ERROR_RESULT(WriteModeFileNotClosed, 6457);
                R_DEFINE_ERROR_RESULT(AllocatorAlreadyRegistered, 6458);
                R_DEFINE_ERROR_RESULT(DefaultAllocatorUsed, 6459);
                R_DEFINE_ERROR_RESULT(AllocatorAlignmentViolation,  6461);
                R_DEFINE_ERROR_RESULT(UserNotExist,           6465);

        R_DEFINE_ERROR_RANGE(NotFound, 6600, 6699);

        R_DEFINE_ERROR_RANGE(OutOfResource, 6700, 6799);
            R_DEFINE_ERROR_RESULT(BufferAllocationFailed, 6705);
            R_DEFINE_ERROR_RESULT(MappingTableFull,       6706);
            R_DEFINE_ERROR_RESULT(OpenCountLimit,         6709);

        R_DEFINE_ERROR_RANGE(MappingFailed, 6800, 6899);
            R_DEFINE_ERROR_RESULT(MapFull,  6811);

        R_DEFINE_ERROR_RANGE(BadState, 6900, 6999);
            R_DEFINE_ERROR_RESULT(NotInitialized, 6902);
            R_DEFINE_ERROR_RESULT(NotMounted,     6905);


        R_DEFINE_ERROR_RANGE(DbmNotFound, 7901, 7904);
            R_DEFINE_ERROR_RESULT(DbmKeyNotFound,       7902);
            R_DEFINE_ERROR_RESULT(DbmFileNotFound,      7903);
            R_DEFINE_ERROR_RESULT(DbmDirectoryNotFound,  7904);

        R_DEFINE_ERROR_RESULT(DbmAlreadyExists,         7906);
        R_DEFINE_ERROR_RESULT(DbmKeyFull,               7907);
        R_DEFINE_ERROR_RESULT(DbmDirectoryEntryFull,    7908);
        R_DEFINE_ERROR_RESULT(DbmFileEntryFull,         7909);

        R_DEFINE_ERROR_RANGE(DbmFindFinished, 7910, 7912);
            R_DEFINE_ERROR_RESULT(DbmFindKeyFinished,   7911);
            R_DEFINE_ERROR_RESULT(DbmIterationFinished, 7912);

        R_DEFINE_ERROR_RESULT(DbmInvalidOperation,      7914);
        R_DEFINE_ERROR_RESULT(DbmInvalidPathFormat,     7915);
        R_DEFINE_ERROR_RESULT(DbmDirectoryNameTooLong,  7916);
        R_DEFINE_ERROR_RESULT(DbmFileNameTooLong,       7917);

}
