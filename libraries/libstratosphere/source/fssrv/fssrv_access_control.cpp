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
#include <stratosphere.hpp>

namespace ams::fssrv {

    namespace {

        constinit bool g_is_debug_flag_enabled = false;

    }

    bool IsDebugFlagEnabled() {
        return g_is_debug_flag_enabled;
    }

    void SetDebugFlagEnabled(bool en) {
        /* Set global debug flag. */
        g_is_debug_flag_enabled = en;
    }

    namespace impl {

        namespace {

            constexpr u8 LatestFsAccessControlInfoVersion = 1;

        }

        AccessControl::AccessControl(const void *data, s64 data_size, const void *desc, s64 desc_size) : AccessControl(data, data_size, desc, desc_size, g_is_debug_flag_enabled ? AllFlagBitsMask : DebugFlagDisableMask) {
            /* ... */
        }

        AccessControl::AccessControl(const void *fac_data, s64 data_size, const void *fac_desc, s64 desc_size, u64 flag_mask) {
            /* If either our data or descriptor is null, give no permissions. */
            if (fac_data == nullptr || fac_desc == nullptr) {
                m_flag_bits.emplace(util::ToUnderlying(AccessControlBits::Bits::None));
                return;
            }

            /* Check that data/desc are big enough. */
            AMS_ABORT_UNLESS(data_size >= static_cast<s64>(sizeof(AccessControlDataHeader)));
            AMS_ABORT_UNLESS(desc_size >= static_cast<s64>(sizeof(AccessControlDescriptor)));

            /* Copy in the data/descriptor. */
            AccessControlDataHeader data = {};
            AccessControlDescriptor desc = {};
            std::memcpy(std::addressof(data), fac_data, sizeof(data));
            std::memcpy(std::addressof(desc), fac_desc, sizeof(desc));

            /* If we don't know how to parse the descriptors, don't. */
            if (data.version != desc.version || data.version < LatestFsAccessControlInfoVersion) {
                m_flag_bits.emplace(util::ToUnderlying(AccessControlBits::Bits::None));
                return;
            }

            /* Restrict the descriptor's flag bits. */
            desc.flag_bits &= flag_mask;

            /* Create our flag bits. */
            m_flag_bits.emplace(data.flag_bits & desc.flag_bits);

            /* Further check sizes. */
            AMS_ABORT_UNLESS(data_size >= data.content_owner_infos_offset + data.content_owner_infos_size);
            AMS_ABORT_UNLESS(desc_size >= static_cast<s64>(sizeof(AccessControlDescriptor) + desc.content_owner_id_count * sizeof(u64)));

            /* Read out the content data owner infos. */
            uintptr_t data_start = reinterpret_cast<uintptr_t>(fac_data);
            uintptr_t desc_start = reinterpret_cast<uintptr_t>(fac_desc);
            if (data.content_owner_infos_size > 0) {
                /* Get the count. */
                const u32 num_content_owner_infos = util::LoadLittleEndian<u32>(reinterpret_cast<u32 *>(data_start + data.content_owner_infos_offset));

                /* Validate the id range. */
                uintptr_t id_start = data_start + data.content_owner_infos_offset + sizeof(u32);
                uintptr_t id_end   = id_start + sizeof(u64) * num_content_owner_infos;
                AMS_ABORT_UNLESS(id_end == data_start + data.content_owner_infos_offset + data.content_owner_infos_size);

                for (u32 i = 0; i < num_content_owner_infos; ++i) {
                    /* Read the id. */
                    const u64 id = util::LoadLittleEndian<u64>(reinterpret_cast<u64 *>(id_start + i * sizeof(u64)));

                    /* Check that the descriptor allows it. */
                    bool allowed = false;
                    if (desc.content_owner_id_count != 0) {
                        for (u8 n = 0; n < desc.content_owner_id_count; ++n) {
                            if (id == util::LoadLittleEndian<u64>(reinterpret_cast<u64 *>(desc_start + sizeof(AccessControlDescriptor) + n * sizeof(u64)))) {
                                allowed = true;
                                break;
                            }
                        }
                    } else if ((desc.content_owner_id_min == 0 && desc.content_owner_id_max == 0) || (desc.content_owner_id_min <= id && id <= desc.content_owner_id_max)) {
                        allowed = true;
                    }

                    /* If the id is allowed, create it. */
                    if (allowed) {
                        if (auto *info = new ContentOwnerInfo(id); info != nullptr) {
                            m_content_owner_infos.push_front(*info);
                        }
                    }
                }
            }

            /* Read out the save data owner infos. */
            AMS_ABORT_UNLESS(data_size >= data.save_data_owner_infos_offset + data.save_data_owner_infos_size);
            AMS_ABORT_UNLESS(desc_size >= static_cast<s64>(sizeof(AccessControlDescriptor) + desc.content_owner_id_count * sizeof(u64) + desc.save_data_owner_id_count * sizeof(u64)));
            if (data.save_data_owner_infos_size > 0) {
                /* Get the count. */
                const u32 num_save_data_owner_infos = util::LoadLittleEndian<u32>(reinterpret_cast<u32 *>(data_start + data.save_data_owner_infos_offset));

                /* Get accessibility region.*/
                uintptr_t accessibility_start = data_start + data.save_data_owner_infos_offset + sizeof(u32);

                /* Validate the id range. */
                uintptr_t id_start = accessibility_start + util::AlignUp(num_save_data_owner_infos * sizeof(Accessibility), alignof(u32));
                uintptr_t id_end   = id_start + sizeof(u64) * num_save_data_owner_infos;
                AMS_ABORT_UNLESS(id_end == data_start + data.save_data_owner_infos_offset + data.save_data_owner_infos_size);

                for (u32 i = 0; i < num_save_data_owner_infos; ++i) {
                    /* Read the accessibility/id. */
                    static_assert(sizeof(Accessibility) == 1);
                    const Accessibility accessibility = *reinterpret_cast<Accessibility *>(accessibility_start + i * sizeof(Accessibility));
                    const u64 id = util::LoadLittleEndian<u64>(reinterpret_cast<u64 *>(id_start + i * sizeof(u64)));

                    /* Check that the descriptor allows it. */
                    bool allowed = false;
                    if (desc.save_data_owner_id_count != 0) {
                        for (u8 n = 0; n < desc.save_data_owner_id_count; ++n) {
                            if (id == util::LoadLittleEndian<u64>(reinterpret_cast<u64 *>(desc_start + sizeof(AccessControlDescriptor) + desc.content_owner_id_count * sizeof(u64) + n * sizeof(u64)))) {
                                allowed = true;
                                break;
                            }
                        }
                    } else if ((desc.save_data_owner_id_min == 0 && desc.save_data_owner_id_max == 0) || (desc.save_data_owner_id_min <= id && id <= desc.save_data_owner_id_max)) {
                        allowed = true;
                    }

                    /* If the id is allowed, create it. */
                    if (allowed) {
                        if (auto *info = new SaveDataOwnerInfo(id, accessibility); info != nullptr) {
                            m_save_data_owner_infos.push_front(*info);
                        }
                    }
                }
            }

        }

        AccessControl::~AccessControl() {
            /* Delete all content owner infos. */
            while (!m_content_owner_infos.empty()) {
                auto *info = std::addressof(*m_content_owner_infos.rbegin());
                m_content_owner_infos.erase(m_content_owner_infos.iterator_to(*info));
                delete info;
            }

            /* Delete all save data owner infos. */
            while (!m_save_data_owner_infos.empty()) {
                auto *info = std::addressof(*m_save_data_owner_infos.rbegin());
                m_save_data_owner_infos.erase(m_save_data_owner_infos.iterator_to(*info));
                delete info;
            }
        }

        bool AccessControl::HasContentOwnerId(u64 owner_id) const {
            /* Check if we have a matching id. */
            for (const auto &info : m_content_owner_infos) {
                if (info.GetId() == owner_id) {
                    return true;
                }
            }

            return false;
        }

        Accessibility AccessControl::GetAccessibilitySaveDataOwnedBy(u64 owner_id) const {
            /* Find a matching save data owner. */
            for (const auto &info : m_save_data_owner_infos) {
                if (info.GetId() == owner_id) {
                    return info.GetAccessibility();
                }
            }

            /* Default to no accessibility. */
            return Accessibility::MakeAccessibility(false, false);
        }

        void AccessControl::ListContentOwnerId(s32 *out_count, u64 *out_owner_ids, s32 offset, s32 count) const {
            /* If we have nothing to read, just give the count. */
            if (count == 0) {
                *out_count = m_content_owner_infos.size();
                return;
            }

            /* Read out the ids. */
            s32 read_offset = 0;
            s32 read_count  = 0;
            if (out_owner_ids != nullptr) {
                auto *cur_out = out_owner_ids;
                for (const auto &info : m_content_owner_infos) {
                    /* Skip until we get to the desired offset. */
                    if (read_offset < offset) {
                        ++read_offset;
                        continue;
                    }

                    /* Set the output value. */
                    *cur_out = info.GetId();
                    ++cur_out;
                    ++read_count;

                    /* If we've read as many as we can, finish. */
                    if (read_count == count) {
                        break;
                    }
                }
            }

            /* Set the out value. */
            *out_count = read_count;
        }

        void AccessControl::ListSaveDataOwnedId(s32 *out_count, ncm::ApplicationId *out_owner_ids, s32 offset, s32 count) const {
            /* If we have nothing to read, just give the count. */
            if (count == 0) {
                *out_count = m_save_data_owner_infos.size();
                return;
            }

            /* Read out the ids. */
            s32 read_offset = 0;
            s32 read_count  = 0;
            if (out_owner_ids != nullptr) {
                auto *cur_out = out_owner_ids;
                for (const auto &info : m_save_data_owner_infos) {
                    /* Skip until we get to the desired offset. */
                    if (read_offset < offset) {
                        ++read_offset;
                        continue;
                    }

                    /* Set the output value. */
                    cur_out->value = info.GetId();
                    ++cur_out;
                    ++read_count;

                    /* If we've read as many as we can, finish. */
                    if (read_count == count) {
                        break;
                    }
                }
            }

            /* Set the out value. */
            *out_count = read_count;
        }

        Accessibility AccessControl::GetAccessibilityFor(AccessibilityType type) const {
            switch (type) {
                using enum AccessibilityType;
                case MountLogo:                                  return Accessibility::MakeAccessibility(m_flag_bits->CanMountLogoRead(), false);
                case MountContentMeta:                           return Accessibility::MakeAccessibility(m_flag_bits->CanMountContentMetaRead(), false);
                case MountContentControl:                        return Accessibility::MakeAccessibility(m_flag_bits->CanMountContentControlRead(), false);
                case MountContentManual:                         return Accessibility::MakeAccessibility(m_flag_bits->CanMountContentManualRead(), false);
                case MountContentData:                           return Accessibility::MakeAccessibility(m_flag_bits->CanMountContentDataRead(), false);
                case MountApplicationPackage:                    return Accessibility::MakeAccessibility(m_flag_bits->CanMountApplicationPackageRead(), false);
                case MountSaveDataStorage:                       return Accessibility::MakeAccessibility(m_flag_bits->CanMountSaveDataStorageRead(), m_flag_bits->CanMountSaveDataStorageWrite());
                case MountContentStorage:                        return Accessibility::MakeAccessibility(m_flag_bits->CanMountContentStorageRead(), m_flag_bits->CanMountContentStorageWrite());
                case MountImageAndVideoStorage:                  return Accessibility::MakeAccessibility(m_flag_bits->CanMountImageAndVideoStorageRead(), m_flag_bits->CanMountImageAndVideoStorageWrite());
                case MountCloudBackupWorkStorage:                return Accessibility::MakeAccessibility(m_flag_bits->CanMountCloudBackupWorkStorageRead(), m_flag_bits->CanMountCloudBackupWorkStorageWrite());
                case MountCustomStorage:                         return Accessibility::MakeAccessibility(m_flag_bits->CanMountCustomStorage0Read(), m_flag_bits->CanMountCustomStorage0Write());
                case MountBisCalibrationFile:                    return Accessibility::MakeAccessibility(m_flag_bits->CanMountBisCalibrationFileRead(), m_flag_bits->CanMountBisCalibrationFileWrite());
                case MountBisSafeMode:                           return Accessibility::MakeAccessibility(m_flag_bits->CanMountBisSafeModeRead(), m_flag_bits->CanMountBisSafeModeWrite());
                case MountBisUser:                               return Accessibility::MakeAccessibility(m_flag_bits->CanMountBisUserRead(), m_flag_bits->CanMountBisUserWrite());
                case MountBisSystem:                             return Accessibility::MakeAccessibility(m_flag_bits->CanMountBisSystemRead(), m_flag_bits->CanMountBisSystemWrite());
                case MountBisSystemProperEncryption:             return Accessibility::MakeAccessibility(m_flag_bits->CanMountBisSystemProperEncryptionRead(), m_flag_bits->CanMountBisSystemProperEncryptionWrite());
                case MountBisSystemProperPartition:              return Accessibility::MakeAccessibility(m_flag_bits->CanMountBisSystemProperPartitionRead(), m_flag_bits->CanMountBisSystemProperPartitionWrite());
                case MountSdCard:                                return Accessibility::MakeAccessibility(m_flag_bits->CanMountSdCardRead(), m_flag_bits->CanMountSdCardWrite());
                case MountGameCard:                              return Accessibility::MakeAccessibility(m_flag_bits->CanMountGameCardRead(), false);
                case MountDeviceSaveData:                        return Accessibility::MakeAccessibility(m_flag_bits->CanMountDeviceSaveDataRead(), m_flag_bits->CanMountDeviceSaveDataWrite());
                case MountSystemSaveData:                        return Accessibility::MakeAccessibility(m_flag_bits->CanMountSystemSaveDataRead(), m_flag_bits->CanMountSystemSaveDataWrite());
                case MountOthersSaveData:                        return Accessibility::MakeAccessibility(m_flag_bits->CanMountOthersSaveDataRead(), m_flag_bits->CanMountOthersSaveDataWrite());
                case MountOthersSystemSaveData:                  return Accessibility::MakeAccessibility(m_flag_bits->CanMountOthersSystemSaveDataRead(), m_flag_bits->CanMountOthersSystemSaveDataWrite());
                case OpenBisPartitionBootPartition1Root:         return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionBootPartition1RootRead(), m_flag_bits->CanOpenBisPartitionBootPartition1RootWrite());
                case OpenBisPartitionBootPartition2Root:         return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionBootPartition2RootRead(), m_flag_bits->CanOpenBisPartitionBootPartition2RootWrite());
                case OpenBisPartitionUserDataRoot:               return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionUserDataRootRead(), m_flag_bits->CanOpenBisPartitionUserDataRootWrite());
                case OpenBisPartitionBootConfigAndPackage2Part1: return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionBootConfigAndPackage2Part1Read(), m_flag_bits->CanOpenBisPartitionBootConfigAndPackage2Part1Write());
                case OpenBisPartitionBootConfigAndPackage2Part2: return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionBootConfigAndPackage2Part2Read(), m_flag_bits->CanOpenBisPartitionBootConfigAndPackage2Part2Write());
                case OpenBisPartitionBootConfigAndPackage2Part3: return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionBootConfigAndPackage2Part3Read(), m_flag_bits->CanOpenBisPartitionBootConfigAndPackage2Part3Write());
                case OpenBisPartitionBootConfigAndPackage2Part4: return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionBootConfigAndPackage2Part4Read(), m_flag_bits->CanOpenBisPartitionBootConfigAndPackage2Part4Write());
                case OpenBisPartitionBootConfigAndPackage2Part5: return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionBootConfigAndPackage2Part5Read(), m_flag_bits->CanOpenBisPartitionBootConfigAndPackage2Part5Write());
                case OpenBisPartitionBootConfigAndPackage2Part6: return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionBootConfigAndPackage2Part6Read(), m_flag_bits->CanOpenBisPartitionBootConfigAndPackage2Part6Write());
                case OpenBisPartitionCalibrationBinary:          return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionCalibrationBinaryRead(), m_flag_bits->CanOpenBisPartitionCalibrationBinaryWrite());
                case OpenBisPartitionCalibrationFile:            return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionCalibrationFileRead(), m_flag_bits->CanOpenBisPartitionCalibrationFileWrite());
                case OpenBisPartitionSafeMode:                   return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionSafeModeRead(), m_flag_bits->CanOpenBisPartitionSafeModeWrite());
                case OpenBisPartitionUser:                       return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionUserRead(), m_flag_bits->CanOpenBisPartitionUserWrite());
                case OpenBisPartitionSystem:                     return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionSystemRead(), m_flag_bits->CanOpenBisPartitionSystemWrite());
                case OpenBisPartitionSystemProperEncryption:     return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionSystemProperEncryptionRead(), m_flag_bits->CanOpenBisPartitionSystemProperEncryptionWrite());
                case OpenBisPartitionSystemProperPartition:      return Accessibility::MakeAccessibility(m_flag_bits->CanOpenBisPartitionSystemProperPartitionRead(), m_flag_bits->CanOpenBisPartitionSystemProperPartitionWrite());
                case OpenSdCardStorage:                          return Accessibility::MakeAccessibility(m_flag_bits->CanOpenSdCardStorageRead(), m_flag_bits->CanOpenSdCardStorageWrite());
                case OpenGameCardStorage:                        return Accessibility::MakeAccessibility(m_flag_bits->CanOpenGameCardStorageRead(), m_flag_bits->CanOpenGameCardStorageWrite());
                case MountSystemDataPrivate:                     return Accessibility::MakeAccessibility(m_flag_bits->CanMountSystemDataPrivateRead(), false);
                case MountHost:                                  return Accessibility::MakeAccessibility(m_flag_bits->CanMountHostRead(), m_flag_bits->CanMountHostWrite());
                case MountRegisteredUpdatePartition:             return Accessibility::MakeAccessibility(m_flag_bits->CanMountRegisteredUpdatePartitionRead() && g_is_debug_flag_enabled, false);
                case MountSaveDataInternalStorage:               return Accessibility::MakeAccessibility(m_flag_bits->CanOpenSaveDataInternalStorageRead(), m_flag_bits->CanOpenSaveDataInternalStorageWrite());
                case MountTemporaryDirectory:                    return Accessibility::MakeAccessibility(m_flag_bits->CanMountTemporaryDirectoryRead(), m_flag_bits->CanMountTemporaryDirectoryWrite());
                case MountAllBaseFileSystem:                     return Accessibility::MakeAccessibility(m_flag_bits->CanMountAllBaseFileSystemRead(), m_flag_bits->CanMountAllBaseFileSystemWrite());
                case NotMount:                                   return Accessibility::MakeAccessibility(false, false);
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        bool AccessControl::CanCall(OperationType type) const {
            switch (type) {
                using enum OperationType;
                case InvalidateBisCache:                               return m_flag_bits->CanInvalidateBisCache();
                case EraseMmc:                                         return m_flag_bits->CanEraseMmc();
                case GetGameCardDeviceCertificate:                     return m_flag_bits->CanGetGameCardDeviceCertificate();
                case GetGameCardIdSet:                                 return m_flag_bits->CanGetGameCardIdSet();
                case FinalizeGameCardDriver:                           return m_flag_bits->CanFinalizeGameCardDriver();
                case GetGameCardAsicInfo:                              return m_flag_bits->CanGetGameCardAsicInfo();
                case CreateSaveData:                                   return m_flag_bits->CanCreateSaveData();
                case DeleteSaveData:                                   return m_flag_bits->CanDeleteSaveData();
                case CreateSystemSaveData:                             return m_flag_bits->CanCreateSystemSaveData();
                case CreateOthersSystemSaveData:                       return m_flag_bits->CanCreateOthersSystemSaveData();
                case DeleteSystemSaveData:                             return m_flag_bits->CanDeleteSystemSaveData();
                case OpenSaveDataInfoReader:                           return m_flag_bits->CanOpenSaveDataInfoReader();
                case OpenSaveDataInfoReaderForSystem:                  return m_flag_bits->CanOpenSaveDataInfoReaderForSystem();
                case OpenSaveDataInfoReaderForInternal:                return m_flag_bits->CanOpenSaveDataInfoReaderForInternal();
                case OpenSaveDataMetaFile:                             return m_flag_bits->CanOpenSaveDataMetaFile();
                case SetCurrentPosixTime:                              return m_flag_bits->CanSetCurrentPosixTime();
                case ReadSaveDataFileSystemExtraData:                  return m_flag_bits->CanReadSaveDataFileSystemExtraData();
                case SetGlobalAccessLogMode:                           return m_flag_bits->CanSetGlobalAccessLogMode();
                case SetSpeedEmulationMode:                            return m_flag_bits->CanSetSpeedEmulationMode();
                case FillBis:                                          return m_flag_bits->CanFillBis();
                case CorruptSaveData:                                  return m_flag_bits->CanCorruptSaveData();
                case CorruptSystemSaveData:                            return m_flag_bits->CanCorruptSystemSaveData();
                case VerifySaveData:                                   return m_flag_bits->CanVerifySaveData();
                case DebugSaveData:                                    return m_flag_bits->CanDebugSaveData();
                case FormatSdCard:                                     return m_flag_bits->CanFormatSdCard();
                case GetRightsId:                                      return m_flag_bits->CanGetRightsId();
                case RegisterExternalKey:                              return m_flag_bits->CanRegisterExternalKey();
                case SetEncryptionSeed:                                return m_flag_bits->CanSetEncryptionSeed();
                case WriteSaveDataFileSystemExtraDataTimeStamp:        return m_flag_bits->CanWriteSaveDataFileSystemExtraDataTimeStamp();
                case WriteSaveDataFileSystemExtraDataFlags:            return m_flag_bits->CanWriteSaveDataFileSystemExtraDataFlags();
                case WriteSaveDataFileSystemExtraDataCommitId:         return m_flag_bits->CanWriteSaveDataFileSystemExtraDataCommitId();
                case WriteSaveDataFileSystemExtraDataAll:              return m_flag_bits->CanWriteSaveDataFileSystemExtraDataAll();
                case ExtendSaveData:                                   return m_flag_bits->CanExtendSaveData();
                case ExtendSystemSaveData:                             return m_flag_bits->CanExtendSystemSaveData();
                case ExtendOthersSystemSaveData:                       return m_flag_bits->CanExtendOthersSystemSaveData();
                case RegisterUpdatePartition:                          return m_flag_bits->CanRegisterUpdatePartition() && g_is_debug_flag_enabled;
                case OpenSaveDataTransferManager:                      return m_flag_bits->CanOpenSaveDataTransferManager();
                case OpenSaveDataTransferManagerVersion2:              return m_flag_bits->CanOpenSaveDataTransferManagerVersion2();
                case OpenSaveDataTransferManagerForSaveDataRepair:     return m_flag_bits->CanOpenSaveDataTransferManagerForSaveDataRepair();
                case OpenSaveDataTransferManagerForSaveDataRepairTool: return m_flag_bits->CanOpenSaveDataTransferManagerForSaveDataRepairTool();
                case OpenSaveDataTransferProhibiter:                   return m_flag_bits->CanOpenSaveDataTransferProhibiter();
                case OpenSaveDataMover:                                return m_flag_bits->CanOpenSaveDataMover();
                case OpenBisWiper:                                     return m_flag_bits->CanOpenBisWiper();
                case ListAccessibleSaveDataOwnerId:                    return m_flag_bits->CanListAccessibleSaveDataOwnerId();
                case ControlMmcPatrol:                                 return m_flag_bits->CanControlMmcPatrol();
                case OverrideSaveDataTransferTokenSignVerificationKey: return m_flag_bits->CanOverrideSaveDataTransferTokenSignVerificationKey();
                case OpenSdCardDetectionEventNotifier:                 return m_flag_bits->CanOpenSdCardDetectionEventNotifier();
                case OpenGameCardDetectionEventNotifier:               return m_flag_bits->CanOpenGameCardDetectionEventNotifier();
                case OpenSystemDataUpdateEventNotifier:                return m_flag_bits->CanOpenSystemDataUpdateEventNotifier();
                case NotifySystemDataUpdateEvent:                      return m_flag_bits->CanNotifySystemDataUpdateEvent();
                case OpenAccessFailureDetectionEventNotifier:          return m_flag_bits->CanOpenAccessFailureDetectionEventNotifier();
                case GetAccessFailureDetectionEvent:                   return m_flag_bits->CanGetAccessFailureDetectionEvent();
                case IsAccessFailureDetected:                          return m_flag_bits->CanIsAccessFailureDetected();
                case ResolveAccessFailure:                             return m_flag_bits->CanResolveAccessFailure();
                case AbandonAccessFailure:                             return m_flag_bits->CanAbandonAccessFailure();
                case QuerySaveDataInternalStorageTotalSize:            return m_flag_bits->CanQuerySaveDataInternalStorageTotalSize();
                case GetSaveDataCommitId:                              return m_flag_bits->CanGetSaveDataCommitId();
                case SetSdCardAccessibility:                           return m_flag_bits->CanSetSdCardAccessibility();
                case SimulateDevice:                                   return m_flag_bits->CanSimulateDevice();
                case CreateSaveDataWithHashSalt:                       return m_flag_bits->CanCreateSaveDataWithHashSalt();
                case RegisterProgramIndexMapInfo:                      return m_flag_bits->CanRegisterProgramIndexMapInfo();
                case ChallengeCardExistence:                           return m_flag_bits->CanChallengeCardExistence();
                case CreateOwnSaveData:                                return m_flag_bits->CanCreateOwnSaveData();
                case DeleteOwnSaveData:                                return m_flag_bits->CanDeleteOwnSaveData();
                case ReadOwnSaveDataFileSystemExtraData:               return m_flag_bits->CanReadOwnSaveDataFileSystemExtraData();
                case ExtendOwnSaveData:                                return m_flag_bits->CanExtendOwnSaveData();
                case OpenOwnSaveDataTransferProhibiter:                return m_flag_bits->CanOpenOwnSaveDataTransferProhibiter();
                case FindOwnSaveDataWithFilter:                        return m_flag_bits->CanFindOwnSaveDataWithFilter();
                case OpenSaveDataTransferManagerForRepair:             return m_flag_bits->CanOpenSaveDataTransferManagerForRepair();
                case SetDebugConfiguration:                            return m_flag_bits->CanSetDebugConfiguration();
                case OpenDataStorageByPath:                            return m_flag_bits->CanOpenDataStorageByPath();
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

    }

}
