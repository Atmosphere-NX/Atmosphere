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

namespace ams::fssrv {

    bool IsDebugFlagEnabled();
    void SetDebugFlagEnabled(bool en);

}

namespace ams::fssrv::impl {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    struct Accessibility {
        u8 value;

        constexpr bool CanRead()  const { return value & (1 << 0); }
        constexpr bool CanWrite() const { return value & (1 << 1); }

        static constexpr Accessibility MakeAccessibility(bool read, bool write) {
            return { static_cast<u8>(read * (1 << 0) + write * (1 << 1)) };
        }
    };
    static_assert(std::is_trivial<Accessibility>::value);

    class ContentOwnerInfo : public util::IntrusiveListBaseNode<ContentOwnerInfo>, public ::ams::fs::impl::Newable {
        private:
            u64 m_id;
        public:
            ContentOwnerInfo(u64 id) : m_id(id) { /* ... */ }

            u64 GetId() const { return m_id; }
    };

    class SaveDataOwnerInfo : public util::IntrusiveListBaseNode<SaveDataOwnerInfo>, public ::ams::fs::impl::Newable {
        private:
            u64 m_id;
            Accessibility m_accessibility;
        public:
            SaveDataOwnerInfo(u64 id, Accessibility access) : m_id(id), m_accessibility(access) { /* ... */ }

            u64 GetId() const { return m_id; }
            Accessibility GetAccessibility() const { return m_accessibility; }
    };

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class AccessControl {
        public:
            enum class AccessibilityType : u32 {
                MountLogo,
                MountContentMeta,
                MountContentControl,
                MountContentManual,
                MountContentData,
                MountApplicationPackage,
                MountSaveDataStorage,
                MountContentStorage,
                MountImageAndVideoStorage,
                MountCloudBackupWorkStorage,
                MountCustomStorage,
                MountBisCalibrationFile,
                MountBisSafeMode,
                MountBisUser,
                MountBisSystem,
                MountBisSystemProperEncryption,
                MountBisSystemProperPartition,
                MountSdCard,
                MountGameCard,
                MountDeviceSaveData,
                MountSystemSaveData,
                MountOthersSaveData,
                MountOthersSystemSaveData,
                OpenBisPartitionBootPartition1Root,
                OpenBisPartitionBootPartition2Root,
                OpenBisPartitionUserDataRoot,
                OpenBisPartitionBootConfigAndPackage2Part1,
                OpenBisPartitionBootConfigAndPackage2Part2,
                OpenBisPartitionBootConfigAndPackage2Part3,
                OpenBisPartitionBootConfigAndPackage2Part4,
                OpenBisPartitionBootConfigAndPackage2Part5,
                OpenBisPartitionBootConfigAndPackage2Part6,
                OpenBisPartitionCalibrationBinary,
                OpenBisPartitionCalibrationFile,
                OpenBisPartitionSafeMode,
                OpenBisPartitionUser,
                OpenBisPartitionSystem,
                OpenBisPartitionSystemProperEncryption,
                OpenBisPartitionSystemProperPartition,
                OpenSdCardStorage,
                OpenGameCardStorage,
                MountSystemDataPrivate,
                MountHost,
                MountRegisteredUpdatePartition,
                MountSaveDataInternalStorage,
                MountTemporaryDirectory,
                MountAllBaseFileSystem,
                NotMount,

                Count,
            };

            enum class OperationType : u32 {
                InvalidateBisCache,
                EraseMmc,
                GetGameCardDeviceCertificate,
                GetGameCardIdSet,
                FinalizeGameCardDriver,
                GetGameCardAsicInfo,
                CreateSaveData,
                DeleteSaveData,
                CreateSystemSaveData,
                CreateOthersSystemSaveData,
                DeleteSystemSaveData,
                OpenSaveDataInfoReader,
                OpenSaveDataInfoReaderForSystem,
                OpenSaveDataInfoReaderForInternal,
                OpenSaveDataMetaFile,
                SetCurrentPosixTime,
                ReadSaveDataFileSystemExtraData,
                SetGlobalAccessLogMode,
                SetSpeedEmulationMode,
                Debug,
                FillBis,
                CorruptSaveData,
                CorruptSystemSaveData,
                VerifySaveData,
                DebugSaveData,
                FormatSdCard,
                GetRightsId,
                RegisterExternalKey,
                SetEncryptionSeed,
                WriteSaveDataFileSystemExtraDataTimeStamp,
                WriteSaveDataFileSystemExtraDataFlags,
                WriteSaveDataFileSystemExtraDataCommitId,
                WriteSaveDataFileSystemExtraDataAll,
                ExtendSaveData,
                ExtendSystemSaveData,
                ExtendOthersSystemSaveData,
                RegisterUpdatePartition,
                OpenSaveDataTransferManager,
                OpenSaveDataTransferManagerVersion2,
                OpenSaveDataTransferManagerForSaveDataRepair,
                OpenSaveDataTransferManagerForSaveDataRepairTool,
                OpenSaveDataTransferProhibiter,
                OpenSaveDataMover,
                OpenBisWiper,
                ListAccessibleSaveDataOwnerId,
                ControlMmcPatrol,
                OverrideSaveDataTransferTokenSignVerificationKey,
                OpenSdCardDetectionEventNotifier,
                OpenGameCardDetectionEventNotifier,
                OpenSystemDataUpdateEventNotifier,
                NotifySystemDataUpdateEvent,
                OpenAccessFailureDetectionEventNotifier,
                GetAccessFailureDetectionEvent,
                IsAccessFailureDetected,
                ResolveAccessFailure,
                AbandonAccessFailure,
                QuerySaveDataInternalStorageTotalSize,
                GetSaveDataCommitId,
                SetSdCardAccessibility,
                SimulateDevice,
                CreateSaveDataWithHashSalt,
                RegisterProgramIndexMapInfo,
                ChallengeCardExistence,
                CreateOwnSaveData,
                DeleteOwnSaveData,
                ReadOwnSaveDataFileSystemExtraData,
                ExtendOwnSaveData,
                OpenOwnSaveDataTransferProhibiter,
                FindOwnSaveDataWithFilter,
                OpenSaveDataTransferManagerForRepair,
                SetDebugConfiguration,
                OpenDataStorageByPath,

                Count,
            };

            AMS_PRAGMA_BEGIN_PACK(4)
            struct AccessControlDataHeader {
                u8 version;
                u8 reserved[3];
                u64 flag_bits;
                u32 content_owner_infos_offset;
                u32 content_owner_infos_size;
                u32 save_data_owner_infos_offset;
                u32 save_data_owner_infos_size;
            };

            struct AccessControlDescriptor {
                u8 version;
                u8 content_owner_id_count;
                u8 save_data_owner_id_count;
                u8 reserved;
                u64 flag_bits;
                u64 content_owner_id_min;
                u64 content_owner_id_max;
                u64 save_data_owner_id_min;
                u64 save_data_owner_id_max;
                /* ... */
            };
            AMS_PRAGMA_END_PACK()

            static_assert(util::is_pod<AccessControlDataHeader>::value);
            static_assert(util::is_pod<AccessControlDescriptor>::value);

            static constexpr u64 AllFlagBitsMask      = ~static_cast<u64>(0);
            static constexpr u64 DebugFlagDisableMask = AllFlagBitsMask & ~util::ToUnderlying(AccessControlBits::Bits::Debug);
        private:
            using ContentOwnerInfoList  = util::IntrusiveListBaseTraits<ContentOwnerInfo>::ListType;
            using SaveDataOwnerInfoList = util::IntrusiveListBaseTraits<SaveDataOwnerInfo>::ListType;
        private:
            util::optional<AccessControlBits> m_flag_bits;
            ContentOwnerInfoList m_content_owner_infos;
            SaveDataOwnerInfoList m_save_data_owner_infos;
        public:
            AccessControl(const void *data, s64 data_size, const void *desc, s64 desc_size);
            AccessControl(const void *data, s64 data_size, const void *desc, s64 desc_size, u64 flag_mask);
            ~AccessControl();
        public:
            bool HasContentOwnerId(u64 owner_id) const;
            Accessibility GetAccessibilitySaveDataOwnedBy(u64 owner_id) const;

            void ListContentOwnerId(s32 *out_count, u64 *out_owner_ids, s32 offset, s32 count) const;
            void ListSaveDataOwnedId(s32 *out_count, ncm::ApplicationId *out_owner_ids, s32 offset, s32 count) const;

            Accessibility GetAccessibilityFor(AccessibilityType type) const;
            bool CanCall(OperationType type) const;
        public:
            u64 GetRawFlagBits() const {
                return m_flag_bits.value().GetValue();
            }
    };

}
