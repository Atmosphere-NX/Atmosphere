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
#include <stratosphere.hpp>
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>
#include "../fssrv/impl/fssrv_allocator_for_service_framework.hpp"
#include "impl/fs_remote_event_notifier.hpp"
#include "impl/fs_remote_device_operator.hpp"

namespace ams::fs {

    #if defined(ATMOSPHERE_OS_HORIZON)

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"

    class RemoteFileSystemProxy {
        NON_COPYABLE(RemoteFileSystemProxy);
        NON_MOVEABLE(RemoteFileSystemProxy);
        private:
            using ObjectFactory = fssrv::impl::FileSystemObjectFactory;
        public:
            RemoteFileSystemProxy(int session_count);
            ~RemoteFileSystemProxy();
        public:
            /* Command interface */
            Result OpenFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, u32 type) {
                AMS_ABORT("TODO");
            }


            Result SetCurrentProcess(const ams::sf::ClientProcessId &client_pid) {
                /* Libnx does this for us automatically. */
                AMS_UNUSED(client_pid);
                R_SUCCEED();
            }

            Result OpenDataFileSystemByCurrentProcess(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out) {
                AMS_ABORT("TODO");
            }

            Result OpenFileSystemWithPatch(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, ncm::ProgramId program_id, u32 type) {
                AMS_ABORT("TODO");
            }

            Result OpenFileSystemWithIdObsolete(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, u64 program_id, u32 type) {
                R_RETURN(this->OpenFileSystemWithId(out, path, fs::ContentAttributes_None, program_id, type));
            }

            Result OpenFileSystemWithId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, fs::ContentAttributes attr, u64 program_id, u32 type) {
                ::FsFileSystem fs;
                R_TRY(fsOpenFileSystemWithId(std::addressof(fs), program_id, static_cast<::FsFileSystemType>(type), path.str, static_cast<::FsContentAttributes>(static_cast<u8>(attr))));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, fssrv::impl::RemoteFileSystem>(fs));
                R_SUCCEED();
            }

            Result OpenDataFileSystemByProgramId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, ncm::ProgramId program_id) {
                AMS_ABORT("TODO");
            }


            Result OpenBisFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, u32 id) {
                AMS_ABORT("TODO");
            }


            Result OpenBisStorage(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, u32 id) {
                FsStorage s;
                R_TRY(fsOpenBisStorage(std::addressof(s), static_cast<::FsBisPartitionId>(id)));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IStorage, fssrv::impl::RemoteStorage>(s));
                R_SUCCEED();
            }

            Result InvalidateBisCache() {
                AMS_ABORT("TODO");
            }

            Result OpenHostFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path) {
                AMS_ABORT("TODO");
            }


            Result OpenSdCardFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out) {
                ::FsFileSystem fs;
                R_TRY(fsOpenSdCardFileSystem(std::addressof(fs)));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, fssrv::impl::RemoteFileSystem>(fs));
                R_SUCCEED();
            }

            Result FormatSdCardFileSystem() {
                AMS_ABORT("TODO");
            }


            Result DeleteSaveDataFileSystem(u64 save_data_id) {
                AMS_UNUSED(save_data_id);
                AMS_ABORT("TODO: libnx binding");
            }

            Result CreateSaveDataFileSystem(const fs::SaveDataAttribute &attribute, const fs::SaveDataCreationInfo &creation_info, const fs::SaveDataMetaInfo &meta_info) {
                AMS_ABORT("TODO");
            }


            Result CreateSaveDataFileSystemBySystemSaveDataId(const fs::SaveDataAttribute &attribute, const fs::SaveDataCreationInfo &creation_info) {
                static_assert(sizeof(attribute)     == sizeof(::FsSaveDataAttribute));
                static_assert(sizeof(creation_info) == sizeof(::FsSaveDataCreationInfo));
                R_RETURN(fsCreateSaveDataFileSystemBySystemSaveDataId(reinterpret_cast<const ::FsSaveDataAttribute *>(std::addressof(attribute)), reinterpret_cast<const ::FsSaveDataCreationInfo *>(std::addressof(creation_info))));
            }

            Result RegisterSaveDataFileSystemAtomicDeletion(const ams::sf::InBuffer &save_data_ids) {
                AMS_ABORT("TODO");
            }


            Result DeleteSaveDataFileSystemBySaveDataSpaceId(u8 indexer_space_id, u64 save_data_id) {
                R_RETURN(fsDeleteSaveDataFileSystemBySaveDataSpaceId(static_cast<::FsSaveDataSpaceId>(indexer_space_id), save_data_id));
            }

            Result FormatSdCardDryRun() {
                AMS_ABORT("TODO");
            }

            Result IsExFatSupported(ams::sf::Out<bool> out) {
                AMS_ABORT("TODO");
            }


            Result DeleteSaveDataFileSystemBySaveDataAttribute(u8 space_id, const fs::SaveDataAttribute &attribute) {
                static_assert(sizeof(attribute) == sizeof(::FsSaveDataAttribute));
                R_RETURN(fsDeleteSaveDataFileSystemBySaveDataAttribute(static_cast<::FsSaveDataSpaceId>(space_id), reinterpret_cast<const ::FsSaveDataAttribute *>(std::addressof(attribute))));
            }

            Result OpenGameCardStorage(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, u32 handle, u32 partition) {
                AMS_ABORT("TODO");
            }


            Result OpenGameCardFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u32 handle, u32 partition) {
                ::FsFileSystem fs;
                const ::FsGameCardHandle _hnd = {handle};
                R_TRY(fsOpenGameCardFileSystem(std::addressof(fs), std::addressof(_hnd), static_cast<::FsGameCardPartition>(partition)));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, fssrv::impl::RemoteFileSystem>(fs));
                R_SUCCEED();
            }

            Result ExtendSaveDataFileSystem(u8 space_id, u64 save_data_id, s64 available_size, s64 journal_size) {
                R_RETURN(::fsExtendSaveDataFileSystem(static_cast<::FsSaveDataSpaceId>(space_id), save_data_id, available_size, journal_size));
            }

            Result DeleteCacheStorage(u16 index) {
                AMS_ABORT("TODO");
            }

            Result GetCacheStorageSize(ams::sf::Out<s64> out_size, ams::sf::Out<s64> out_journal_size, u16 index) {
                AMS_ABORT("TODO");
            }

            Result CreateSaveDataFileSystemWithHashSalt(const fs::SaveDataAttribute &attribute, const fs::SaveDataCreationInfo &creation_info, const fs::SaveDataMetaInfo &meta_info, const fs::HashSalt &salt) {
                AMS_ABORT("TODO");
            }

            Result OpenHostFileSystemWithOption(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, u32 option) {
                AMS_ABORT("TODO");
            }


            Result OpenSaveDataFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u8 space_id, const fs::SaveDataAttribute &attribute) {
                ::FsFileSystem fs;
                R_TRY(fsOpenSaveDataFileSystem(std::addressof(fs), static_cast<::FsSaveDataSpaceId>(space_id), reinterpret_cast<const ::FsSaveDataAttribute *>(std::addressof(attribute))));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, fssrv::impl::RemoteFileSystem>(fs));
                R_SUCCEED();
            }

            Result OpenSaveDataFileSystemBySystemSaveDataId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u8 space_id, const fs::SaveDataAttribute &attribute) {
                ::FsFileSystem fs;
                R_TRY(fsOpenSaveDataFileSystemBySystemSaveDataId(std::addressof(fs), static_cast<::FsSaveDataSpaceId>(space_id), reinterpret_cast<const ::FsSaveDataAttribute *>(std::addressof(attribute))));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, fssrv::impl::RemoteFileSystem>(fs));
                R_SUCCEED();
            }

            Result OpenReadOnlySaveDataFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u8 space_id, const fs::SaveDataAttribute &attribute) {
                AMS_ABORT("TODO");
            }


            Result ReadSaveDataFileSystemExtraDataBySaveDataSpaceId(const ams::sf::OutBuffer &buffer, u8 space_id, u64 save_data_id) {
                R_RETURN(fsReadSaveDataFileSystemExtraDataBySaveDataSpaceId(buffer.GetPointer(), buffer.GetSize(), static_cast<::FsSaveDataSpaceId>(space_id), save_data_id));
            }

            Result ReadSaveDataFileSystemExtraData(const ams::sf::OutBuffer &buffer, u64 save_data_id) {
                R_RETURN(fsReadSaveDataFileSystemExtraData(buffer.GetPointer(), buffer.GetSize(), save_data_id));
            }

            Result WriteSaveDataFileSystemExtraData(u64 save_data_id, u8 space_id, const ams::sf::InBuffer &buffer) {
                R_RETURN(fsWriteSaveDataFileSystemExtraData(buffer.GetPointer(), buffer.GetSize(), static_cast<::FsSaveDataSpaceId>(space_id), save_data_id));
            }

            /* ... */

            Result OpenImageDirectoryFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u32 id) {
                ::FsFileSystem fs;
                R_TRY(fsOpenImageDirectoryFileSystem(std::addressof(fs), static_cast<::FsImageDirectoryId>(id)));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, fssrv::impl::RemoteFileSystem>(fs));
                R_SUCCEED();
            }

            /* ... */

            Result OpenContentStorageFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u32 id) {
                ::FsFileSystem fs;
                R_TRY(fsOpenContentStorageFileSystem(std::addressof(fs), static_cast<::FsContentStorageId>(id)));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, fssrv::impl::RemoteFileSystem>(fs));
                R_SUCCEED();
            }

            /* ... */

            Result OpenDataStorageByCurrentProcess(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out) {
                AMS_ABORT("TODO");
            }

            Result OpenDataStorageByProgramId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, ncm::ProgramId program_id) {
                AMS_ABORT("TODO");
            }

            Result OpenDataStorageByDataId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, ncm::DataId data_id, u8 storage_id) {
                ::FsStorage s;
                R_TRY(fsOpenDataStorageByDataId(std::addressof(s), data_id.value, static_cast<::NcmStorageId>(storage_id)));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IStorage, fssrv::impl::RemoteStorage>(s));
                R_SUCCEED();
            }
            Result OpenPatchDataStorageByCurrentProcess(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out) {
                AMS_ABORT("TODO");
            }

            Result OpenDataFileSystemWithProgramIndex(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u8 index) {
                AMS_ABORT("TODO");
            }

            Result OpenDataStorageWithProgramIndex(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, u8 index) {
                AMS_ABORT("TODO");
            }

            Result OpenDataStorageByPathObsolete(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, const fssrv::sf::FspPath &path, u32 type) {
                AMS_ABORT("TODO");
            }

            Result OpenDataStorageByPath(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, const fssrv::sf::FspPath &path, fs::ContentAttributes attr, u32 type) {
                AMS_ABORT("TODO");
            }

            Result OpenDeviceOperator(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IDeviceOperator>> out) {
                ::FsDeviceOperator d;
                R_TRY(fsOpenDeviceOperator(std::addressof(d)));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IDeviceOperator, impl::RemoteDeviceOperator>(d));
                R_SUCCEED();
            }

            Result OpenSdCardDetectionEventNotifier(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IEventNotifier>> out) {
                ::FsEventNotifier e;
                R_TRY(fsOpenSdCardDetectionEventNotifier(std::addressof(e)));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<fssrv::sf::IEventNotifier, impl::RemoteEventNotifier>(e));
                R_SUCCEED();
            }

            Result OpenGameCardDetectionEventNotifier(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IEventNotifier>> out) {
                AMS_ABORT("TODO");
            }

            Result OpenSystemDataUpdateEventNotifier(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IEventNotifier>> out) {
                AMS_ABORT("TODO");
            }

            Result NotifySystemDataUpdateEvent() {
                AMS_ABORT("TODO");
            }

            /* ... */

            Result SetCurrentPosixTime(s64 posix_time) {
                AMS_ABORT("TODO");
            }

            /* ... */

            Result GetRightsId(ams::sf::Out<fs::RightsId> out, ncm::ProgramId program_id, ncm::StorageId storage_id) {
                AMS_ABORT("TODO");
            }

            Result RegisterExternalKey(const fs::RightsId &rights_id, const spl::AccessKey &access_key) {
                AMS_ABORT("TODO");
            }

            Result UnregisterAllExternalKey() {
                AMS_ABORT("TODO");
            }

            Result GetProgramId(ams::sf::Out<ncm::ProgramId> out, const fssrv::sf::FspPath &path, fs::ContentAttributes attr) {
                static_assert(sizeof(ncm::ProgramId) == sizeof(u64));
                R_RETURN(fsGetProgramId(reinterpret_cast<u64 *>(out.GetPointer()), path.str, static_cast<::FsContentAttributes>(static_cast<u8>(attr))));
            }

            Result GetRightsIdByPath(ams::sf::Out<fs::RightsId> out, const fssrv::sf::FspPath &path) {
                static_assert(sizeof(RightsId) == sizeof(::FsRightsId));
                R_RETURN(fsGetRightsIdByPath(path.str, reinterpret_cast<::FsRightsId *>(out.GetPointer())));
            }

            Result GetRightsIdAndKeyGenerationByPathObsolete(ams::sf::Out<fs::RightsId> out, ams::sf::Out<u8> out_key_generation, const fssrv::sf::FspPath &path) {
                R_RETURN(this->GetRightsIdAndKeyGenerationByPath(out, out_key_generation, path, fs::ContentAttributes_None))
            }

            Result GetRightsIdAndKeyGenerationByPath(ams::sf::Out<fs::RightsId> out, ams::sf::Out<u8> out_key_generation, const fssrv::sf::FspPath &path, fs::ContentAttributes attr) {
                static_assert(sizeof(RightsId) == sizeof(::FsRightsId));
                R_RETURN(fsGetRightsIdAndKeyGenerationByPath(path.str, static_cast<::FsContentAttributes>(static_cast<u8>(attr)), out_key_generation.GetPointer(), reinterpret_cast<::FsRightsId *>(out.GetPointer())));
            }

            Result SetCurrentPosixTimeWithTimeDifference(s64 posix_time, s32 time_difference) {
                AMS_ABORT("TODO");
            }

            Result GetFreeSpaceSizeForSaveData(ams::sf::Out<s64> out, u8 space_id) {
                AMS_ABORT("TODO");
            }

            Result VerifySaveDataFileSystemBySaveDataSpaceId() {
                AMS_ABORT("TODO");
            }

            Result CorruptSaveDataFileSystemBySaveDataSpaceId() {
                AMS_ABORT("TODO");
            }

            Result QuerySaveDataInternalStorageTotalSize() {
                AMS_ABORT("TODO");
            }

            Result GetSaveDataCommitId() {
                AMS_ABORT("TODO");
            }

            Result UnregisterExternalKey(const fs::RightsId &rights_id) {
                AMS_ABORT("TODO");
            }

            Result SetSdCardEncryptionSeed(const fs::EncryptionSeed &seed) {
                AMS_ABORT("TODO");
            }

            Result SetSdCardAccessibility(bool accessible) {
                AMS_ABORT("TODO");
            }

            Result IsSdCardAccessible(ams::sf::Out<bool> out) {
                AMS_ABORT("TODO");
            }

            Result IsSignedSystemPartitionOnSdCardValid(ams::sf::Out<bool> out) {
                R_RETURN(fsIsSignedSystemPartitionOnSdCardValid(out.GetPointer()));
            }

            Result OpenAccessFailureDetectionEventNotifier() {
                AMS_ABORT("TODO");
            }

            /* ... */

            Result GetAndClearErrorInfo(ams::sf::Out<fs::FileSystemProxyErrorInfo> out) {
                static_assert(sizeof(fs::FileSystemProxyErrorInfo) == sizeof(::FsFileSystemProxyErrorInfo));
                R_RETURN(::fsGetAndClearErrorInfo(reinterpret_cast<::FsFileSystemProxyErrorInfo *>(out.GetPointer())));
            }

            Result RegisterProgramIndexMapInfo(const ams::sf::InBuffer &buffer, s32 count) {
                AMS_ABORT("TODO");
            }

            Result SetBisRootForHost(u32 id, const fssrv::sf::FspPath &path) {
                AMS_ABORT("TODO");
            }

            Result SetSaveDataSize(s64 size, s64 journal_size) {
                AMS_ABORT("TODO");
            }

            Result SetSaveDataRootPath(const fssrv::sf::FspPath &path) {
                AMS_ABORT("TODO");
            }

            Result DisableAutoSaveDataCreation() {
                R_RETURN(::fsDisableAutoSaveDataCreation());
            }

            Result SetGlobalAccessLogMode(u32 mode) {
                R_RETURN(::fsSetGlobalAccessLogMode(mode));
            }

            Result GetGlobalAccessLogMode(sf::Out<u32> out) {
                R_RETURN(::fsGetGlobalAccessLogMode(out.GetPointer()));
            }

            Result OutputAccessLogToSdCard(const ams::sf::InBuffer &buf) {
                R_RETURN(::fsOutputAccessLogToSdCard(reinterpret_cast<const char *>(buf.GetPointer()), buf.GetSize()));
            }

            Result RegisterUpdatePartition() {
                AMS_ABORT("TODO");
            }

            Result OpenRegisteredUpdatePartition(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out) {
                AMS_ABORT("TODO");
            }

            Result GetAndClearMemoryReportInfo(ams::sf::Out<fs::MemoryReportInfo> out) {
                static_assert(sizeof(fs::MemoryReportInfo) == sizeof(::FsMemoryReportInfo));
                R_RETURN(::fsGetAndClearMemoryReportInfo(reinterpret_cast<::FsMemoryReportInfo *>(out.GetPointer())));
            }

            /* ... */

            Result GetProgramIndexForAccessLog(ams::sf::Out<u32> out_idx, ams::sf::Out<u32> out_count) {
                R_RETURN(::fsGetProgramIndexForAccessLog(out_idx.GetPointer(), out_count.GetPointer()));
            }

            Result GetFsStackUsage(ams::sf::Out<u32> out, u32 type) {
                AMS_ABORT("TODO");
            }

            Result UnsetSaveDataRootPath() {
                AMS_ABORT("TODO");
            }

            Result OutputMultiProgramTagAccessLog() {
                AMS_ABORT("TODO");
            }

            Result FlushAccessLogOnSdCard() {
                AMS_ABORT("TODO");
            }

            Result OutputApplicationInfoAccessLog() {
                AMS_ABORT("TODO");
            }

            Result RegisterDebugConfiguration(u32 key, s64 value) {
                AMS_ABORT("TODO");
            }

            Result UnregisterDebugConfiguration(u32 key) {
                AMS_ABORT("TODO");
            }

            Result OverrideSaveDataTransferTokenSignVerificationKey(const ams::sf::InBuffer &buf) {
                AMS_ABORT("TODO");
            }

            Result CorruptSaveDataFileSystemByOffset(u8 space_id, u64 save_data_id, s64 offset) {
                AMS_ABORT("TODO");
            }

            /* ... */
    };
    static_assert(fssrv::sf::IsIFileSystemProxy<RemoteFileSystemProxy>);

    #pragma GCC diagnostic pop

    #endif

}
