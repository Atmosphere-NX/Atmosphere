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
#include <stratosphere/fssrv/sf/fssrv_sf_i_file_system_proxy.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_i_program_registry.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_i_file_system_proxy_for_loader.hpp>

namespace ams::fssrv {

    namespace impl {

        class FileSystemProxyCoreImpl;

    }

    class NcaFileSystemService;
    class SaveDataFileSystemService;

    /* ACCURATE_TO_VERSION: Unknown */
    class FileSystemProxyImpl {
        NON_COPYABLE(FileSystemProxyImpl);
        NON_MOVEABLE(FileSystemProxyImpl);
        private:
            impl::FileSystemProxyCoreImpl *m_impl;
            std::shared_ptr<NcaFileSystemService> m_nca_service;
            std::shared_ptr<SaveDataFileSystemService> m_save_data_service;
            u64 m_process_id;
        public:
            FileSystemProxyImpl();
            ~FileSystemProxyImpl();

            /* TODO */
        public:
            /* fsp-srv */
            Result OpenFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, u32 type);
            Result SetCurrentProcess(const ams::sf::ClientProcessId &client_pid);
            Result OpenDataFileSystemByCurrentProcess(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out);
            Result OpenFileSystemWithPatch(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, ncm::ProgramId program_id, u32 type);
            Result OpenFileSystemWithIdObsolete(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, u64 program_id, u32 type);
            Result OpenDataFileSystemByProgramId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, ncm::ProgramId program_id);
            Result OpenFileSystemWithId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, fs::ContentAttributes attr, u64 program_id, u32 type);
            Result OpenBisFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, u32 id);
            Result OpenBisStorage(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, u32 id);
            Result InvalidateBisCache();
            Result OpenHostFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path);
            Result OpenSdCardFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out);
            Result FormatSdCardFileSystem();
            Result DeleteSaveDataFileSystem(u64 save_data_id);
            Result CreateSaveDataFileSystem(const fs::SaveDataAttribute &attribute, const fs::SaveDataCreationInfo &creation_info, const fs::SaveDataMetaInfo &meta_info);
            Result CreateSaveDataFileSystemBySystemSaveDataId(const fs::SaveDataAttribute &attribute, const fs::SaveDataCreationInfo &creation_info);
            Result RegisterSaveDataFileSystemAtomicDeletion(const ams::sf::InBuffer &save_data_ids);
            Result DeleteSaveDataFileSystemBySaveDataSpaceId(u8 indexer_space_id, u64 save_data_id);
            Result FormatSdCardDryRun();
            Result IsExFatSupported(ams::sf::Out<bool> out);
            Result DeleteSaveDataFileSystemBySaveDataAttribute(u8 space_id, const fs::SaveDataAttribute &attribute);
            Result OpenGameCardStorage(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, u32 handle, u32 partition);
            Result OpenGameCardFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u32 handle, u32 partition);
            Result ExtendSaveDataFileSystem(u8 space_id, u64 save_data_id, s64 available_size, s64 journal_size);
            Result DeleteCacheStorage(u16 index);
            Result GetCacheStorageSize(ams::sf::Out<s64> out_size, ams::sf::Out<s64> out_journal_size, u16 index);
            Result CreateSaveDataFileSystemWithHashSalt(const fs::SaveDataAttribute &attribute, const fs::SaveDataCreationInfo &creation_info, const fs::SaveDataMetaInfo &meta_info, const fs::HashSalt &salt);
            Result OpenHostFileSystemWithOption(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, u32 option);
            Result OpenSaveDataFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u8 space_id, const fs::SaveDataAttribute &attribute);
            Result OpenSaveDataFileSystemBySystemSaveDataId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u8 space_id, const fs::SaveDataAttribute &attribute);
            Result OpenReadOnlySaveDataFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u8 space_id, const fs::SaveDataAttribute &attribute);
            Result ReadSaveDataFileSystemExtraDataBySaveDataSpaceId(const ams::sf::OutBuffer &buffer, u8 space_id, u64 save_data_id);
            Result ReadSaveDataFileSystemExtraData(const ams::sf::OutBuffer &buffer, u64 save_data_id);
            Result WriteSaveDataFileSystemExtraData(u64 save_data_id, u8 space_id, const ams::sf::InBuffer &buffer);
            /* ... */
            Result OpenImageDirectoryFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u32 id);
            /* ... */
            Result OpenContentStorageFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u32 id);
            /* ... */
            Result OpenDataStorageByCurrentProcess(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out);
            Result OpenDataStorageByProgramId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, ncm::ProgramId program_id);
            Result OpenDataStorageByDataId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, ncm::DataId data_id, u8 storage_id);
            Result OpenPatchDataStorageByCurrentProcess(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out);
            Result OpenDataFileSystemWithProgramIndex(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u8 index);
            Result OpenDataStorageWithProgramIndex(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, u8 index);
            Result OpenDataStorageByPathObsolete(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, const fssrv::sf::FspPath &path, u32 type);
            Result OpenDataStorageByPath(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, const fssrv::sf::FspPath &path, fs::ContentAttributes attr, u32 type);
            Result OpenDeviceOperator(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IDeviceOperator>> out);
            Result OpenSdCardDetectionEventNotifier(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IEventNotifier>> out);
            Result OpenGameCardDetectionEventNotifier(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IEventNotifier>> out);
            Result OpenSystemDataUpdateEventNotifier(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IEventNotifier>> out);
            Result NotifySystemDataUpdateEvent();
            /* ... */
            Result SetCurrentPosixTime(s64 posix_time);
            /* ... */
            Result GetRightsId(ams::sf::Out<fs::RightsId> out, ncm::ProgramId program_id, ncm::StorageId storage_id);
            Result RegisterExternalKey(const fs::RightsId &rights_id, const spl::AccessKey &access_key);
            Result UnregisterAllExternalKey();
            Result GetProgramId(ams::sf::Out<ncm::ProgramId> out, const fssrv::sf::FspPath &path, fs::ContentAttributes attr);
            Result GetRightsIdByPath(ams::sf::Out<fs::RightsId> out, const fssrv::sf::FspPath &path);
            Result GetRightsIdAndKeyGenerationByPathObsolete(ams::sf::Out<fs::RightsId> out, ams::sf::Out<u8> out_key_generation, const fssrv::sf::FspPath &path);
            Result GetRightsIdAndKeyGenerationByPath(ams::sf::Out<fs::RightsId> out, ams::sf::Out<u8> out_key_generation, const fssrv::sf::FspPath &path, fs::ContentAttributes attr);
            Result SetCurrentPosixTimeWithTimeDifference(s64 posix_time, s32 time_difference);
            Result GetFreeSpaceSizeForSaveData(ams::sf::Out<s64> out, u8 space_id);
            Result VerifySaveDataFileSystemBySaveDataSpaceId();
            Result CorruptSaveDataFileSystemBySaveDataSpaceId();
            Result QuerySaveDataInternalStorageTotalSize();
            Result GetSaveDataCommitId();
            Result UnregisterExternalKey(const fs::RightsId &rights_id);
            Result SetSdCardEncryptionSeed(const fs::EncryptionSeed &seed);
            Result SetSdCardAccessibility(bool accessible);
            Result IsSdCardAccessible(ams::sf::Out<bool> out);
            Result IsSignedSystemPartitionOnSdCardValid(ams::sf::Out<bool> out);
            Result OpenAccessFailureDetectionEventNotifier();
            /* ... */
            Result GetAndClearErrorInfo(ams::sf::Out<fs::FileSystemProxyErrorInfo> out);
            Result RegisterProgramIndexMapInfo(const ams::sf::InBuffer &buffer, s32 count);
            Result SetBisRootForHost(u32 id, const fssrv::sf::FspPath &path);
            Result SetSaveDataSize(s64 size, s64 journal_size);
            Result SetSaveDataRootPath(const fssrv::sf::FspPath &path);
            Result DisableAutoSaveDataCreation();
            Result SetGlobalAccessLogMode(u32 mode);
            Result GetGlobalAccessLogMode(ams::sf::Out<u32> out);
            Result OutputAccessLogToSdCard(const ams::sf::InBuffer &buf);
            Result RegisterUpdatePartition();
            Result OpenRegisteredUpdatePartition(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out);
            Result GetAndClearMemoryReportInfo(ams::sf::Out<fs::MemoryReportInfo> out);
            /* ... */
            Result GetProgramIndexForAccessLog(ams::sf::Out<u32> out_idx, ams::sf::Out<u32> out_count);
            Result GetFsStackUsage(ams::sf::Out<u32> out, u32 type);
            Result UnsetSaveDataRootPath();
            Result OutputMultiProgramTagAccessLog();
            Result FlushAccessLogOnSdCard();
            Result OutputApplicationInfoAccessLog();
            Result RegisterDebugConfiguration(u32 key, s64 value);
            Result UnregisterDebugConfiguration(u32 key);
            Result OverrideSaveDataTransferTokenSignVerificationKey(const ams::sf::InBuffer &buf);
            Result CorruptSaveDataFileSystemByOffset(u8 space_id, u64 save_data_id, s64 offset);
            /* ... */
        public:
            /* fsp-ldr */
            Result OpenCodeFileSystemDeprecated(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const fssrv::sf::Path &path, ncm::ProgramId program_id);
            Result OpenCodeFileSystemDeprecated2(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, ncm::ProgramId program_id);
            Result OpenCodeFileSystemDeprecated3(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, fs::ContentAttributes attr, ncm::ProgramId program_id);
            Result OpenCodeFileSystemDeprecated4(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const ams::sf::OutBuffer &out_verif, const fssrv::sf::Path &path, fs::ContentAttributes attr, ncm::ProgramId program_id);
            Result OpenCodeFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const ams::sf::OutBuffer &out_verif, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id);
            Result IsArchivedProgram(ams::sf::Out<bool> out, u64 process_id);
    };
    static_assert(sf::IsIFileSystemProxy<FileSystemProxyImpl>);
    static_assert(sf::IsIFileSystemProxyForLoader<FileSystemProxyImpl>);

    /* ACCURATE_TO_VERSION: Unknown */
    class InvalidFileSystemProxyImplForLoader {
        public:
            Result OpenCodeFileSystemDeprecated(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const fssrv::sf::Path &path, ncm::ProgramId program_id) {
                AMS_UNUSED(out_fs, path, program_id);
                R_THROW(fs::ResultPortAcceptableCountLimited());
            }

            Result OpenCodeFileSystemDeprecated2(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, ncm::ProgramId program_id) {
                AMS_UNUSED(out_fs, out_verif, path, program_id);
                R_THROW(fs::ResultPortAcceptableCountLimited());
            }

            Result OpenCodeFileSystemDeprecated3(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, fs::ContentAttributes attr, ncm::ProgramId program_id) {
                AMS_UNUSED(out_fs, out_verif, path, attr, program_id);
                R_THROW(fs::ResultPortAcceptableCountLimited());
            }

            Result OpenCodeFileSystemDeprecated4(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const ams::sf::OutBuffer &out_verif, const fssrv::sf::Path &path, fs::ContentAttributes attr, ncm::ProgramId program_id) {
                AMS_UNUSED(out_fs, out_verif, path, attr, program_id);
                R_THROW(fs::ResultPortAcceptableCountLimited());
            }

            Result OpenCodeFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const ams::sf::OutBuffer &out_verif, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id) {
                AMS_UNUSED(out_fs, out_verif, attr, program_id, storage_id);
                R_THROW(fs::ResultPortAcceptableCountLimited());
            }

            Result IsArchivedProgram(ams::sf::Out<bool> out, u64 process_id) {
                AMS_UNUSED(out, process_id);
                R_THROW(fs::ResultPortAcceptableCountLimited());
            }

            Result SetCurrentProcess(const ams::sf::ClientProcessId &client_pid) {
                AMS_UNUSED(client_pid);
                R_THROW(fs::ResultPortAcceptableCountLimited());
            }
    };
    static_assert(sf::IsIFileSystemProxyForLoader<FileSystemProxyImpl>);

}
