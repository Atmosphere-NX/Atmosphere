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

R_DEFINE_NAMESPACE_RESULT_MODULE(ams::fs, 2);

namespace ams::fs {

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
        R_DEFINE_ERROR_RESULT(PartitionNotFound,      1001);
        R_DEFINE_ERROR_RESULT(TargetNotFound,         1002);
        R_DEFINE_ERROR_RESULT(NcaExternalKeyNotFound, 1004);

        R_DEFINE_ERROR_RANGE(SdCardAccessFailed, 2000, 2499);
            R_DEFINE_ERROR_RESULT(SdCardNotPresent, 2001);

        R_DEFINE_ERROR_RANGE(GameCardAccessFailed, 2500, 2999);
            R_DEFINE_ERROR_RESULT(GameCardPreconditionViolation, 2503);

            R_DEFINE_ERROR_RANGE(GameCardCardAccessFailure, 2530, 2559);
                R_DEFINE_ERROR_RANGE(CameCardWrongCard, 2543, 2546);
                    R_DEFINE_ERROR_RESULT(GameCardInitialDataMismatch,      2544);
                    R_DEFINE_ERROR_RESULT(GameCardInitialNotFilledWithZero, 2545);
                    R_DEFINE_ERROR_RESULT(GameCardKekIndexMismatch,         2546);

                R_DEFINE_ERROR_RESULT(GameCardInvalidCardHeader,        2554);
                R_DEFINE_ERROR_RESULT(GameCardInvalidT1CardCertificate, 2555);
                R_DEFINE_ERROR_RESULT(GameCardInvalidCa10Certificate,   2557);

            R_DEFINE_ERROR_RANGE(GameCardSplFailure, 2665, 2669);
                R_DEFINE_ERROR_RESULT(GameCardSplDecryptAesKeyFailure, 2666);

        R_DEFINE_ERROR_RESULT(NotImplemented,     3001);
        R_DEFINE_ERROR_RESULT(UnsupportedVersion, 3002);
        R_DEFINE_ERROR_RESULT(OutOfRange,         3005);

        R_DEFINE_ERROR_RESULT(SystemPartitionNotReady, 3100);

        R_DEFINE_ERROR_RANGE(AllocationMemoryFailed, 3200, 3499);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFatFileSystemA,                         3201);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFatFileSystemC,                         3203);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFatFileSystemD,                         3204);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFatFileSystemE,                         3205);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFatFileSystemF,                         3206);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFatFileSystemH,                         3208);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFileSystemAccessorA,                    3211);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFileSystemAccessorB,                    3212);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInApplicationA,                           3213);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInBcatSaveDataA,                          3214);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInBisA,                                   3215);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInBisB,                                   3216);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInBisC,                                   3217);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInCodeA,                                  3218);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInContentA,                               3219);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInContentStorageA,                        3220);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInContentStorageB,                        3221);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInDataA,                                  3222);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInDataB,                                  3223);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInDeviceSaveDataA,                        3224);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardA,                              3225);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardB,                              3226);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardC,                              3227);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardD,                              3228);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInHostA,                                  3229);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInHostB,                                  3230);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInHostC,                                  3231);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInImageDirectoryA,                        3232);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInLogoA,                                  3233);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomA,                                   3234);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomB,                                   3235);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomC,                                   3236);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomD,                                   3237);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomE,                                   3238);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomF,                                   3239);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataManagementA,                    3242);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataThumbnailA,                     3243);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSdCardA,                                3244);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSdCardB,                                3245);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSystemSaveDataA,                        3246);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomFsFileSystemA,                       3247);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomFsFileSystemB,                       3248);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomFsFileSystemC,                       3249);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGuidPartitionTableA,                    3251);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInDeviceDetectionEventManagerA,           3252);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataFileSystemServiceImplA,         3253);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFileSystemProxyCoreImplB,               3254);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSdCardProxyFileSystemCreatorA,          3255);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInNcaFileSystemServiceImplA,              3256);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInNcaFileSystemServiceImplB,              3257);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInProgramRegistryManagerA,                3258);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSdmmcStorageServiceA,                   3259);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInBuiltInStorageCreatorA,                 3260);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInBuiltInStorageCreatorB,                 3261);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInBuiltInStorageCreatorC,                 3262);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedFatFileSystemWithBufferA,                 3264);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFatFileSystemCreatorA,                  3265);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFatFileSystemCreatorB,                  3266);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardFileSystemCreatorA,             3267);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardFileSystemCreatorB,             3268);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardFileSystemCreatorC,             3269);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardFileSystemCreatorD,             3270);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardFileSystemCreatorE,             3271);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardFileSystemCreatorF,             3272);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardManagerA,                       3273);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardManagerB,                       3274);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardManagerC,                       3275);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardManagerD,                       3276);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardManagerE,                       3277);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardManagerF,                       3278);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInLocalFileSystemCreatorA,                3279);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInPartitionFileSystemCreatorA,            3280);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomFileSystemCreatorA,                  3281);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataFileSystemCreatorA,             3282);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataFileSystemCreatorB,             3283);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataFileSystemCreatorC,             3284);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataFileSystemCreatorD,             3285);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataFileSystemCreatorE,             3286);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInStorageOnNcaCreatorA,                   3288);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInStorageOnNcaCreatorB,                   3289);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSubDirectoryFileSystemCreatorA,         3290);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInTargetManagerFileSystemCreatorA,        3291);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataIndexerA,                       3292);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataIndexerB,                       3293);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFileSystemBuddyHeapA,                   3294);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFileSystemBufferManagerA,               3295);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInBlockCacheBufferedStorageA,             3296);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInBlockCacheBufferedStorageB,             3297);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInDuplexStorageA,                         3298);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInIntegrityVerificationStorageA,          3304);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInIntegrityVerificationStorageB,          3305);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInJournalStorageA,                        3306);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInJournalStorageB,                        3307);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataFileSystemCoreA,                3310);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataFileSystemCoreB,                3311);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInAesXtsFileA,                            3312);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInAesXtsFileB,                            3313);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInAesXtsFileC,                            3314);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInAesXtsFileD,                            3315);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInAesXtsFileSystemA,                      3316);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInConcatenationFileSystemA,               3319);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInConcatenationFileSystemB,               3320);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInDirectorySaveDataFileSystemA,           3321);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInLocalFileSystemA,                       3322);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInLocalFileSystemB,                       3323);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInNcaFileSystemDriverI,                   3341);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInPartitionFileSystemA,                   3347);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInPartitionFileSystemB,                   3348);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInPartitionFileSystemC,                   3349);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInPartitionFileSystemMetaA,               3350);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInPartitionFileSystemMetaB,               3351);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomFsFileSystemD,                       3352);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSubdirectoryFileSystemA,                3355);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInTmFileSystemA,                          3356);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInTmFileSystemB,                          3357);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInProxyFileSystemA,                       3359);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInProxyFileSystemB,                       3360);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataExtraDataAccessorCacheManagerA, 3362);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInNcaReaderA,                             3363);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRegisterA,                              3365);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRegisterB,                              3366);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInPathNormalizer,                         3367);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInDbmRomKeyValueStorage,                  3375);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInDbmHierarchicalRomFileTable,            3376);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomFsFileSystemE,                       3377);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInISaveFileSystemA,                       3378);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInISaveFileSystemB,                       3379);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomOnFileA,                             3380);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomOnFileB,                             3381);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInRomOnFileC,                             3382);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInAesXtsFileE,                            3383);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInAesXtsFileF,                            3384);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInAesXtsFileG,                            3385);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInReadOnlyFileSystemA,                    3386);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInEncryptedFileSystemCreatorA,            3394);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInAesCtrCounterExtendedStorageA,          3399);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInAesCtrCounterExtendedStorageB,          3400);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSdmmcStorageServiceB,                   3406);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFileSystemInterfaceAdapterA,            3407);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardFileSystemCreatorG,             3408);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardFileSystemCreatorH,             3409);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInAesXtsFileSystemB,                      3410);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInBufferedStorageA,                       3411);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInIntegrityRomFsStorageA,                 3412);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataFileSystemServiceImplB,         3416);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedNew,                                      3420);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInFileSystemProxyImplA,                   3421);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedMakeUnique,                               3422);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedAllocateShared,                           3423);
            R_DEFINE_ERROR_RESULT(AllocationPooledBufferNotEnoughSize,                            3424);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInWriteThroughCacheStorageA,              3428);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataTransferManagerA,               3429);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInSaveDataTransferManagerB,               3430);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInHtcFileSystemA,                         3431);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInHtcFileSystemB,                         3432);
            R_DEFINE_ERROR_RESULT(AllocationMemoryFailedInGameCardManagerG,                       3433);

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

                    R_DEFINE_ERROR_RESULT(RomNcaInvalidHierarchicalIntegrityVerificationLayerCount, 4081);
                    R_DEFINE_ERROR_RESULT(RomNcaInvalidNcaIndirectStorageOutOfRange,                4082);
                    R_DEFINE_ERROR_RESULT(RomNcaInvalidCompressionInfo,                             4083);
                    R_DEFINE_ERROR_RESULT(RomNcaInvalidPatchMetaDataHashType,                       4084);
                    R_DEFINE_ERROR_RESULT(RomNcaInvalidIntegrityLayerInfoOffset,                    4085);
                    R_DEFINE_ERROR_RESULT(RomNcaInvalidPatchMetaDataHashDataSize,                   4086);
                    R_DEFINE_ERROR_RESULT(RomNcaInvalidPatchMetaDataHashDataOffset,                 4087);
                    R_DEFINE_ERROR_RESULT(RomNcaInvalidPatchMetaDataHashDataHash,                   4088);
                    R_DEFINE_ERROR_RESULT(RomNcaInvalidSparseMetaDataHashType,                      4089);
                    R_DEFINE_ERROR_RESULT(RomNcaInvalidSparseMetaDataHashDataSize,                  4090);
                    R_DEFINE_ERROR_RESULT(RomNcaInvalidSparseMetaDataHashDataOffset,                4091);
                    R_DEFINE_ERROR_RESULT(RomNcaInvalidSparseMetaDataHashDataHash,                  4092);

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
                R_DEFINE_ERROR_RESULT(NcaBaseStorageOutOfRangeC, 4510);
                R_DEFINE_ERROR_RESULT(NcaBaseStorageOutOfRangeD, 4511);

                R_DEFINE_ERROR_RESULT_CLASS_IMPL(NcaFileSystemCorrupted, 4512, 4529);
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

                R_DEFINE_ERROR_RESULT(NcaBaseStorageOutOfRangeE, 4530);

                R_DEFINE_ERROR_RANGE(NcaHierarchicalSha256StorageCorrupted, 4531, 4539);
                    R_DEFINE_ERROR_RESULT(InvalidHierarchicalSha256BlockSize,       4532);
                    R_DEFINE_ERROR_RESULT(InvalidHierarchicalSha256LayerCount,      4533);
                    R_DEFINE_ERROR_RESULT(HierarchicalSha256BaseStorageTooLarge,    4534);
                    R_DEFINE_ERROR_RESULT(HierarchicalSha256HashVerificationFailed, 4535);

                /* TODO: Range? */
                R_DEFINE_ERROR_RESULT(InvalidNcaHierarchicalIntegrityVerificationLayerCount, 4541);
                R_DEFINE_ERROR_RESULT(InvalidNcaIndirectStorageOutOfRange,                   4542);
                R_DEFINE_ERROR_RESULT(InvalidNcaHeader1SignatureKeyGeneration,               4543);

                /* TODO: Range? */
                R_DEFINE_ERROR_RESULT(InvalidCompressedStorageSize,   4547);
                R_DEFINE_ERROR_RESULT(InvalidNcaMetaDataHashDataSize, 4548);
                R_DEFINE_ERROR_RESULT(InvalidNcaMetaDataHashDataHash, 4549);

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
            R_DEFINE_ERROR_RESULT(UnexpectedInLocalFileSystemA,          5305);
            R_DEFINE_ERROR_RESULT(UnexpectedInLocalFileSystemB,          5306);
            R_DEFINE_ERROR_RESULT(UnexpectedInLocalFileSystemC,          5307);
            R_DEFINE_ERROR_RESULT(UnexpectedInLocalFileSystemD,          5308);
            R_DEFINE_ERROR_RESULT(UnexpectedInLocalFileSystemE,          5309);
            R_DEFINE_ERROR_RESULT(UnexpectedInLocalFileSystemF,          5310);
            R_DEFINE_ERROR_RESULT(UnexpectedInPathOnExecutionDirectoryA, 5312);
            R_DEFINE_ERROR_RESULT(UnexpectedInPathOnExecutionDirectoryB, 5313);
            R_DEFINE_ERROR_RESULT(UnexpectedInPathOnExecutionDirectoryC, 5314);
            R_DEFINE_ERROR_RESULT(UnexpectedInAesCtrStorageA,            5315);
            R_DEFINE_ERROR_RESULT(UnexpectedInAesXtsStorageA,            5316);
            R_DEFINE_ERROR_RESULT(UnexpectedInFindFileSystemA,           5319);
            R_DEFINE_ERROR_RESULT(UnexpectedInCompressedStorageA,        5324);
            R_DEFINE_ERROR_RESULT(UnexpectedInCompressedStorageB,        5325);
            R_DEFINE_ERROR_RESULT(UnexpectedInCompressedStorageC,        5326);
            R_DEFINE_ERROR_RESULT(UnexpectedInCompressedStorageD,        5327);
            R_DEFINE_ERROR_RESULT(UnexpectedInPathA,                     5328);

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
                R_DEFINE_ERROR_RESULT(TooLargeSize,    6073);

                R_DEFINE_ERROR_RANGE(InvalidEnumValue,  6080, 6099);
                    R_DEFINE_ERROR_RESULT(InvalidSaveDataState, 6081);
                    R_DEFINE_ERROR_RESULT(InvalidSaveDataSpaceId, 6082);

                R_DEFINE_ERROR_RANGE(InvalidOperationForOpenMode, 6200, 6299);
                    R_DEFINE_ERROR_RESULT(FileExtensionWithoutOpenModeAllowAppend, 6201);
                    R_DEFINE_ERROR_RESULT(ReadNotPermitted,                        6202);
                    R_DEFINE_ERROR_RESULT(WriteNotPermitted,                       6203);

                R_DEFINE_ERROR_RANGE(UnsupportedOperation, 6300, 6399);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForNotResizableSubStorage,                        6302);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForResizableSubStorage,                           6303);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForMemoryStorage,                                 6304);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForMemoryStorage,                            6305);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForFileStorage,                              6306);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForFileHandleStorage,                        6307);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForSwitchStorage,                            6308);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForStorageServiceObjectAdapter,              6309);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForAesCtrCounterExtendedStorage,                    6310);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForAesCtrCounterExtendedStorage,                  6311);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForAesCtrCounterExtendedStorage,             6312);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForAesCtrStorageExternal,                           6313);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForAesCtrStorageExternal,                         6314);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForAesCtrStorage,                                 6315);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForHierarchicalIntegrityVerificationStorage,      6316);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForHierarchicalIntegrityVerificationStorage, 6317);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForIntegrityVerificationStorage,                  6318);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForWritableIntegrityVerificationStorage,     6319);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForIntegrityVerificationStorage,             6320);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForBlockCacheBufferedStorage,                     6321);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForWritableBlockCacheBufferedStorage,        6322);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForBlockCacheBufferedStorage,                6323);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForIndirectStorage,                                 6324);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForIndirectStorage,                               6325);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForIndirectStorage,                          6326);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForZeroStorage,                                     6327);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForZeroStorage,                                   6328);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForHierarchicalSha256Storage,                     6329);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForReadOnlyBlockCacheStorage,                       6330);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForReadOnlyBlockCacheStorage,                     6331);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForIntegrityRomFsStorage,                         6332);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForDuplexStorage,                                 6333);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForDuplexStorage,                            6334);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForHierarchicalDuplexStorage,                     6335);
                    R_DEFINE_ERROR_RESULT(UnsupportedGetSizeForRemapStorage,                                  6336);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForRemapStorage,                                  6337);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForRemapStorage,                             6338);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForIntegritySaveDataStorage,                      6339);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForIntegritySaveDataStorage,                 6340);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForJournalIntegritySaveDataStorage,               6341);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForJournalIntegritySaveDataStorage,          6342);
                    R_DEFINE_ERROR_RESULT(UnsupportedGetSizeForJournalStorage,                                6343);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForJournalStorage,                                6344);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForJournalStorage,                           6345);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForUnionStorage,                                  6346);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForAllocationTableStorage,                        6347);
                    R_DEFINE_ERROR_RESULT(UnsupportedReadForWriteOnlyGameCardStorage,                         6348);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForWriteOnlyGameCardStorage,                      6349);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForReadOnlyGameCardStorage,                         6350);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForReadOnlyGameCardStorage,                       6351);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForReadOnlyGameCardStorage,                  6352);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForSdmmcStorage,                                  6353);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForSdmmcStorage,                             6354);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForFatFile,                                  6355);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForStorageFile,                              6356);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForInternalStorageConcatenationFile,              6357);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForInternalStorageConcatenationFile,         6358);
                    R_DEFINE_ERROR_RESULT(UnsupportedQueryEntryForConcatenationFileSystem,                    6359);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForConcatenationFile,                        6360);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForZeroBitmapFile,                                6361);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForFileServiceObjectAdapter,                 6362);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForAesXtsFile,                               6363);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForRomFsFileSystem,                                 6364);
                    R_DEFINE_ERROR_RESULT(UnsupportedCommitProvisionallyForRomFsFileSystem,                   6365);
                    R_DEFINE_ERROR_RESULT(UnsupportedGetTotalSpaceSizeForRomFsFileSystem,                     6366);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForRomFsFile,                                       6367);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForRomFsFile,                                6368);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForReadOnlyFileSystem,                              6369);
                    R_DEFINE_ERROR_RESULT(UnsupportedCommitProvisionallyForReadOnlyFileSystem,                6370);
                    R_DEFINE_ERROR_RESULT(UnsupportedGetTotalSpaceSizeForReadOnlyFileSystem,                  6371);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForReadOnlyFile,                                    6372);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForReadOnlyFile,                             6373);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForPartitionFileSystem,                             6374);
                    R_DEFINE_ERROR_RESULT(UnsupportedCommitProvisionallyForPartitionFileSystem,               6375);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForPartitionFile,                                   6376);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForPartitionFile,                            6377);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForTmFileSystemFile,                         6378);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForSaveDataInternalStorageFileSystem,               6379);
                    R_DEFINE_ERROR_RESULT(UnsupportedCommitProvisionallyForApplicationTemporaryFileSystem,    6382);
                    R_DEFINE_ERROR_RESULT(UnsupportedCommitProvisionallyForSaveDataFileSystem,                6383);
                    R_DEFINE_ERROR_RESULT(UnsupportedCommitProvisionallyForDirectorySaveDataFileSystem,       6384);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForZeroBitmapHashStorageFile,                       6385);
                    R_DEFINE_ERROR_RESULT(UnsupportedSetSizeForZeroBitmapHashStorageFile,                     6386);
                    R_DEFINE_ERROR_RESULT(UnsupportedWriteForCompressedStorage,                               6387);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForCompressedStorage,                        6388);
                    R_DEFINE_ERROR_RESULT(UnsupportedOperateRangeForRegionSwitchStorage,                      6397);


                R_DEFINE_ERROR_RANGE(PermissionDenied, 6400, 6449);
                    R_DEFINE_ERROR_RESULT(PermissionDeniedForCreateHostFileSystem, 6403);

                R_DEFINE_ERROR_RESULT(PortAcceptableCountLimited, 6450);
                R_DEFINE_ERROR_RESULT(NcaExternalKeyUnregistered, 6451);
                R_DEFINE_ERROR_RESULT(NcaExternalKeyInconsistent, 6452);
                R_DEFINE_ERROR_RESULT(NeedFlush, 6454);
                R_DEFINE_ERROR_RESULT(FileNotClosed, 6455);
                R_DEFINE_ERROR_RESULT(DirectoryNotClosed, 6456);
                R_DEFINE_ERROR_RESULT(WriteModeFileNotClosed, 6457);
                R_DEFINE_ERROR_RESULT(AllocatorAlreadyRegistered, 6458);
                R_DEFINE_ERROR_RESULT(DefaultAllocatorUsed, 6459);
                R_DEFINE_ERROR_RESULT(AllocatorAlignmentViolation,  6461);
                R_DEFINE_ERROR_RESULT(UserNotExist,           6465);

        R_DEFINE_ERROR_RANGE(NotFound, 6600, 6699);
            R_DEFINE_ERROR_RESULT(ProgramInfoNotFound, 6605);

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
            R_DEFINE_ERROR_RESULT(DbmDirectoryNotFound, 7904);

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
