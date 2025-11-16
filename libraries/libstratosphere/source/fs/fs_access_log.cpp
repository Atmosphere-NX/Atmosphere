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
#include "impl/fs_file_system_proxy_service_object.hpp"
#include "fsa/fs_user_mount_table.hpp"
#include "fsa/fs_directory_accessor.hpp"
#include "fsa/fs_file_accessor.hpp"
#include "fsa/fs_filesystem_accessor.hpp"

#define AMS_FS_IMPL_ACCESS_LOG_AMS_API_VERSION "ams_version: " STRINGIZE(ATMOSPHERE_RELEASE_VERSION_MAJOR) "." STRINGIZE(ATMOSPHERE_RELEASE_VERSION_MINOR) "." STRINGIZE(ATMOSPHERE_RELEASE_VERSION_MICRO)

/* TODO: Other specs? */
#define AMS_FS_IMPL_ACCESS_LOG_SPEC "spec: NX"

namespace ams::fs {

    /* Forward declare priority getter. */
    fs::PriorityRaw GetPriorityRawOnCurrentThreadInternal();

    namespace {

        constinit u32 g_global_access_log_mode  = fs::AccessLogMode_None;
        constinit u32 g_local_access_log_target = fs::impl::AccessLogTarget_None;

        constinit std::atomic_bool g_access_log_initialized = false;
        constinit os::SdkMutex g_access_log_initialization_mutex;

        void SetLocalAccessLogImpl(bool enabled) {
            if (enabled) {
                g_local_access_log_target |= fs::impl::AccessLogTarget_Application;
            } else {
                g_local_access_log_target &= ~fs::impl::AccessLogTarget_Application;
            }
        }

    }

    Result GetGlobalAccessLogMode(u32 *out) {
        const auto fsp = impl::GetFileSystemProxyServiceObject();
        AMS_FS_R_TRY(fsp->GetGlobalAccessLogMode(out));
        R_SUCCEED();
    }

    Result SetGlobalAccessLogMode(u32 mode) {
        const auto fsp = impl::GetFileSystemProxyServiceObject();
        AMS_FS_R_TRY(fsp->SetGlobalAccessLogMode(mode));
        R_SUCCEED();
    }

    void SetLocalAccessLog(bool enabled) {
        SetLocalAccessLogImpl(enabled);
    }

    void SetLocalApplicationAccessLog(bool enabled) {
        SetLocalAccessLogImpl(enabled);
    }

    void SetLocalSystemAccessLogForDebug(bool enabled) {
        #if defined(AMS_BUILD_FOR_DEBUGGING)
            if (enabled) {
                g_local_access_log_target |= (fs::impl::AccessLogTarget_Application | fs::impl::AccessLogTarget_System);
            } else {
                g_local_access_log_target &= ~(fs::impl::AccessLogTarget_Application | fs::impl::AccessLogTarget_System);
            }
        #else
            AMS_UNUSED(enabled);
        #endif
    }

}

namespace ams::fs::impl {

    #define ADD_ENUM_CASE(v) case v: return #v

    const char *IdString::ToValueString(int id) {
        const int len = util::SNPrintf(m_buffer, sizeof(m_buffer), "%d", id);
        AMS_ASSERT(static_cast<size_t>(len) < sizeof(m_buffer));
        AMS_UNUSED(len);
        return m_buffer;
    }

    template<> const char *IdString::ToString<fs::Priority>(fs::Priority id) {
        switch (id) {
            case fs::Priority_Realtime: return "Realtime";
            case fs::Priority_Normal:   return "Normal";
            case fs::Priority_Low:      return "Low";
            default:                    return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fs::PriorityRaw>(fs::PriorityRaw id) {
        switch (id) {
            case fs::PriorityRaw_Realtime:   return "Realtime";
            case fs::PriorityRaw_Normal:     return "Normal";
            case fs::PriorityRaw_Low:        return "Low";
            case fs::PriorityRaw_Background: return "Realtime";
            default:                         return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fs::ContentStorageId>(fs::ContentStorageId id) {
        switch (id) {
            using enum fs::ContentStorageId;
            ADD_ENUM_CASE(User);
            ADD_ENUM_CASE(System);
            ADD_ENUM_CASE(SdCard);
            ADD_ENUM_CASE(System0);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fs::SaveDataSpaceId>(fs::SaveDataSpaceId id) {
        switch (id) {
            using enum fs::SaveDataSpaceId;
            ADD_ENUM_CASE(System);
            ADD_ENUM_CASE(User);
            ADD_ENUM_CASE(SdSystem);
            ADD_ENUM_CASE(ProperSystem);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fs::ContentType>(fs::ContentType id) {
        switch (id) {
            case fs::ContentType_Meta:    return "Meta";
            case fs::ContentType_Control: return "Control";
            case fs::ContentType_Manual:  return "Manual";
            case fs::ContentType_Logo:    return "Logo";
            case fs::ContentType_Data:    return "Data";
            default:                      return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fs::MountHostOption>(fs::MountHostOption id) {
        if (id == MountHostOption::PseudoCaseSensitive) {
            return "MountHostOptionFlag_PseudoCaseSensitive";
        } else {
            return ToValueString(static_cast<int>(id._value));
        }
    }

    template<> const char *IdString::ToString<fs::BisPartitionId>(fs::BisPartitionId id) {
        switch (id) {
            using enum fs::BisPartitionId;
            ADD_ENUM_CASE(BootPartition1Root);
            ADD_ENUM_CASE(BootPartition2Root);
            ADD_ENUM_CASE(UserDataRoot);
            ADD_ENUM_CASE(BootConfigAndPackage2Part1);
            ADD_ENUM_CASE(BootConfigAndPackage2Part2);
            ADD_ENUM_CASE(BootConfigAndPackage2Part3);
            ADD_ENUM_CASE(BootConfigAndPackage2Part4);
            ADD_ENUM_CASE(BootConfigAndPackage2Part5);
            ADD_ENUM_CASE(BootConfigAndPackage2Part6);
            ADD_ENUM_CASE(CalibrationBinary);
            ADD_ENUM_CASE(CalibrationFile);
            ADD_ENUM_CASE(SafeMode);
            ADD_ENUM_CASE(User);
            ADD_ENUM_CASE(System);
            ADD_ENUM_CASE(SystemProperEncryption);
            ADD_ENUM_CASE(SystemProperPartition);
            ADD_ENUM_CASE(DeviceTreeBlob);
            ADD_ENUM_CASE(System0);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fs::DirectoryEntryType>(fs::DirectoryEntryType type) {
        switch (type) {
            case fs::DirectoryEntryType_Directory: return "Directory";
            case fs::DirectoryEntryType_File:      return "File";
            default:                               return ToValueString(static_cast<int>(type));
        }
    }

    template<> const char *IdString::ToString<fs::GameCardPartition>(fs::GameCardPartition id) {
        switch (id) {
            using enum fs::GameCardPartition;
            ADD_ENUM_CASE(Update);
            ADD_ENUM_CASE(Normal);
            ADD_ENUM_CASE(Secure);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fssystem::NcaHeader::ContentType>(fssystem::NcaHeader::ContentType id) {
        switch (id) {
            using enum fssystem::NcaHeader::ContentType;
            ADD_ENUM_CASE(Program);
            ADD_ENUM_CASE(Meta);
            ADD_ENUM_CASE(Control);
            ADD_ENUM_CASE(Manual);
            ADD_ENUM_CASE(Data);
            ADD_ENUM_CASE(PublicData);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fssystem::NcaHeader::DistributionType>(fssystem::NcaHeader::DistributionType id) {
        switch (id) {
            using enum fssystem::NcaHeader::DistributionType;
            ADD_ENUM_CASE(Download);
            ADD_ENUM_CASE(GameCard);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fssystem::NcaHeader::EncryptionType>(fssystem::NcaHeader::EncryptionType id) {
        switch (id) {
            using enum fssystem::NcaHeader::EncryptionType;
            ADD_ENUM_CASE(Auto);
            ADD_ENUM_CASE(None);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fssystem::NcaHeader::DecryptionKey>(fssystem::NcaHeader::DecryptionKey id) {
        switch (id) {
            using enum fssystem::NcaHeader::DecryptionKey;
            case DecryptionKey_AesXts1:  return "AesXts1";
            case DecryptionKey_AesXts2:  return "AesXts2";
            case DecryptionKey_AesCtr:   return "AesCtr";
            case DecryptionKey_AesCtrEx: return "AesCtrEx";
            case DecryptionKey_AesCtrHw: return "AesCtrHw";
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fssystem::NcaFsHeader::FsType>(fssystem::NcaFsHeader::FsType id) {
        switch (id) {
            using enum fssystem::NcaFsHeader::FsType;
            ADD_ENUM_CASE(RomFs);
            ADD_ENUM_CASE(PartitionFs);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fssystem::NcaFsHeader::EncryptionType>(fssystem::NcaFsHeader::EncryptionType id) {
        switch (id) {
            using enum fssystem::NcaFsHeader::EncryptionType;
            ADD_ENUM_CASE(Auto);
            ADD_ENUM_CASE(None);
            ADD_ENUM_CASE(AesXts);
            ADD_ENUM_CASE(AesCtr);
            ADD_ENUM_CASE(AesCtrEx);
            ADD_ENUM_CASE(AesCtrSkipLayerHash);
            ADD_ENUM_CASE(AesCtrExSkipLayerHash);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fssystem::NcaFsHeader::HashType>(fssystem::NcaFsHeader::HashType id) {
        switch (id) {
            using enum fssystem::NcaFsHeader::HashType;
            ADD_ENUM_CASE(Auto);
            ADD_ENUM_CASE(None);
            ADD_ENUM_CASE(HierarchicalSha256Hash);
            ADD_ENUM_CASE(HierarchicalIntegrityHash);
            ADD_ENUM_CASE(AutoSha3);
            ADD_ENUM_CASE(HierarchicalSha3256Hash);
            ADD_ENUM_CASE(HierarchicalIntegritySha3Hash);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fssystem::NcaFsHeader::MetaDataHashType>(fssystem::NcaFsHeader::MetaDataHashType id) {
        switch (id) {
            using enum fssystem::NcaFsHeader::MetaDataHashType;
            ADD_ENUM_CASE(None);
            ADD_ENUM_CASE(HierarchicalIntegrity);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<gc::impl::MemoryCapacity>(gc::impl::MemoryCapacity id) {
        switch (id) {
            using enum gc::impl::MemoryCapacity;
            case MemoryCapacity_1GB:  return "1GB";
            case MemoryCapacity_2GB:  return "2GB";
            case MemoryCapacity_4GB:  return "4GB";
            case MemoryCapacity_8GB:  return "8GB";
            case MemoryCapacity_16GB: return "16GB";
            case MemoryCapacity_32GB: return "32GB";
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<gc::impl::SelSec>(gc::impl::SelSec id) {
        switch (id) {
            using enum gc::impl::SelSec;
            case SelSec_T1: return "T1";
            case SelSec_T2: return "T2";
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<gc::impl::KekIndex>(gc::impl::KekIndex id) {
        switch (id) {
            using enum gc::impl::KekIndex;
            case KekIndex_Version0:      return "Version0";
            case KekIndex_VersionForDev: return "VersionForDev";
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<gc::impl::AccessControl1ClockRate>(gc::impl::AccessControl1ClockRate id) {
        switch (id) {
            using enum gc::impl::AccessControl1ClockRate;
            case AccessControl1ClockRate_25MHz:  return "25 MHz";
            case AccessControl1ClockRate_50MHz:  return "50 MHz";
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<gc::impl::FwVersion>(gc::impl::FwVersion id) {
        switch (id) {
            using enum gc::impl::FwVersion;
            case FwVersion_ForDev: return "ForDev";
            case FwVersion_1_0_0:  return "1.0.0";
            case FwVersion_4_0_0:  return "4.0.0";
            case FwVersion_9_0_0:  return "9.0.0";
            case FwVersion_11_0_0: return "11.0.0";
            case FwVersion_12_0_0: return "12.0.0";
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fs::GameCardCompatibilityType>(fs::GameCardCompatibilityType id) {
        switch (id) {
            using enum fs::GameCardCompatibilityType;
            ADD_ENUM_CASE(Normal);
            ADD_ENUM_CASE(Terra);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fssrv::impl::AccessControlBits::Bits>(fssrv::impl::AccessControlBits::Bits id) {
        switch (id) {
            using enum fssrv::impl::AccessControlBits::Bits;
            ADD_ENUM_CASE(ApplicationInfo);
            ADD_ENUM_CASE(BootModeControl);
            ADD_ENUM_CASE(Calibration);
            ADD_ENUM_CASE(SystemSaveData);
            ADD_ENUM_CASE(GameCard);
            ADD_ENUM_CASE(SaveDataBackUp);
            ADD_ENUM_CASE(SaveDataManagement);
            ADD_ENUM_CASE(BisAllRaw);
            ADD_ENUM_CASE(GameCardRaw);
            ADD_ENUM_CASE(GameCardPrivate);
            ADD_ENUM_CASE(SetTime);
            ADD_ENUM_CASE(ContentManager);
            ADD_ENUM_CASE(ImageManager);
            ADD_ENUM_CASE(CreateSaveData);
            ADD_ENUM_CASE(SystemSaveDataManagement);
            ADD_ENUM_CASE(BisFileSystem);
            ADD_ENUM_CASE(SystemUpdate);
            ADD_ENUM_CASE(SaveDataMeta);
            ADD_ENUM_CASE(DeviceSaveData);
            ADD_ENUM_CASE(SettingsControl);
            ADD_ENUM_CASE(SystemData);
            ADD_ENUM_CASE(SdCard);
            ADD_ENUM_CASE(Host);
            ADD_ENUM_CASE(FillBis);
            ADD_ENUM_CASE(CorruptSaveData);
            ADD_ENUM_CASE(SaveDataForDebug);
            ADD_ENUM_CASE(FormatSdCard);
            ADD_ENUM_CASE(GetRightsId);
            ADD_ENUM_CASE(RegisterExternalKey);
            ADD_ENUM_CASE(RegisterUpdatePartition);
            ADD_ENUM_CASE(SaveDataTransfer);
            ADD_ENUM_CASE(DeviceDetection);
            ADD_ENUM_CASE(AccessFailureResolution);
            ADD_ENUM_CASE(SaveDataTransferVersion2);
            ADD_ENUM_CASE(RegisterProgramIndexMapInfo);
            ADD_ENUM_CASE(CreateOwnSaveData);
            ADD_ENUM_CASE(MoveCacheStorage);

            ADD_ENUM_CASE(Debug);
            ADD_ENUM_CASE(FullPermission);
            default: return ToValueString(util::CountTrailingZeros(util::ToUnderlying(id)));
        }
    }

    template<> const char *IdString::ToString<fssrv::impl::AccessControl::AccessibilityType>(fssrv::impl::AccessControl::AccessibilityType id) {
        switch (id) {
            using enum fssrv::impl::AccessControl::AccessibilityType;
            ADD_ENUM_CASE(MountLogo);
            ADD_ENUM_CASE(MountContentMeta);
            ADD_ENUM_CASE(MountContentControl);
            ADD_ENUM_CASE(MountContentManual);
            ADD_ENUM_CASE(MountContentData);
            ADD_ENUM_CASE(MountApplicationPackage);
            ADD_ENUM_CASE(MountSaveDataStorage);
            ADD_ENUM_CASE(MountContentStorage);
            ADD_ENUM_CASE(MountImageAndVideoStorage);
            ADD_ENUM_CASE(MountCloudBackupWorkStorage);
            ADD_ENUM_CASE(MountCustomStorage);
            ADD_ENUM_CASE(MountBisCalibrationFile);
            ADD_ENUM_CASE(MountBisSafeMode);
            ADD_ENUM_CASE(MountBisUser);
            ADD_ENUM_CASE(MountBisSystem);
            ADD_ENUM_CASE(MountBisSystemProperEncryption);
            ADD_ENUM_CASE(MountBisSystemProperPartition);
            ADD_ENUM_CASE(MountSdCard);
            ADD_ENUM_CASE(MountGameCard);
            ADD_ENUM_CASE(MountDeviceSaveData);
            ADD_ENUM_CASE(MountSystemSaveData);
            ADD_ENUM_CASE(MountOthersSaveData);
            ADD_ENUM_CASE(MountOthersSystemSaveData);
            ADD_ENUM_CASE(OpenBisPartitionBootPartition1Root);
            ADD_ENUM_CASE(OpenBisPartitionBootPartition2Root);
            ADD_ENUM_CASE(OpenBisPartitionUserDataRoot);
            ADD_ENUM_CASE(OpenBisPartitionBootConfigAndPackage2Part1);
            ADD_ENUM_CASE(OpenBisPartitionBootConfigAndPackage2Part2);
            ADD_ENUM_CASE(OpenBisPartitionBootConfigAndPackage2Part3);
            ADD_ENUM_CASE(OpenBisPartitionBootConfigAndPackage2Part4);
            ADD_ENUM_CASE(OpenBisPartitionBootConfigAndPackage2Part5);
            ADD_ENUM_CASE(OpenBisPartitionBootConfigAndPackage2Part6);
            ADD_ENUM_CASE(OpenBisPartitionCalibrationBinary);
            ADD_ENUM_CASE(OpenBisPartitionCalibrationFile);
            ADD_ENUM_CASE(OpenBisPartitionSafeMode);
            ADD_ENUM_CASE(OpenBisPartitionUser);
            ADD_ENUM_CASE(OpenBisPartitionSystem);
            ADD_ENUM_CASE(OpenBisPartitionSystemProperEncryption);
            ADD_ENUM_CASE(OpenBisPartitionSystemProperPartition);
            ADD_ENUM_CASE(OpenSdCardStorage);
            ADD_ENUM_CASE(OpenGameCardStorage);
            ADD_ENUM_CASE(MountSystemDataPrivate);
            ADD_ENUM_CASE(MountHost);
            ADD_ENUM_CASE(MountRegisteredUpdatePartition);
            ADD_ENUM_CASE(MountSaveDataInternalStorage);
            ADD_ENUM_CASE(MountTemporaryDirectory);
            ADD_ENUM_CASE(MountAllBaseFileSystem);
            ADD_ENUM_CASE(NotMount);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fssrv::impl::AccessControl::OperationType>(fssrv::impl::AccessControl::OperationType id) {
        switch (id) {
            using enum fssrv::impl::AccessControl::OperationType;
            ADD_ENUM_CASE(InvalidateBisCache);
            ADD_ENUM_CASE(EraseMmc);
            ADD_ENUM_CASE(GetGameCardDeviceCertificate);
            ADD_ENUM_CASE(GetGameCardIdSet);
            ADD_ENUM_CASE(FinalizeGameCardDriver);
            ADD_ENUM_CASE(GetGameCardAsicInfo);
            ADD_ENUM_CASE(CreateSaveData);
            ADD_ENUM_CASE(DeleteSaveData);
            ADD_ENUM_CASE(CreateSystemSaveData);
            ADD_ENUM_CASE(CreateOthersSystemSaveData);
            ADD_ENUM_CASE(DeleteSystemSaveData);
            ADD_ENUM_CASE(OpenSaveDataInfoReader);
            ADD_ENUM_CASE(OpenSaveDataInfoReaderForSystem);
            ADD_ENUM_CASE(OpenSaveDataInfoReaderForInternal);
            ADD_ENUM_CASE(OpenSaveDataMetaFile);
            ADD_ENUM_CASE(SetCurrentPosixTime);
            ADD_ENUM_CASE(ReadSaveDataFileSystemExtraData);
            ADD_ENUM_CASE(SetGlobalAccessLogMode);
            ADD_ENUM_CASE(SetSpeedEmulationMode);
            ADD_ENUM_CASE(Debug);
            ADD_ENUM_CASE(FillBis);
            ADD_ENUM_CASE(CorruptSaveData);
            ADD_ENUM_CASE(CorruptSystemSaveData);
            ADD_ENUM_CASE(VerifySaveData);
            ADD_ENUM_CASE(DebugSaveData);
            ADD_ENUM_CASE(FormatSdCard);
            ADD_ENUM_CASE(GetRightsId);
            ADD_ENUM_CASE(RegisterExternalKey);
            ADD_ENUM_CASE(SetEncryptionSeed);
            ADD_ENUM_CASE(WriteSaveDataFileSystemExtraDataTimeStamp);
            ADD_ENUM_CASE(WriteSaveDataFileSystemExtraDataFlags);
            ADD_ENUM_CASE(WriteSaveDataFileSystemExtraDataCommitId);
            ADD_ENUM_CASE(WriteSaveDataFileSystemExtraDataAll);
            ADD_ENUM_CASE(ExtendSaveData);
            ADD_ENUM_CASE(ExtendSystemSaveData);
            ADD_ENUM_CASE(ExtendOthersSystemSaveData);
            ADD_ENUM_CASE(RegisterUpdatePartition);
            ADD_ENUM_CASE(OpenSaveDataTransferManager);
            ADD_ENUM_CASE(OpenSaveDataTransferManagerVersion2);
            ADD_ENUM_CASE(OpenSaveDataTransferManagerForSaveDataRepair);
            ADD_ENUM_CASE(OpenSaveDataTransferManagerForSaveDataRepairTool);
            ADD_ENUM_CASE(OpenSaveDataTransferProhibiter);
            ADD_ENUM_CASE(OpenSaveDataMover);
            ADD_ENUM_CASE(OpenBisWiper);
            ADD_ENUM_CASE(ListAccessibleSaveDataOwnerId);
            ADD_ENUM_CASE(ControlMmcPatrol);
            ADD_ENUM_CASE(OverrideSaveDataTransferTokenSignVerificationKey);
            ADD_ENUM_CASE(OpenSdCardDetectionEventNotifier);
            ADD_ENUM_CASE(OpenGameCardDetectionEventNotifier);
            ADD_ENUM_CASE(OpenSystemDataUpdateEventNotifier);
            ADD_ENUM_CASE(NotifySystemDataUpdateEvent);
            ADD_ENUM_CASE(OpenAccessFailureDetectionEventNotifier);
            ADD_ENUM_CASE(GetAccessFailureDetectionEvent);
            ADD_ENUM_CASE(IsAccessFailureDetected);
            ADD_ENUM_CASE(ResolveAccessFailure);
            ADD_ENUM_CASE(AbandonAccessFailure);
            ADD_ENUM_CASE(QuerySaveDataInternalStorageTotalSize);
            ADD_ENUM_CASE(GetSaveDataCommitId);
            ADD_ENUM_CASE(SetSdCardAccessibility);
            ADD_ENUM_CASE(SimulateDevice);
            ADD_ENUM_CASE(CreateSaveDataWithHashSalt);
            ADD_ENUM_CASE(RegisterProgramIndexMapInfo);
            ADD_ENUM_CASE(ChallengeCardExistence);
            ADD_ENUM_CASE(CreateOwnSaveData);
            ADD_ENUM_CASE(DeleteOwnSaveData);
            ADD_ENUM_CASE(ReadOwnSaveDataFileSystemExtraData);
            ADD_ENUM_CASE(ExtendOwnSaveData);
            ADD_ENUM_CASE(OpenOwnSaveDataTransferProhibiter);
            ADD_ENUM_CASE(FindOwnSaveDataWithFilter);
            ADD_ENUM_CASE(OpenSaveDataTransferManagerForRepair);
            ADD_ENUM_CASE(SetDebugConfiguration);
            ADD_ENUM_CASE(OpenDataStorageByPath);
            default: return ToValueString(static_cast<int>(id));
        }
    }

    namespace {

        class AccessLogPrinterCallbackManager {
            private:
                AccessLogPrinterCallback m_callback;
            public:
                constexpr AccessLogPrinterCallbackManager() : m_callback(nullptr) { /* ... */ }

                constexpr bool IsRegisteredCallback() const { return m_callback != nullptr; }

                constexpr void RegisterCallback(AccessLogPrinterCallback c) {
                    AMS_ASSERT(m_callback == nullptr);
                    m_callback = c;
                }

                constexpr int InvokeCallback(char *buf, size_t size) const {
                    AMS_ASSERT(m_callback != nullptr);
                    return m_callback(buf, size);
                }
        };

        constinit AccessLogPrinterCallbackManager g_access_log_manager_printer_callback_manager;

        ALWAYS_INLINE AccessLogPrinterCallbackManager &GetStartAccessLogPrinterCallbackManager() {
            return g_access_log_manager_printer_callback_manager;
        }

        const char *GetPriorityRawName(fs::impl::IdString &id_string) {
            return id_string.ToString(fs::GetPriorityRawOnCurrentThreadInternal());
        }

        Result OutputAccessLogToSdCardImpl(const char *log, size_t size) {
            const auto fsp = impl::GetFileSystemProxyServiceObject();
            AMS_FS_R_TRY(fsp->OutputAccessLogToSdCard(sf::InBuffer(log, size)));
            R_SUCCEED();
        }

        void OutputAccessLogToSdCard(const char *format, std::va_list vl) {
            if ((g_global_access_log_mode & AccessLogMode_SdCard) != 0) {
                /* Create a buffer to hold the log's input string. */
                int log_buffer_size = 1_KB;
                auto log_buffer = fs::impl::MakeUnique<char[]>(log_buffer_size);
                while (true) {
                    if (log_buffer == nullptr) {
                        return;
                    }

                    const auto size = util::VSNPrintf(log_buffer.get(), log_buffer_size, format, vl);
                    if (size < log_buffer_size) {
                        break;
                    }

                    log_buffer_size = size + 1;
                    log_buffer = fs::impl::MakeUnique<char[]>(log_buffer_size);
                }

                /* Output. */
                static_cast<void>(OutputAccessLogToSdCardImpl(log_buffer.get(), log_buffer_size - 1));
            }
        }

        void OutputAccessLogImpl(const char *log, size_t size) {
            if ((g_global_access_log_mode & AccessLogMode_Log) != 0) {
                /* TODO: Support logging. */
            } else if ((g_global_access_log_mode & AccessLogMode_SdCard) != 0) {
                static_cast<void>(OutputAccessLogToSdCardImpl(log, size - 1));
            }
        }

        void OutputAccessLog(Result result, const char *priority, os::Tick start, os::Tick end, const char *name, const void *handle, const char *format, std::va_list vl) {
            /* Create a buffer to hold the log's input string. */
            int str_buffer_size = 1_KB;
            auto str_buffer = fs::impl::MakeUnique<char[]>(str_buffer_size);
            while (true) {
                if (str_buffer == nullptr) {
                    return;
                }

                const auto size = util::VSNPrintf(str_buffer.get(), str_buffer_size, format, vl);
                if (size < str_buffer_size) {
                    break;
                }

                str_buffer_size = size + 1;
                str_buffer = fs::impl::MakeUnique<char[]>(str_buffer_size);
            }

            /* Create a buffer to hold the log. */
            int log_buffer_size = 0;
            decltype(str_buffer) log_buffer;
            {
                /* Declare format string. */
                constexpr const char FormatString[] = "FS_ACCESS { "
                                                      "start: %9" PRId64 ", "
                                                      "end: %9" PRId64 ", "
                                                      "result: 0x%08" PRIX32 ", "
                                                      "handle: 0x%p, "
                                                      "priority: %s, "
                                                      "function: \"%s\""
                                                      "%s"
                                                      " }\n";

                /* Convert the timing to ms. */
                const s64 start_ms = start.ToTimeSpan().GetMilliSeconds();
                const s64 end_ms   = end.ToTimeSpan().GetMilliSeconds();

                /* Print the log. */
                int try_size = std::max<int>(str_buffer_size + sizeof(FormatString) + 0x100, 1_KB);
                while (true) {
                    log_buffer = fs::impl::MakeUnique<char[]>(try_size);
                    if (log_buffer == nullptr) {
                        return;
                    }

                    log_buffer_size = 1 + util::SNPrintf(log_buffer.get(), try_size, FormatString, start_ms, end_ms, result.GetValue(), handle, priority, name, str_buffer.get());
                    if (log_buffer_size <= try_size) {
                        break;
                    }
                    try_size = log_buffer_size;
                }
            }

            OutputAccessLogImpl(log_buffer.get(), log_buffer_size);
        }

        void GetProgramIndexForAccessLog(u32 *out_index, u32 *out_count) {
            if (hos::GetVersion() >= hos::Version_7_0_0) {
                /* Use libnx bindings if available. */
                const auto fsp = impl::GetFileSystemProxyServiceObject();
                R_ABORT_UNLESS(fsp->GetProgramIndexForAccessLog(out_index, out_count));
            } else {
                /* Use hardcoded defaults. */
                *out_index = 0;
                *out_count = 0;
            }
        }

        void OutputAccessLogStart() {
            /* Get the program index. */
            u32 program_index = 0, program_count = 0;
            GetProgramIndexForAccessLog(std::addressof(program_index), std::addressof(program_count));

            /* Print the log buffer. */
            if (program_count < 2) {
                constexpr const char StartLog[] = "FS_ACCESS: { "
                                                  AMS_FS_IMPL_ACCESS_LOG_AMS_API_VERSION ", "
                                                  AMS_FS_IMPL_ACCESS_LOG_SPEC
                                                  " }\n";

                OutputAccessLogImpl(StartLog, sizeof(StartLog));
            } else {
                constexpr const char StartLog[] = "FS_ACCESS: { "
                                                  AMS_FS_IMPL_ACCESS_LOG_AMS_API_VERSION ", "
                                                  AMS_FS_IMPL_ACCESS_LOG_SPEC ", "
                                                  "program_index: %d"
                                                  " }\n";

                char log_buffer[0x80];
                const int len = 1 + util::SNPrintf(log_buffer, sizeof(log_buffer), StartLog, static_cast<int>(program_index));
                if (static_cast<size_t>(len) <= sizeof(log_buffer)) {
                    OutputAccessLogImpl(log_buffer, len);
                }
            }
        }

        [[maybe_unused]] void OutputAccessLogStartForSystem() {
            constexpr const char StartLog[] = "FS_ACCESS: { "
                                              AMS_FS_IMPL_ACCESS_LOG_AMS_API_VERSION ", "
                                              AMS_FS_IMPL_ACCESS_LOG_SPEC ", "
                                              "for_system: true"
                                              " }\n";
            OutputAccessLogImpl(StartLog, sizeof(StartLog));
        }

        void OutputAccessLogStartGeneratedByCallback() {
            /* Get the manager. */
            const auto &manager = GetStartAccessLogPrinterCallbackManager();
            if (manager.IsRegisteredCallback()) {
                /* Invoke the callback. */
                char log_buffer[0x80];
                const int len = 1 + manager.InvokeCallback(log_buffer, sizeof(log_buffer));

                /* Print, if we fit. */
                if (static_cast<size_t>(len) <= sizeof(log_buffer)) {
                    OutputAccessLogImpl(log_buffer, len);
                }
            }
        }

    }

    bool IsEnabledAccessLog(u32 target) {
        /* If we don't need to log to the target, return false. */
        if ((g_local_access_log_target & target) == 0) {
            return false;
        }

        /* Ensure we've initialized. */
        if (!g_access_log_initialized) {
            std::scoped_lock lk(g_access_log_initialization_mutex);
            if (!g_access_log_initialized) {

                #if defined (AMS_BUILD_FOR_DEBUGGING)
                if ((g_local_access_log_target & fs::impl::AccessLogTarget_System) != 0)
                {
                    g_global_access_log_mode = AccessLogMode_Log;
                    OutputAccessLogStartForSystem();
                    OutputAccessLogStartGeneratedByCallback();
                }
                else
                #endif
                {
                    AMS_FS_R_ABORT_UNLESS(GetGlobalAccessLogMode(std::addressof(g_global_access_log_mode)));
                    if (g_global_access_log_mode != AccessLogMode_None) {
                        OutputAccessLogStart();
                        OutputAccessLogStartGeneratedByCallback();
                    }
                }

                g_access_log_initialized = true;
            }
        }

        return g_global_access_log_mode != AccessLogMode_None;
    }

    bool IsEnabledAccessLog() {
        return IsEnabledAccessLog(fs::impl::AccessLogTarget_Application | fs::impl::AccessLogTarget_System);
    }

    void RegisterStartAccessLogPrinterCallback(AccessLogPrinterCallback callback) {
        GetStartAccessLogPrinterCallbackManager().RegisterCallback(callback);
    }

    void OutputAccessLog(Result result, fs::Priority priority, os::Tick start, os::Tick end, const char *name, const void *handle, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        OutputAccessLog(result, fs::impl::IdString().ToString(priority), start, end, name, handle, fmt, vl);
        va_end(vl);
    }

    void OutputAccessLog(Result result, fs::PriorityRaw priority_raw, os::Tick start, os::Tick end, const char *name, const void *handle, const char *fmt, ...){
        std::va_list vl;
        va_start(vl, fmt);
        OutputAccessLog(result, fs::impl::IdString().ToString(priority_raw), start, end, name, handle, fmt, vl);
        va_end(vl);
    }

    void OutputAccessLog(Result result, os::Tick start, os::Tick end, const char *name, fs::FileHandle handle, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        fs::impl::IdString id_string;
        OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle.handle, fmt, vl);
        va_end(vl);
    }

    void OutputAccessLog(Result result, os::Tick start, os::Tick end, const char *name, fs::DirectoryHandle handle, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        fs::impl::IdString id_string;
        OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle.handle, fmt, vl);
        va_end(vl);
    }

    void OutputAccessLog(Result result, os::Tick start, os::Tick end, const char *name, fs::impl::IdentifyAccessLogHandle handle, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        fs::impl::IdString id_string;
        OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle.handle, fmt, vl);
        va_end(vl);
    }

    void OutputAccessLog(Result result, os::Tick start, os::Tick end, const char *name, const void *handle, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        fs::impl::IdString id_string;
        OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle, fmt, vl);
        va_end(vl);
    }

    void OutputAccessLogToOnlySdCard(const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        OutputAccessLogToSdCard(fmt, vl);
        va_end(vl);
    }

    void OutputAccessLogUnlessResultSuccess(Result result, os::Tick start, os::Tick end, const char *name, fs::FileHandle handle, const char *fmt, ...) {
        if (R_FAILED(result)) {
            std::va_list vl;
            va_start(vl, fmt);
            fs::impl::IdString id_string;
            OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle.handle, fmt, vl);
            va_end(vl);
        }
    }

    void OutputAccessLogUnlessResultSuccess(Result result, os::Tick start, os::Tick end, const char *name, fs::DirectoryHandle handle, const char *fmt, ...) {
        if (R_FAILED(result)) {
            std::va_list vl;
            va_start(vl, fmt);
            fs::impl::IdString id_string;
            OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle.handle, fmt, vl);
            va_end(vl);
        }
    }

    void OutputAccessLogUnlessResultSuccess(Result result, os::Tick start, os::Tick end, const char *name, const void *handle, const char *fmt, ...) {
        if (R_FAILED(result)) {
            std::va_list vl;
            va_start(vl, fmt);
            fs::impl::IdString id_string;
            OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle, fmt, vl);
            va_end(vl);
        }
    }

    bool IsEnabledHandleAccessLog(fs::FileHandle handle) {
        /* Get the file accessor. */
        impl::FileAccessor *accessor = reinterpret_cast<impl::FileAccessor *>(handle.handle);
        if (accessor == nullptr) {
            return true;
        }

        /* Check the parent. */
        if (auto *parent = accessor->GetParent(); parent != nullptr) {
            return parent->IsEnabledAccessLog();
        } else {
            return false;
        }
    }

    bool IsEnabledHandleAccessLog(fs::DirectoryHandle handle) {
        /* Get the file accessor. */
        impl::DirectoryAccessor *accessor = reinterpret_cast<impl::DirectoryAccessor *>(handle.handle);
        if (accessor == nullptr) {
            return true;
        }

        /* Check the parent. */
        if (auto *parent = accessor->GetParent(); parent != nullptr) {
            return parent->IsEnabledAccessLog();
        } else {
            return false;
        }
    }

    bool IsEnabledHandleAccessLog(fs::impl::IdentifyAccessLogHandle handle) {
        AMS_UNUSED(handle);
        return true;
    }

    bool IsEnabledHandleAccessLog(const void *handle) {
        if (handle == nullptr) {
            return true;
        }

        /* We should never receive non-null here. */
        AMS_ASSERT(handle == nullptr);
        return false;
    }

    bool IsEnabledFileSystemAccessorAccessLog(const char *mount_name) {
        /* Get the accessor. */
        impl::FileSystemAccessor *accessor = nullptr;
        if (R_FAILED(impl::Find(std::addressof(accessor), mount_name))) {
            return true;
        }

        return accessor->IsEnabledAccessLog();
    }

    void EnableFileSystemAccessorAccessLog(const char *mount_name) {
        /* Get the accessor. */
        impl::FileSystemAccessor *accessor = nullptr;
        AMS_FS_R_ABORT_UNLESS(impl::Find(std::addressof(accessor), mount_name));
        accessor->SetAccessLogEnabled(true);
    }

}
