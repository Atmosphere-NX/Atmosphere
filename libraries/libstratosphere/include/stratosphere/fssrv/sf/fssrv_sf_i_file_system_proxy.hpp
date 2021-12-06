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
#include <vapours.hpp>
#include <stratosphere/sf.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_ifilesystem.hpp>

#define AMS_FSSRV_I_FILE_SYSTEM_PROXY_INTERFACE_INFO(C, H)                                                                                                              \
    /* AMS_SF_METHOD_INFO(C, H,    0, Result, OpenFileSystem,                                              (TODO), (TODO), hos::Version_Min,    hos::Version_1_0_0)  */ \
    /* AMS_SF_METHOD_INFO(C, H,    1, Result, SetCurrentProcess,                                           (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,    2, Result, OpenDataFileSystemByCurrentProcess,                          (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,    7, Result, OpenFileSystemWithPatch,                                     (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,    8, Result, OpenFileSystemWithId,                                        (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,    9, Result, OpenDataFileSystemByProgramId,                               (TODO), (TODO), hos::Version_3_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   11, Result, OpenBisFileSystem,                                           (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   12, Result, OpenBisStorage,                                              (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   13, Result, InvalidateBisCache,                                          (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   17, Result, OpenHostFileSystem,                                          (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   18, Result, OpenSdCardFileSystem,                                        (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   19, Result, FormatSdCardFileSystem,                                      (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   21, Result, DeleteSaveDataFileSystem,                                    (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   22, Result, CreateSaveDataFileSystem,                                    (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   23, Result, CreateSaveDataFileSystemBySystemSaveDataId,                  (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   24, Result, RegisterSaveDataFileSystemAtomicDeletion,                    (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   25, Result, DeleteSaveDataFileSystemBySaveDataSpaceId,                   (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   26, Result, FormatSdCardDryRun,                                          (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   27, Result, IsExFatSupported,                                            (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   28, Result, DeleteSaveDataFileSystemBySaveDataAttribute,                 (TODO), (TODO), hos::Version_4_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   30, Result, OpenGameCardStorage,                                         (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   31, Result, OpenGameCardFileSystem,                                      (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   32, Result, ExtendSaveDataFileSystem,                                    (TODO), (TODO), hos::Version_3_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   33, Result, DeleteCacheStorage,                                          (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   34, Result, GetCacheStorageSize,                                         (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   35, Result, CreateSaveDataFileSystemWithHashSalt,                        (TODO), (TODO), hos::Version_6_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   36, Result, OpenHostFileSystemWithOption,                                (TODO), (TODO), hos::Version_9_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   51, Result, OpenSaveDataFileSystem,                                      (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   52, Result, OpenSaveDataFileSystemBySystemSaveDataId,                    (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   53, Result, OpenReadOnlySaveDataFileSystem,                              (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   57, Result, ReadSaveDataFileSystemExtraDataBySaveDataSpaceId,            (TODO), (TODO), hos::Version_3_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   58, Result, ReadSaveDataFileSystemExtraData,                             (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   59, Result, WriteSaveDataFileSystemExtraData,                            (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   60, Result, OpenSaveDataInfoReader,                                      (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   61, Result, OpenSaveDataInfoReaderBySaveDataSpaceId,                     (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   62, Result, OpenSaveDataInfoReaderOnlyCacheStorage,                      (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   64, Result, OpenSaveDataInternalStorageFileSystem,                       (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   65, Result, UpdateSaveDataMacForDebug,                                   (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   66, Result, WriteSaveDataFileSystemExtraDataWithMask,                    (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   67, Result, FindSaveDataWithFilter,                                      (TODO), (TODO), hos::Version_6_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   68, Result, OpenSaveDataInfoReaderWithFilter,                            (TODO), (TODO), hos::Version_6_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   69, Result, ReadSaveDataFileSystemExtraDataBySaveDataAttribute,          (TODO), (TODO), hos::Version_8_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   70, Result, WriteSaveDataFileSystemExtraDataWithMaskBySaveDataAttribute, (TODO), (TODO), hos::Version_8_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   71, Result, ReadSaveDataFileSystemExtraDataWithMaskBySaveDataAttribute,  (TODO), (TODO), hos::Version_10_0_0)                      */ \
    /* AMS_SF_METHOD_INFO(C, H,   80, Result, OpenSaveDataMetaFile,                                        (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,   81, Result, OpenSaveDataTransferManager,                                 (TODO), (TODO), hos::Version_4_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   82, Result, OpenSaveDataTransferManagerVersion2,                         (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   83, Result, OpenSaveDataTransferProhibiter,                              (TODO), (TODO), hos::Version_6_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   84, Result, ListAccessibleSaveDataOwnerId,                               (TODO), (TODO), hos::Version_6_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   85, Result, OpenSaveDataTransferManagerForSaveDataRepair,                (TODO), (TODO), hos::Version_9_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,   86, Result, OpenSaveDataMover,                                           (TODO), (TODO), hos::Version_10_0_0)                      */ \
    /* AMS_SF_METHOD_INFO(C, H,   87, Result, OpenSaveDataTransferManagerForRepair,                        (TODO), (TODO), hos::Version_11_0_0)                      */ \
    /* AMS_SF_METHOD_INFO(C, H,  100, Result, OpenImageDirectoryFileSystem,                                (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  101, Result, OpenBaseFileSystem,                                          (TODO), (TODO), hos::Version_11_0_0)                      */ \
    /* AMS_SF_METHOD_INFO(C, H,  102, Result, FormatBaseFileSystem,                                        (TODO), (TODO), hos::Version_12_0_0)                      */ \
    /* AMS_SF_METHOD_INFO(C, H,  110, Result, OpenContentStorageFileSystem,                                (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  120, Result, OpenCloudBackupWorkStorageFileSystem,                        (TODO), (TODO), hos::Version_6_0_0,  hos::Version_9_2_0)  */ \
    /* AMS_SF_METHOD_INFO(C, H,  130, Result, OpenCustomStorageFileSystem,                                 (TODO), (TODO), hos::Version_7_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  200, Result, OpenDataStorageByCurrentProcess,                             (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  201, Result, OpenDataStorageByProgramId,                                  (TODO), (TODO), hos::Version_3_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  202, Result, OpenDataStorageByDataId,                                     (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  203, Result, OpenPatchDataStorageByCurrentProcess,                        (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  204, Result, OpenDataFileSystemWithProgramIndex,                          (TODO), (TODO), hos::Version_7_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  205, Result, OpenDataStorageWithProgramIndex,                             (TODO), (TODO), hos::Version_7_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  206, Result, OpenDataStorageByPath,                                       (TODO), (TODO), hos::Version_13_0_0)                      */ \
    /* AMS_SF_METHOD_INFO(C, H,  400, Result, OpenDeviceOperator,                                          (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  500, Result, OpenSdCardDetectionEventNotifier,                            (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  501, Result, OpenGameCardDetectionEventNotifier,                          (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  510, Result, OpenSystemDataUpdateEventNotifier,                           (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  511, Result, NotifySystemDataUpdateEvent,                                 (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  520, Result, SimulateDeviceDetectionEvent,                                (TODO), (TODO), hos::Version_6_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  600, Result, SetCurrentPosixTime,                                         (TODO), (TODO), hos::Version_Min,    hos::Version_3_0_2)  */ \
    /* AMS_SF_METHOD_INFO(C, H,  601, Result, QuerySaveDataTotalSize,                                      (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  602, Result, VerifySaveDataFileSystem,                                    (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  603, Result, CorruptSaveDataFileSystem,                                   (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  604, Result, CreatePaddingFile,                                           (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  605, Result, DeleteAllPaddingFiles,                                       (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H,  606, Result, GetRightsId,                                                 (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  607, Result, RegisterExternalKey,                                         (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  608, Result, UnregisterAllExternalKey,                                    (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  609, Result, GetRightsIdByPath,                                           (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  610, Result, GetRightsIdAndKeyGenerationByPath,                           (TODO), (TODO), hos::Version_3_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  611, Result, SetCurrentPosixTimeWithTimeDifference,                       (TODO), (TODO), hos::Version_4_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  612, Result, GetFreeSpaceSizeForSaveData,                                 (TODO), (TODO), hos::Version_4_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  613, Result, VerifySaveDataFileSystemBySaveDataSpaceId,                   (TODO), (TODO), hos::Version_4_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  614, Result, CorruptSaveDataFileSystemBySaveDataSpaceId,                  (TODO), (TODO), hos::Version_4_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  615, Result, QuerySaveDataInternalStorageTotalSize,                       (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  616, Result, GetSaveDataCommitId,                                         (TODO), (TODO), hos::Version_6_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  617, Result, UnregisterExternalKey,                                       (TODO), (TODO), hos::Version_7_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  620, Result, SetSdCardEncryptionSeed,                                     (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  630, Result, SetSdCardAccessibility,                                      (TODO), (TODO), hos::Version_4_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  631, Result, IsSdCardAccessible,                                          (TODO), (TODO), hos::Version_4_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  640, Result, IsSignedSystemPartitionOnSdCardValid,                        (TODO), (TODO), hos::Version_4_0_0,  hos::Version_7_0_1)  */ \
    /* AMS_SF_METHOD_INFO(C, H,  700, Result, OpenAccessFailureDetectionEventNotifier,                     (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  701, Result, GetAccessFailureDetectionEvent,                              (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  702, Result, IsAccessFailureDetected,                                     (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  710, Result, ResolveAccessFailure,                                        (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  720, Result, AbandonAccessFailure,                                        (TODO), (TODO), hos::Version_5_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  800, Result, GetAndClearErrorInfo,                                        (TODO), (TODO), hos::Version_2_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H,  810, Result, RegisterProgramIndexMapInfo,                                 (TODO), (TODO), hos::Version_7_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H, 1000, Result, SetBisRootForHost,                                           (TODO), (TODO), hos::Version_Min,    hos::Version_9_2_0)  */ \
    /* AMS_SF_METHOD_INFO(C, H, 1001, Result, SetSaveDataSize,                                             (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H, 1002, Result, SetSaveDataRootPath,                                         (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H, 1003, Result, DisableAutoSaveDataCreation,                                 (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H, 1004, Result, SetGlobalAccessLogMode,                                      (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H, 1005, Result, GetGlobalAccessLogMode,                                      (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H, 1006, Result, OutputAccessLogToSdCard,                                     (TODO), (TODO))                                           */ \
    /* AMS_SF_METHOD_INFO(C, H, 1007, Result, RegisterUpdatePartition,                                     (TODO), (TODO), hos::Version_4_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H, 1008, Result, OpenRegisteredUpdatePartition,                               (TODO), (TODO), hos::Version_4_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H, 1009, Result, GetAndClearMemoryReportInfo,                                 (TODO), (TODO), hos::Version_4_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H, 1010, Result, SetDataStorageRedirectTarget,                                (TODO), (TODO), hos::Version_5_1_0, hos::Version_6_2_0)   */ \
    /* AMS_SF_METHOD_INFO(C, H, 1011, Result, GetProgramIndexForAccessLog,                                 (TODO), (TODO), hos::Version_7_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H, 1012, Result, GetFsStackUsage,                                             (TODO), (TODO), hos::Version_9_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H, 1013, Result, UnsetSaveDataRootPath,                                       (TODO), (TODO), hos::Version_9_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H, 1014, Result, OutputMultiProgramTagAccessLog,                              (TODO), (TODO), hos::Version_10_0_0, hos::Version_10_2_0) */ \
    /* AMS_SF_METHOD_INFO(C, H, 1016, Result, FlushAccessLogOnSdCard,                                      (TODO), (TODO), hos::Version_11_0_0)                      */ \
    /* AMS_SF_METHOD_INFO(C, H, 1017, Result, OutputApplicationInfoAccessLog,                              (TODO), (TODO), hos::Version_11_0_0)                      */ \
    /* AMS_SF_METHOD_INFO(C, H, 1018, Result, SetDebugOption,                                              (TODO), (TODO), hos::Version_13_0_0)                      */ \
    /* AMS_SF_METHOD_INFO(C, H, 1019, Result, UnsetDebugOption,                                            (TODO), (TODO), hos::Version_13_0_0)                      */ \
    /* AMS_SF_METHOD_INFO(C, H, 1100, Result, OverrideSaveDataTransferTokenSignVerificationKey,            (TODO), (TODO), hos::Version_4_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H, 1110, Result, CorruptSaveDataFileSystemByOffset,                           (TODO), (TODO), hos::Version_6_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H, 1200, Result, OpenMultiCommitManager,                                      (TODO), (TODO), hos::Version_6_0_0)                       */ \
    /* AMS_SF_METHOD_INFO(C, H, 1300, Result, OpenBisWiper,                                                (TODO), (TODO), hos::Version_10_0_0)                      */

AMS_SF_DEFINE_INTERFACE(ams::fssrv::sf, IFileSystemProxy, AMS_FSSRV_I_FILE_SYSTEM_PROXY_INTERFACE_INFO)
