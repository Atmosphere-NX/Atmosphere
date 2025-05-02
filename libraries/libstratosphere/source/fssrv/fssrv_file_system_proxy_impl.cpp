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
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>
#include "impl/fssrv_allocator_for_service_framework.hpp"
#include "impl/fssrv_program_info.hpp"

namespace ams::fssrv {

    FileSystemProxyImpl::FileSystemProxyImpl() {
        /* TODO: Set core impl. */
        m_process_id = os::InvalidProcessId.value;
    }

    FileSystemProxyImpl::~FileSystemProxyImpl() {
        /* ... */
    }

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-parameter"

    Result FileSystemProxyImpl::OpenFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, u32 type) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::FileSystemProxyImpl::SetCurrentProcess(const ams::sf::ClientProcessId &client_pid) {
        /* Set current process. */
        m_process_id = client_pid.GetValue().value;

        /* TODO: Allocate NcaFileSystemService. */
        /* TODO: Allocate SaveDataFileSystemService. */
        R_SUCCEED();
    }

    Result FileSystemProxyImpl::OpenDataFileSystemByCurrentProcess(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenFileSystemWithPatch(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, ncm::ProgramId program_id, u32 type) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenFileSystemWithIdObsolete(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, u64 program_id, u32 type) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenFileSystemWithId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, fs::ContentAttributes attr, u64 program_id, u32 type) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenDataFileSystemByProgramId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, ncm::ProgramId program_id) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenBisFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, u32 id) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenBisStorage(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, u32 id) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::InvalidateBisCache() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenHostFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path) {
        /* Invoke the modern API from the legacy API. */
        R_RETURN(this->OpenHostFileSystemWithOption(out, path, fs::MountHostOption::None._value));
    }

    Result FileSystemProxyImpl::OpenSdCardFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::FormatSdCardFileSystem() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::DeleteSaveDataFileSystem(u64 save_data_id) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::CreateSaveDataFileSystem(const fs::SaveDataAttribute &attribute, const fs::SaveDataCreationInfo &creation_info, const fs::SaveDataMetaInfo &meta_info) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::CreateSaveDataFileSystemBySystemSaveDataId(const fs::SaveDataAttribute &attribute, const fs::SaveDataCreationInfo &creation_info) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::RegisterSaveDataFileSystemAtomicDeletion(const ams::sf::InBuffer &save_data_ids) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::DeleteSaveDataFileSystemBySaveDataSpaceId(u8 indexer_space_id, u64 save_data_id) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::FormatSdCardDryRun() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::IsExFatSupported(ams::sf::Out<bool> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::DeleteSaveDataFileSystemBySaveDataAttribute(u8 space_id, const fs::SaveDataAttribute &attribute) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenGameCardStorage(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, u32 handle, u32 partition) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenGameCardFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u32 handle, u32 partition) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::ExtendSaveDataFileSystem(u8 space_id, u64 save_data_id, s64 available_size, s64 journal_size) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::DeleteCacheStorage(u16 index) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::GetCacheStorageSize(ams::sf::Out<s64> out_size, ams::sf::Out<s64> out_journal_size, u16 index) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::CreateSaveDataFileSystemWithHashSalt(const fs::SaveDataAttribute &attribute, const fs::SaveDataCreationInfo &creation_info, const fs::SaveDataMetaInfo &meta_info, const fs::HashSalt &salt) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenHostFileSystemWithOption(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, const fssrv::sf::FspPath &path, u32 _option) {
        /* TODO: GetProgramInfo */
        /* TODO: GetAccessibility. */
        /* TODO: Check Accessibility CanRead/CanWrite */

        /* Convert the path. */
        fs::Path normalized_path;
        #if defined(ATMOSPHERE_OS_WINDOWS)
        R_TRY(normalized_path.Initialize(path.str));
        #else
        if (path.str[0] == '/' && path.str[1] == '/') {
            R_TRY(normalized_path.Initialize(path.str));
        } else {
            R_TRY(normalized_path.InitializeWithReplaceUnc(path.str));
        }
        #endif

        /* Normalize the path. */
        fs::PathFlags path_flags;
        path_flags.AllowWindowsPath();
        path_flags.AllowRelativePath();
        path_flags.AllowEmptyPath();
        R_TRY(normalized_path.Normalize(path_flags));

        /* Parse option. */
        const fs::MountHostOption option{ _option };

        /* TODO: FileSystemProxyCoreImpl::OpenHostFileSystem */
        /* TODO: use creator interfaces */
        std::shared_ptr<fs::fsa::IFileSystem> fs;
        {
            fssrv::fscreator::LocalFileSystemCreator local_fs_creator(true);

            R_TRY(static_cast<fscreator::ILocalFileSystemCreator &>(local_fs_creator).Create(std::addressof(fs), normalized_path, option.HasPseudoCaseSensitiveFlag()));
        }

        /* Determine path flags for the result fs. */
        fs::PathFlags host_path_flags;
        if (path.str[0] == 0) {
            host_path_flags.AllowWindowsPath();
        }

        /* Create an interface adapter. */
        auto sf_fs = impl::FileSystemObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystem, impl::FileSystemInterfaceAdapter>(std::move(fs), host_path_flags, false);
        R_UNLESS(sf_fs != nullptr, fs::ResultAllocationMemoryFailedInFileSystemProxyImplA());

        /* Set the output. */
        *out = std::move(sf_fs);
        R_SUCCEED();
    }

    Result FileSystemProxyImpl::OpenSaveDataFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u8 space_id, const fs::SaveDataAttribute &attribute) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenSaveDataFileSystemBySystemSaveDataId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u8 space_id, const fs::SaveDataAttribute &attribute) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenReadOnlySaveDataFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u8 space_id, const fs::SaveDataAttribute &attribute) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::ReadSaveDataFileSystemExtraDataBySaveDataSpaceId(const ams::sf::OutBuffer &buffer, u8 space_id, u64 save_data_id) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::ReadSaveDataFileSystemExtraData(const ams::sf::OutBuffer &buffer, u64 save_data_id) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::WriteSaveDataFileSystemExtraData(u64 save_data_id, u8 space_id, const ams::sf::InBuffer &buffer) {
        AMS_ABORT("TODO");
    }

    /* ... */

    Result FileSystemProxyImpl::OpenImageDirectoryFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u32 id) {
        AMS_ABORT("TODO");
    }

    /* ... */

    Result FileSystemProxyImpl::OpenContentStorageFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u32 id) {
        AMS_ABORT("TODO");
    }

    /* ... */

    Result FileSystemProxyImpl::OpenDataStorageByCurrentProcess(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenDataStorageByProgramId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, ncm::ProgramId program_id) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenDataStorageByDataId(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, ncm::DataId data_id, u8 storage_id) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenPatchDataStorageByCurrentProcess(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenDataFileSystemWithProgramIndex(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out, u8 index) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenDataStorageWithProgramIndex(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, u8 index) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenDataStorageByPathObsolete(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, const fssrv::sf::FspPath &path, u32 type) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenDataStorageByPath(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IStorage>> out, const fssrv::sf::FspPath &path, fs::ContentAttributes attr, u32 type) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenDeviceOperator(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IDeviceOperator>> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenSdCardDetectionEventNotifier(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IEventNotifier>> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenGameCardDetectionEventNotifier(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IEventNotifier>> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenSystemDataUpdateEventNotifier(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IEventNotifier>> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::NotifySystemDataUpdateEvent() {
        AMS_ABORT("TODO");
    }

    /* ... */

    Result FileSystemProxyImpl::SetCurrentPosixTime(s64 posix_time) {
        AMS_ABORT("TODO");
    }

    /* ... */

    Result FileSystemProxyImpl::GetRightsId(ams::sf::Out<fs::RightsId> out, ncm::ProgramId program_id, ncm::StorageId storage_id) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::RegisterExternalKey(const fs::RightsId &rights_id, const spl::AccessKey &access_key) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::UnregisterAllExternalKey() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::GetProgramId(ams::sf::Out<ncm::ProgramId> out_program_id, const fssrv::sf::FspPath &path, fs::ContentAttributes attr) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::GetRightsIdByPath(ams::sf::Out<fs::RightsId> out, const fssrv::sf::FspPath &path) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::GetRightsIdAndKeyGenerationByPathObsolete(ams::sf::Out<fs::RightsId> out, ams::sf::Out<u8> out_key_generation, const fssrv::sf::FspPath &path) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::GetRightsIdAndKeyGenerationByPath(ams::sf::Out<fs::RightsId> out, ams::sf::Out<u8> out_key_generation, const fssrv::sf::FspPath &path, fs::ContentAttributes attr) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::SetCurrentPosixTimeWithTimeDifference(s64 posix_time, s32 time_difference) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::GetFreeSpaceSizeForSaveData(ams::sf::Out<s64> out, u8 space_id) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::VerifySaveDataFileSystemBySaveDataSpaceId() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::CorruptSaveDataFileSystemBySaveDataSpaceId() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::QuerySaveDataInternalStorageTotalSize() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::GetSaveDataCommitId() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::UnregisterExternalKey(const fs::RightsId &rights_id) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::SetSdCardEncryptionSeed(const fs::EncryptionSeed &seed) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::SetSdCardAccessibility(bool accessible) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::IsSdCardAccessible(ams::sf::Out<bool> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::IsSignedSystemPartitionOnSdCardValid(ams::sf::Out<bool> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenAccessFailureDetectionEventNotifier() {
        AMS_ABORT("TODO");
    }

    /* ... */

    Result FileSystemProxyImpl::GetAndClearErrorInfo(ams::sf::Out<fs::FileSystemProxyErrorInfo> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::RegisterProgramIndexMapInfo(const ams::sf::InBuffer &buffer, s32 count) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::SetBisRootForHost(u32 id, const fssrv::sf::FspPath &path) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::SetSaveDataSize(s64 size, s64 journal_size) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::SetSaveDataRootPath(const fssrv::sf::FspPath &path) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::DisableAutoSaveDataCreation() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::SetGlobalAccessLogMode(u32 mode) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::GetGlobalAccessLogMode(ams::sf::Out<u32> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OutputAccessLogToSdCard(const ams::sf::InBuffer &buf) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::RegisterUpdatePartition() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OpenRegisteredUpdatePartition(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::GetAndClearMemoryReportInfo(ams::sf::Out<fs::MemoryReportInfo> out) {
        AMS_ABORT("TODO");
    }

    /* ... */

    Result FileSystemProxyImpl::GetProgramIndexForAccessLog(ams::sf::Out<u32> out_idx, ams::sf::Out<u32> out_count) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::GetFsStackUsage(ams::sf::Out<u32> out, u32 type) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::UnsetSaveDataRootPath() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OutputMultiProgramTagAccessLog() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::FlushAccessLogOnSdCard() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OutputApplicationInfoAccessLog() {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::RegisterDebugConfiguration(u32 key, s64 value) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::UnregisterDebugConfiguration(u32 key) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::OverrideSaveDataTransferTokenSignVerificationKey(const ams::sf::InBuffer &buf) {
        AMS_ABORT("TODO");
    }

    Result FileSystemProxyImpl::CorruptSaveDataFileSystemByOffset(u8 space_id, u64 save_data_id, s64 offset) {
        AMS_ABORT("TODO");
    }

    /* ... */

    #pragma GCC diagnostic pop

    Result FileSystemProxyImpl::OpenCodeFileSystemDeprecated(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const fssrv::sf::Path &path, ncm::ProgramId program_id) {
        AMS_ABORT("TODO");
        AMS_UNUSED(out_fs, path, program_id);
    }

    Result FileSystemProxyImpl::OpenCodeFileSystemDeprecated2(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, ncm::ProgramId program_id) {
        AMS_ABORT("TODO");
        AMS_UNUSED(out_fs, out_verif, path, program_id);
    }

    Result FileSystemProxyImpl::OpenCodeFileSystemDeprecated3(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, fs::ContentAttributes attr, ncm::ProgramId program_id) {
        AMS_ABORT("TODO");
        AMS_UNUSED(out_fs, out_verif, path, attr, program_id);
    }

    Result FileSystemProxyImpl::OpenCodeFileSystemDeprecated4(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const ams::sf::OutBuffer &out_verif, const fssrv::sf::Path &path, fs::ContentAttributes attr, ncm::ProgramId program_id) {
        AMS_ABORT("TODO");
        AMS_UNUSED(out_fs, out_verif, path, attr, program_id);
    }

    Result FileSystemProxyImpl::OpenCodeFileSystem(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const ams::sf::OutBuffer &out_verif, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id) {
        AMS_ABORT("TODO");
        AMS_UNUSED(out_fs, out_verif, attr, program_id, storage_id);
    }

    Result FileSystemProxyImpl::IsArchivedProgram(ams::sf::Out<bool> out, u64 process_id) {
        AMS_ABORT("TODO");
        AMS_UNUSED(out, process_id);
    }

}
