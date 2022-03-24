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
#include <stratosphere/ncm/ncm_ids.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fssrv/impl/fssrv_access_control_bits.hpp>

namespace ams::fssrv::impl {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    #define AMS_FSSRV_FOR_EACH_ACCESS_CONTROL_CAPABILITY(HANDLER, _NS_)                                                                                    \
        HANDLER(CanAbandonAccessFailure,                             _NS_::AccessFailureResolution)                                                        \
        HANDLER(CanChallengeCardExistence,                           _NS_::GameCard)                                                                       \
        HANDLER(CanControlMmcPatrol,                                 _NS_::None)                                                                           \
        HANDLER(CanCorruptSaveData,                                  _NS_::Debug, _NS_::CorruptSaveData)                                                   \
        HANDLER(CanCorruptSystemSaveData,                            _NS_::CorruptSaveData, _NS_::SaveDataManagement, _NS_::SaveDataBackUp)                \
        HANDLER(CanCreateOthersSystemSaveData,                       _NS_::SaveDataBackUp)                                                                 \
        HANDLER(CanCreateOwnSaveData,                                _NS_::CreateOwnSaveData)                                                              \
        HANDLER(CanCreateSaveData,                                   _NS_::CreateSaveData, _NS_::SaveDataBackUp)                                           \
        HANDLER(CanCreateSaveDataWithHashSalt,                       _NS_::None)                                                                           \
        HANDLER(CanCreateSystemSaveData,                             _NS_::SaveDataBackUp, _NS_::SystemSaveData)                                           \
        HANDLER(CanDebugSaveData,                                    _NS_::Debug, _NS_::SaveDataForDebug)                                                  \
        HANDLER(CanDeleteOwnSaveData,                                _NS_::CreateOwnSaveData)                                                              \
        HANDLER(CanDeleteSaveData,                                   _NS_::SaveDataManagement, _NS_::SaveDataBackUp)                                       \
        HANDLER(CanDeleteSystemSaveData,                             _NS_::SystemSaveDataManagement, _NS_::SaveDataBackUp, _NS_::SystemSaveData)           \
        HANDLER(CanEraseMmc,                                         _NS_::BisAllRaw)                                                                      \
        HANDLER(CanExtendOthersSystemSaveData,                       _NS_::SaveDataBackUp)                                                                 \
        HANDLER(CanExtendOwnSaveData,                                _NS_::CreateOwnSaveData)                                                              \
        HANDLER(CanExtendSaveData,                                   _NS_::CreateSaveData, _NS_::SaveDataBackUp)                                           \
        HANDLER(CanExtendSystemSaveData,                             _NS_::SaveDataBackUp, _NS_::SystemSaveData)                                           \
        HANDLER(CanFillBis,                                          _NS_::Debug, _NS_::FillBis)                                                           \
        HANDLER(CanFinalizeGameCardDriver,                           _NS_::GameCardPrivate)                                                                \
        HANDLER(CanFindOwnSaveDataWithFilter,                        _NS_::CreateOwnSaveData)                                                              \
        HANDLER(CanFormatSdCard,                                     _NS_::FormatSdCard)                                                                   \
        HANDLER(CanGetAccessFailureDetectionEvent,                   _NS_::AccessFailureResolution)                                                        \
        HANDLER(CanGetGameCardAsicInfo,                              _NS_::GameCardPrivate)                                                                \
        HANDLER(CanGetGameCardDeviceCertificate,                     _NS_::GameCard)                                                                       \
        HANDLER(CanGetGameCardIdSet,                                 _NS_::GameCard)                                                                       \
        HANDLER(CanGetRightsId,                                      _NS_::GetRightsId)                                                                    \
        HANDLER(CanGetSaveDataCommitId,                              _NS_::SaveDataTransferVersion2, _NS_::SaveDataBackUp)                                 \
        HANDLER(CanInvalidateBisCache,                               _NS_::BisAllRaw)                                                                      \
        HANDLER(CanIsAccessFailureDetected,                          _NS_::AccessFailureResolution)                                                        \
        HANDLER(CanListAccessibleSaveDataOwnerId,                    _NS_::SaveDataTransferVersion2, _NS_::SaveDataTransfer, _NS_::CreateSaveData)         \
        HANDLER(CanMountAllBaseFileSystemRead,                       _NS_::None)                                                                           \
        HANDLER(CanMountAllBaseFileSystemWrite,                      _NS_::None)                                                                           \
        HANDLER(CanMountApplicationPackageRead,                      _NS_::ContentManager, _NS_::ApplicationInfo)                                          \
        HANDLER(CanMountBisCalibrationFileRead,                      _NS_::BisAllRaw, _NS_::Calibration)                                                   \
        HANDLER(CanMountBisCalibrationFileWrite,                     _NS_::BisAllRaw, _NS_::Calibration)                                                   \
        HANDLER(CanMountBisSafeModeRead,                             _NS_::BisAllRaw)                                                                      \
        HANDLER(CanMountBisSafeModeWrite,                            _NS_::BisAllRaw)                                                                      \
        HANDLER(CanMountBisSystemProperEncryptionRead,               _NS_::BisAllRaw)                                                                      \
        HANDLER(CanMountBisSystemProperEncryptionWrite,              _NS_::BisAllRaw)                                                                      \
        HANDLER(CanMountBisSystemProperPartitionRead,                _NS_::BisFileSystem, _NS_::BisAllRaw)                                                 \
        HANDLER(CanMountBisSystemProperPartitionWrite,               _NS_::BisFileSystem, _NS_::BisAllRaw)                                                 \
        HANDLER(CanMountBisSystemRead,                               _NS_::BisFileSystem, _NS_::BisAllRaw)                                                 \
        HANDLER(CanMountBisSystemWrite,                              _NS_::BisFileSystem, _NS_::BisAllRaw)                                                 \
        HANDLER(CanMountBisUserRead,                                 _NS_::BisFileSystem, _NS_::BisAllRaw)                                                 \
        HANDLER(CanMountBisUserWrite,                                _NS_::BisFileSystem, _NS_::BisAllRaw)                                                 \
        HANDLER(CanMountCloudBackupWorkStorageRead,                  _NS_::SaveDataTransferVersion2)                                                       \
        HANDLER(CanMountCloudBackupWorkStorageWrite,                 _NS_::SaveDataTransferVersion2)                                                       \
        HANDLER(CanMountContentControlRead,                          _NS_::ContentManager, _NS_::ApplicationInfo)                                          \
        HANDLER(CanMountContentDataRead,                             _NS_::ContentManager, _NS_::ApplicationInfo)                                          \
        HANDLER(CanMountContentManualRead,                           _NS_::ContentManager, _NS_::ApplicationInfo)                                          \
        HANDLER(CanMountContentMetaRead,                             _NS_::ContentManager, _NS_::ApplicationInfo)                                          \
        HANDLER(CanMountContentStorageRead,                          _NS_::ContentManager)                                                                 \
        HANDLER(CanMountContentStorageWrite,                         _NS_::ContentManager)                                                                 \
        HANDLER(CanMountCustomStorage0Read,                          _NS_::None)                                                                           \
        HANDLER(CanMountCustomStorage0Write,                         _NS_::None)                                                                           \
        HANDLER(CanMountDeviceSaveDataRead,                          _NS_::DeviceSaveData, _NS_::SaveDataBackUp)                                           \
        HANDLER(CanMountDeviceSaveDataWrite,                         _NS_::DeviceSaveData, _NS_::SaveDataBackUp)                                           \
        HANDLER(CanMountGameCardRead,                                _NS_::GameCard)                                                                       \
        HANDLER(CanMountHostRead,                                    _NS_::Debug, _NS_::Host)                                                              \
        HANDLER(CanMountHostWrite,                                   _NS_::Debug, _NS_::Host)                                                              \
        HANDLER(CanMountImageAndVideoStorageRead,                    _NS_::ImageManager)                                                                   \
        HANDLER(CanMountImageAndVideoStorageWrite,                   _NS_::ImageManager)                                                                   \
        HANDLER(CanMountLogoRead,                                    _NS_::ContentManager, _NS_::ApplicationInfo)                                          \
        HANDLER(CanMountOthersSaveDataRead,                          _NS_::SaveDataBackUp)                                                                 \
        HANDLER(CanMountOthersSaveDataWrite,                         _NS_::SaveDataBackUp)                                                                 \
        HANDLER(CanMountOthersSystemSaveDataRead,                    _NS_::SaveDataBackUp)                                                                 \
        HANDLER(CanMountOthersSystemSaveDataWrite,                   _NS_::SaveDataBackUp)                                                                 \
        HANDLER(CanMountRegisteredUpdatePartitionRead,               _NS_::SystemUpdate)                                                                   \
        HANDLER(CanMountSaveDataStorageRead,                         _NS_::None)                                                                           \
        HANDLER(CanMountSaveDataStorageWrite,                        _NS_::None)                                                                           \
        HANDLER(CanMountSdCardRead,                                  _NS_::Debug, _NS_::SdCard)                                                            \
        HANDLER(CanMountSdCardWrite,                                 _NS_::Debug, _NS_::SdCard)                                                            \
        HANDLER(CanMountSystemDataPrivateRead,                       _NS_::SystemData, _NS_::SystemSaveData)                                               \
        HANDLER(CanMountSystemSaveDataRead,                          _NS_::SaveDataBackUp, _NS_::SystemSaveData)                                           \
        HANDLER(CanMountSystemSaveDataWrite,                         _NS_::SaveDataBackUp, _NS_::SystemSaveData)                                           \
        HANDLER(CanMountTemporaryDirectoryRead,                      _NS_::Debug)                                                                          \
        HANDLER(CanMountTemporaryDirectoryWrite,                     _NS_::Debug)                                                                          \
        HANDLER(CanNotifySystemDataUpdateEvent,                      _NS_::SystemUpdate)                                                                   \
        HANDLER(CanOpenAccessFailureDetectionEventNotifier,          _NS_::AccessFailureResolution)                                                        \
        HANDLER(CanOpenBisPartitionBootConfigAndPackage2Part1Read,   _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootConfigAndPackage2Part1Write,  _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootConfigAndPackage2Part2Read,   _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootConfigAndPackage2Part2Write,  _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootConfigAndPackage2Part3Read,   _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootConfigAndPackage2Part3Write,  _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootConfigAndPackage2Part4Read,   _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootConfigAndPackage2Part4Write,  _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootConfigAndPackage2Part5Read,   _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootConfigAndPackage2Part5Write,  _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootConfigAndPackage2Part6Read,   _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootConfigAndPackage2Part6Write,  _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootPartition1RootRead,           _NS_::SystemUpdate, _NS_::BisAllRaw, _NS_::BootModeControl)                           \
        HANDLER(CanOpenBisPartitionBootPartition1RootWrite,          _NS_::SystemUpdate, _NS_::BisAllRaw, _NS_::BootModeControl)                           \
        HANDLER(CanOpenBisPartitionBootPartition2RootRead,           _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionBootPartition2RootWrite,          _NS_::SystemUpdate, _NS_::BisAllRaw)                                                  \
        HANDLER(CanOpenBisPartitionCalibrationBinaryRead,            _NS_::BisAllRaw, _NS_::Calibration)                                                   \
        HANDLER(CanOpenBisPartitionCalibrationBinaryWrite,           _NS_::BisAllRaw, _NS_::Calibration)                                                   \
        HANDLER(CanOpenBisPartitionCalibrationFileRead,              _NS_::BisAllRaw, _NS_::Calibration)                                                   \
        HANDLER(CanOpenBisPartitionCalibrationFileWrite,             _NS_::BisAllRaw, _NS_::Calibration)                                                   \
        HANDLER(CanOpenBisPartitionSafeModeRead,                     _NS_::BisAllRaw)                                                                      \
        HANDLER(CanOpenBisPartitionSafeModeWrite,                    _NS_::BisAllRaw)                                                                      \
        HANDLER(CanOpenBisPartitionSystemProperEncryptionRead,       _NS_::BisAllRaw)                                                                      \
        HANDLER(CanOpenBisPartitionSystemProperEncryptionWrite,      _NS_::BisAllRaw)                                                                      \
        HANDLER(CanOpenBisPartitionSystemProperPartitionRead,        _NS_::BisAllRaw)                                                                      \
        HANDLER(CanOpenBisPartitionSystemProperPartitionWrite,       _NS_::BisAllRaw)                                                                      \
        HANDLER(CanOpenBisPartitionSystemRead,                       _NS_::BisAllRaw)                                                                      \
        HANDLER(CanOpenBisPartitionSystemWrite,                      _NS_::BisAllRaw)                                                                      \
        HANDLER(CanOpenBisPartitionUserDataRootRead,                 _NS_::BisAllRaw)                                                                      \
        HANDLER(CanOpenBisPartitionUserDataRootWrite,                _NS_::BisAllRaw)                                                                      \
        HANDLER(CanOpenBisPartitionUserRead,                         _NS_::BisAllRaw)                                                                      \
        HANDLER(CanOpenBisPartitionUserWrite,                        _NS_::BisAllRaw)                                                                      \
        HANDLER(CanOpenBisWiper,                                     _NS_::ContentManager)                                                                 \
        HANDLER(CanOpenDataStorageByPath,                            _NS_::None)                                                                           \
        HANDLER(CanOpenGameCardDetectionEventNotifier,               _NS_::DeviceDetection, _NS_::GameCardRaw, _NS_::GameCard)                             \
        HANDLER(CanOpenGameCardStorageRead,                          _NS_::GameCardRaw)                                                                    \
        HANDLER(CanOpenGameCardStorageWrite,                         _NS_::GameCardRaw)                                                                    \
        HANDLER(CanOpenOwnSaveDataTransferProhibiter,                _NS_::CreateOwnSaveData)                                                              \
        HANDLER(CanOpenSaveDataInfoReader,                           _NS_::SaveDataManagement, _NS_::SaveDataBackUp)                                       \
        HANDLER(CanOpenSaveDataInfoReaderForInternal,                _NS_::SaveDataManagement)                                                             \
        HANDLER(CanOpenSaveDataInfoReaderForSystem,                  _NS_::SystemSaveDataManagement, _NS_::SaveDataBackUp)                                 \
        HANDLER(CanOpenSaveDataInternalStorageRead,                  _NS_::None)                                                                           \
        HANDLER(CanOpenSaveDataInternalStorageWrite,                 _NS_::None)                                                                           \
        HANDLER(CanOpenSaveDataMetaFile,                             _NS_::SaveDataMeta)                                                                   \
        HANDLER(CanOpenSaveDataMover,                                _NS_::MoveCacheStorage)                                                               \
        HANDLER(CanOpenSaveDataTransferManager,                      _NS_::SaveDataTransfer)                                                               \
        HANDLER(CanOpenSaveDataTransferManagerForRepair,             _NS_::SaveDataBackUp)                                                                 \
        HANDLER(CanOpenSaveDataTransferManagerForSaveDataRepair,     _NS_::SaveDataTransferVersion2)                                                       \
        HANDLER(CanOpenSaveDataTransferManagerForSaveDataRepairTool, _NS_::None)                                                                           \
        HANDLER(CanOpenSaveDataTransferManagerVersion2,              _NS_::SaveDataTransferVersion2)                                                       \
        HANDLER(CanOpenSaveDataTransferProhibiter,                   _NS_::SaveDataTransferVersion2, _NS_::CreateSaveData)                                 \
        HANDLER(CanOpenSdCardDetectionEventNotifier,                 _NS_::DeviceDetection, _NS_::SdCard)                                                  \
        HANDLER(CanOpenSdCardStorageRead,                            _NS_::Debug, _NS_::SdCard)                                                            \
        HANDLER(CanOpenSdCardStorageWrite,                           _NS_::Debug, _NS_::SdCard)                                                            \
        HANDLER(CanOpenSystemDataUpdateEventNotifier,                _NS_::SystemData, _NS_::SystemSaveData)                                               \
        HANDLER(CanOverrideSaveDataTransferTokenSignVerificationKey, _NS_::None)                                                                           \
        HANDLER(CanQuerySaveDataInternalStorageTotalSize,            _NS_::SaveDataTransfer)                                                               \
        HANDLER(CanReadOwnSaveDataFileSystemExtraData,               _NS_::CreateOwnSaveData)                                                              \
        HANDLER(CanReadSaveDataFileSystemExtraData,                  _NS_::SystemSaveDataManagement, _NS_::SaveDataManagement, _NS_::SaveDataBackUp)       \
        HANDLER(CanRegisterExternalKey,                              _NS_::RegisterExternalKey)                                                            \
        HANDLER(CanRegisterProgramIndexMapInfo,                      _NS_::RegisterProgramIndexMapInfo)                                                    \
        HANDLER(CanRegisterUpdatePartition,                          _NS_::RegisterUpdatePartition)                                                        \
        HANDLER(CanResolveAccessFailure,                             _NS_::AccessFailureResolution)                                                        \
        HANDLER(CanSetCurrentPosixTime,                              _NS_::SetTime)                                                                        \
        HANDLER(CanSetDebugConfiguration,                            _NS_::None)                                                                           \
        HANDLER(CanSetEncryptionSeed,                                _NS_::ContentManager)                                                                 \
        HANDLER(CanSetGlobalAccessLogMode,                           _NS_::SettingsControl)                                                                \
        HANDLER(CanSetSdCardAccessibility,                           _NS_::SdCard)                                                                         \
        HANDLER(CanSetSpeedEmulationMode,                            _NS_::SettingsControl)                                                                \
        HANDLER(CanSimulateDevice,                                   _NS_::Debug)                                                                          \
        HANDLER(CanVerifySaveData,                                   _NS_::SaveDataManagement, _NS_::SaveDataBackUp)                                       \
        HANDLER(CanWriteSaveDataFileSystemExtraDataAll,              _NS_::None)                                                                           \
        HANDLER(CanWriteSaveDataFileSystemExtraDataCommitId,         _NS_::SaveDataBackUp)                                                                 \
        HANDLER(CanWriteSaveDataFileSystemExtraDataFlags,            _NS_::SaveDataTransferVersion2, _NS_::SystemSaveDataManagement, _NS_::SaveDataBackUp) \
        HANDLER(CanWriteSaveDataFileSystemExtraDataTimeStamp,        _NS_::SaveDataBackUp)

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class AccessControlBits {
        public:
            enum class Bits : u64 {
                None                        = 0,

                ApplicationInfo             = UINT64_C(1) <<  0,
                BootModeControl             = UINT64_C(1) <<  1,
                Calibration                 = UINT64_C(1) <<  2,
                SystemSaveData              = UINT64_C(1) <<  3,
                GameCard                    = UINT64_C(1) <<  4,
                SaveDataBackUp              = UINT64_C(1) <<  5,
                SaveDataManagement          = UINT64_C(1) <<  6,
                BisAllRaw                   = UINT64_C(1) <<  7,
                GameCardRaw                 = UINT64_C(1) <<  8,
                GameCardPrivate             = UINT64_C(1) <<  9,
                SetTime                     = UINT64_C(1) << 10,
                ContentManager              = UINT64_C(1) << 11,
                ImageManager                = UINT64_C(1) << 12,
                CreateSaveData              = UINT64_C(1) << 13,
                SystemSaveDataManagement    = UINT64_C(1) << 14,
                BisFileSystem               = UINT64_C(1) << 15,
                SystemUpdate                = UINT64_C(1) << 16,
                SaveDataMeta                = UINT64_C(1) << 17,
                DeviceSaveData              = UINT64_C(1) << 18,
                SettingsControl             = UINT64_C(1) << 19,
                SystemData                  = UINT64_C(1) << 20,
                SdCard                      = UINT64_C(1) << 21,
                Host                        = UINT64_C(1) << 22,
                FillBis                     = UINT64_C(1) << 23,
                CorruptSaveData             = UINT64_C(1) << 24,
                SaveDataForDebug            = UINT64_C(1) << 25,
                FormatSdCard                = UINT64_C(1) << 26,
                GetRightsId                 = UINT64_C(1) << 27,
                RegisterExternalKey         = UINT64_C(1) << 28,
                RegisterUpdatePartition     = UINT64_C(1) << 29,
                SaveDataTransfer            = UINT64_C(1) << 30,
                DeviceDetection             = UINT64_C(1) << 31,
                AccessFailureResolution     = UINT64_C(1) << 32,
                SaveDataTransferVersion2    = UINT64_C(1) << 33,
                RegisterProgramIndexMapInfo = UINT64_C(1) << 34,
                CreateOwnSaveData           = UINT64_C(1) << 35,
                MoveCacheStorage            = UINT64_C(1) << 36,

                Debug                       = UINT64_C(1) << 62,
                FullPermission              = UINT64_C(1) << 63
            };
        private:
            static constexpr u64 CombineBits(Bits b) {
                return util::ToUnderlying(b);
            }

            template<typename... Args>
            static constexpr u64 CombineBits(Bits b, Args... args) {
                return CombineBits(b) | CombineBits(args...);
            }
        private:
            const u64 m_value;
        public:
            constexpr AccessControlBits(u64 v) : m_value(v) { /* ... */ }

            constexpr u64 GetValue() const { return m_value; }

            #define DEFINE_ACCESS_GETTER(name, ...) \
                constexpr bool name() const { constexpr u64 Mask = CombineBits(Bits::FullPermission, ## __VA_ARGS__); return (m_value & Mask); }

            AMS_FSSRV_FOR_EACH_ACCESS_CONTROL_CAPABILITY(DEFINE_ACCESS_GETTER, Bits)

            #undef DEFINE_ACCESS_GETTER
    };

}
