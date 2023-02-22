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
#include "sysupdater_service.hpp"
#include "sysupdater_async_impl.hpp"
#include "sysupdater_fs_utils.hpp"

namespace ams::mitm::sysupdater {

    namespace {

        /* ExFat NCAs prior to 2.0.0 do not actually include the exfat driver, and don't boot. */
        constexpr inline u32 MinimumVersionForExFatDriver = 65536;

        bool IsExFatDriverSupported(const ncm::ContentMetaInfo &info) {
            return info.version >= MinimumVersionForExFatDriver && ((info.attributes & ncm::ContentMetaAttribute_IncludesExFatDriver) != 0);
        }

        template<typename F>
        Result ForEachFileInDirectory(const char *root_path, F f) {
            /* Open the directory. */
            fs::DirectoryHandle dir;
            R_TRY(fs::OpenDirectory(std::addressof(dir), root_path, fs::OpenDirectoryMode_File));
            ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

            while (true) {
                /* Read the current entry. */
                s64 count;
                fs::DirectoryEntry entry;
                R_TRY(fs::ReadDirectory(std::addressof(count), std::addressof(entry), dir, 1));
                if (count == 0) {
                    break;
                }

                /* Invoke our handler on the entry. */
                bool done;
                R_TRY(f(std::addressof(done), entry));
                R_SUCCEED_IF(done);
            }

            R_SUCCEED();
        }

        Result ConvertToFsCommonPath(char *dst, size_t dst_size, const char *package_root_path, const char *entry_path) {
            char package_path[ams::fs::EntryNameLengthMax];

            const size_t path_len = util::SNPrintf(package_path, sizeof(package_path), "%s%s", package_root_path, entry_path);
            AMS_ABORT_UNLESS(path_len < ams::fs::EntryNameLengthMax);

            R_RETURN(ams::fs::ConvertToFsCommonPath(dst, dst_size, package_path));
        }

        Result LoadContentMeta(ncm::AutoBuffer *out, const char *package_root_path, const fs::DirectoryEntry &entry) {
            AMS_ABORT_UNLESS(PathView(entry.name).HasSuffix(".cnmt.nca"));

            char path[ams::fs::EntryNameLengthMax];
            R_TRY(ConvertToFsCommonPath(path, sizeof(path), package_root_path, entry.name));

            R_RETURN(ncm::TryReadContentMetaPath(out, path, ncm::ReadContentMetaPathAlongWithExtendedDataAndDigest));
        }

        Result ReadContentMetaPath(ncm::AutoBuffer *out, const char *package_root, const ncm::ContentInfo &content_info) {
            /* Get the .cnmt.nca path for the info. */
            char cnmt_nca_name[ncm::ContentIdStringLength + 10];
            ncm::GetStringFromContentId(cnmt_nca_name, sizeof(cnmt_nca_name), content_info.GetId());
            std::memcpy(cnmt_nca_name + ncm::ContentIdStringLength, ".cnmt.nca", std::strlen(".cnmt.nca"));
            cnmt_nca_name[sizeof(cnmt_nca_name) - 1] = '\x00';

            /* Create a new path. */
            ncm::Path content_path;
            R_TRY(ConvertToFsCommonPath(content_path.str, sizeof(content_path.str), package_root, cnmt_nca_name));

            /* Read the content meta path. */
            R_RETURN(ncm::TryReadContentMetaPath(out, content_path.str, ncm::ReadContentMetaPathAlongWithExtendedDataAndDigest));
        }

        Result GetSystemUpdateUpdateContentInfoFromPackage(ncm::ContentInfo *out, const char *package_root) {
            bool found_system_update = false;

            /* Iterate over all files to find the system update meta. */
            R_TRY(ForEachFileInDirectory(package_root, [&](bool *done, const fs::DirectoryEntry &entry) -> Result {
                /* Don't early terminate by default. */
                *done = false;

                /* We have nothing to list if we're not looking at a meta. */
                R_SUCCEED_IF(!PathView(entry.name).HasSuffix(".cnmt.nca"));

                /* Read the content meta path, and build. */
                ncm::AutoBuffer package_meta;
                R_TRY(LoadContentMeta(std::addressof(package_meta), package_root, entry));

                /* Create a reader. */
                const auto reader = ncm::PackagedContentMetaReader(package_meta.Get(), package_meta.GetSize());

                /* If we find a system update, we're potentially done. */
                if (reader.GetHeader()->type == ncm::ContentMetaType::SystemUpdate) {
                    /* Try to parse a content id from the name. */
                    auto content_id = ncm::GetContentIdFromString(entry.name, sizeof(entry.name));
                    R_UNLESS(content_id, ncm::ResultInvalidPackageFormat());

                    /* We're done. */
                    *done = true;
                    found_system_update = true;

                    *out = ncm::ContentInfo::Make(*content_id, entry.file_size, ncm::ContentInfo::DefaultContentAttributes, ncm::ContentType::Meta);
                }

                R_SUCCEED();
            }));

            /* If we didn't find anything, error. */
            R_UNLESS(found_system_update, ncm::ResultSystemUpdateNotFoundInPackage());

            R_SUCCEED();
        }

        Result ValidateSystemUpdate(Result *out_result, Result *out_exfat_result, UpdateValidationInfo *out_info, const ncm::PackagedContentMetaReader &update_reader, const char *package_root) {
            /* Clear output. */
            *out_result       = ResultSuccess();
            *out_exfat_result = ResultSuccess();

            /* We want to track all content the update requires. */
            const size_t num_content_metas = update_reader.GetContentMetaCount();
            bool content_meta_valid[num_content_metas] = {};

            /* Allocate a buffer to use for validation. */
            size_t data_buffer_size = 1_MB;
            void *data_buffer;
            do {
                data_buffer = std::malloc(data_buffer_size);
                if (data_buffer != nullptr) {
                    break;
                }

                data_buffer_size /= 2;
            } while (data_buffer_size >= 16_KB);
            R_UNLESS(data_buffer != nullptr, fs::ResultAllocationMemoryFailedNew());

            ON_SCOPE_EXIT { std::free(data_buffer); };

            /* Declare helper for result validation. */
            auto ValidateResult = [&](Result result) ALWAYS_INLINE_LAMBDA -> Result {
                *out_result = result;
                R_RETURN(result);
            };

            /* Iterate over all files to find all content metas. */
            R_TRY(ForEachFileInDirectory(package_root, [&](bool *done, const fs::DirectoryEntry &entry) -> Result {
                /* Clear output. */
                *out_info = {};

                /* Don't early terminate by default. */
                *done = false;

                /* We have nothing to list if we're not looking at a meta. */
                R_SUCCEED_IF(!PathView(entry.name).HasSuffix(".cnmt.nca"));

                /* Read the content meta path, and build. */
                ncm::AutoBuffer package_meta;
                R_TRY(LoadContentMeta(std::addressof(package_meta), package_root, entry));

                /* Create a reader. */
                const auto reader = ncm::PackagedContentMetaReader(package_meta.Get(), package_meta.GetSize());

                /* Get the key for the reader. */
                const auto key = reader.GetKey();

                /* Check if we need to validate this content. */
                bool need_validate = false;
                size_t validation_index = 0;
                for (size_t i = 0; i < num_content_metas; ++i) {
                    if (update_reader.GetContentMetaInfo(i)->ToKey() == key) {
                        need_validate = true;
                        validation_index = i;
                        break;
                    }
                }

                /* If we don't need to validate, continue. */
                R_SUCCEED_IF(!need_validate);

                /* We're validating. */
                out_info->invalid_key = key;

                /* Validate all contents. */
                for (size_t i = 0; i < reader.GetContentCount(); ++i) {
                    const auto *content_info = reader.GetContentInfo(i);
                    const auto &content_id   = content_info->GetId();
                    const s64 content_size   = content_info->info.GetSize();
                    out_info->invalid_content_id = content_id;

                    /* Get the content id string. */
                    auto content_id_str = ncm::GetContentIdString(content_id);

                    /* Open the file. */
                    fs::FileHandle file;
                    {
                        char path[fs::EntryNameLengthMax];
                        util::SNPrintf(path, sizeof(path), "%s%s%s", package_root, content_id_str.data, content_info->GetType() == ncm::ContentType::Meta ? ".cnmt.nca" : ".nca");
                        if (R_FAILED(ValidateResult(fs::OpenFile(std::addressof(file), path, ams::fs::OpenMode_Read)))) {
                            *done = true;
                            R_SUCCEED();
                        }
                    }
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    /* Validate the file size is correct. */
                    s64 file_size;
                    if (R_FAILED(ValidateResult(fs::GetFileSize(std::addressof(file_size), file)))) {
                        *done = true;
                        R_SUCCEED();
                    }
                    if (file_size != content_size) {
                        *out_result = ncm::ResultInvalidContentHash();
                        *done = true;
                        R_SUCCEED();
                    }

                    /* Read and hash the file in chunks. */
                    crypto::Sha256Generator sha;
                    sha.Initialize();

                    s64 ofs = 0;
                    while (ofs < content_size) {
                        const size_t cur_size = std::min(static_cast<size_t>(content_size - ofs), data_buffer_size);
                        if (R_FAILED(ValidateResult(fs::ReadFile(file, ofs, data_buffer, cur_size)))) {
                            *done = true;
                            R_SUCCEED();
                        }

                        sha.Update(data_buffer, cur_size);

                        ofs += cur_size;
                    }

                    /* Get the hash. */
                    ncm::Digest calc_digest;
                    sha.GetHash(std::addressof(calc_digest), sizeof(calc_digest));

                    /* Validate the hash. */
                    if (std::memcmp(std::addressof(calc_digest), std::addressof(content_info->digest), sizeof(ncm::Digest)) != 0) {
                        *out_result = ncm::ResultInvalidContentHash();
                        *done = true;
                        R_SUCCEED();
                    }
                }

                /* Mark the relevant content as validated. */
                content_meta_valid[validation_index] = true;
                *out_info = {};

                R_SUCCEED();
            }));

            /* If we're otherwise going to succeed, ensure that every content was found. */
            if (R_SUCCEEDED(*out_result)) {
                for (size_t i = 0; i < num_content_metas; ++i) {
                    if (!content_meta_valid[i]) {
                        const ncm::ContentMetaInfo *info = update_reader.GetContentMetaInfo(i);

                        *out_info = { .invalid_key = info->ToKey(), };

                        if (IsExFatDriverSupported(*info)) {
                            *out_exfat_result = fs::ResultPathNotFound();
                            /* Continue, in case there's a non-exFAT failure result. */
                        } else {
                            *out_result = fs::ResultPathNotFound();
                            break;
                        }
                    }
                }
            }

            R_SUCCEED();
        }

        Result FormatUserPackagePath(ncm::Path *out, const ncm::Path &user_path) {
            /* Ensure that the user path is valid. */
            R_UNLESS(user_path.str[0] == '/', fs::ResultInvalidPath());

            /* Print as @Sdcard:<user_path>/ */
            util::SNPrintf(out->str, sizeof(out->str), "%s:%s/", ams::fs::impl::SdCardFileSystemMountName, user_path.str);

            /* Normalize, if the user provided an ending / */
            const size_t len = std::strlen(out->str);
            if (out->str[len - 1] == '/' && out->str[len - 2] == '/') {
                out->str[len - 1] = '\x00';
            }

            R_SUCCEED();
        }

        const char *GetFirmwareVariationSettingName(settings::system::PlatformRegion region) {
            switch (region) {
                case settings::system::PlatformRegion_Global: return "firmware_variation";
                case settings::system::PlatformRegion_China:  return "t_firmware_variation";
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        ncm::FirmwareVariationId GetFirmwareVariationId() {
            /* Get the firmware variation setting name. */
            const char * const setting_name = GetFirmwareVariationSettingName(settings::system::GetPlatformRegion());

            /* Retrieve the firmware variation id. */
            ncm::FirmwareVariationId id = {};
            settings::fwdbg::GetSettingsItemValue(std::addressof(id.value), sizeof(u8), "ns.systemupdate", setting_name);

            return id;
        }

    }

    Result SystemUpdateService::GetUpdateInformation(sf::Out<UpdateInformation> out, const ncm::Path &path) {
        /* Adjust the path. */
        ncm::Path package_root;
        R_TRY(FormatUserPackagePath(std::addressof(package_root), path));

        /* Create a new update information. */
        UpdateInformation update_info = {};

        /* Parse the update. */
        {
            /* Get the content info for the system update. */
            ncm::ContentInfo content_info;
            R_TRY(GetSystemUpdateUpdateContentInfoFromPackage(std::addressof(content_info), package_root.str));

            /* Read the content meta. */
            ncm::AutoBuffer content_meta_buffer;
            R_TRY(ReadContentMetaPath(std::addressof(content_meta_buffer), package_root.str, content_info));

            /* Create a reader. */
            const auto reader = ncm::PackagedContentMetaReader(content_meta_buffer.Get(), content_meta_buffer.GetSize());

            /* Get the version from the header. */
            update_info.version = reader.GetHeader()->version;

            /* Iterate over infos to find the system update info. */
            for (size_t i = 0; i < reader.GetContentMetaCount(); ++i) {
                const auto &meta_info = *reader.GetContentMetaInfo(i);

                switch (meta_info.type) {
                    case ncm::ContentMetaType::BootImagePackage:
                        /* Detect exFAT support. */
                        update_info.exfat_supported |= IsExFatDriverSupported(meta_info);
                        break;
                    default:
                        break;
                }
            }

            /* Default to no firmware variations. */
            update_info.firmware_variation_count = 0;

            /* Parse firmware variations if relevant. */
            if (reader.GetExtendedDataSize() != 0) {
                /* Get the actual firmware variation count. */
                ncm::SystemUpdateMetaExtendedDataReader extended_data_reader(reader.GetExtendedData(), reader.GetExtendedDataSize());
                update_info.firmware_variation_count = extended_data_reader.GetFirmwareVariationCount();

                /* NOTE: Update this if Nintendo ever actually releases an update with this many variations? */
                R_UNLESS(update_info.firmware_variation_count <= FirmwareVariationCountMax, ncm::ResultInvalidFirmwareVariation());

                for (size_t i = 0; i < update_info.firmware_variation_count; ++i) {
                    update_info.firmware_variation_ids[i] = *extended_data_reader.GetFirmwareVariationId(i);
                }
            }
        }

        /* Set the parsed update info. */
        out.SetValue(update_info);
        R_SUCCEED();
    }

    Result SystemUpdateService::ValidateUpdate(sf::Out<Result> out_validate_result, sf::Out<Result> out_validate_exfat_result, sf::Out<UpdateValidationInfo> out_validate_info, const ncm::Path &path) {
        /* Adjust the path. */
        ncm::Path package_root;
        R_TRY(FormatUserPackagePath(std::addressof(package_root), path));

        /* Parse the update. */
        {
            /* Get the content info for the system update. */
            ncm::ContentInfo content_info;
            R_TRY(GetSystemUpdateUpdateContentInfoFromPackage(std::addressof(content_info), package_root.str));

            /* Read the content meta. */
            ncm::AutoBuffer content_meta_buffer;
            R_TRY(ReadContentMetaPath(std::addressof(content_meta_buffer), package_root.str, content_info));

            /* Create a reader. */
            const auto reader = ncm::PackagedContentMetaReader(content_meta_buffer.Get(), content_meta_buffer.GetSize());

            /* Validate the update. */
            R_TRY(ValidateSystemUpdate(out_validate_result.GetPointer(), out_validate_exfat_result.GetPointer(), out_validate_info.GetPointer(), reader, package_root.str));
        }

        R_SUCCEED();
    };

    Result SystemUpdateService::SetupUpdate(sf::CopyHandle &&transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat) {
        R_RETURN(this->SetupUpdateImpl(std::move(transfer_memory), transfer_memory_size, path, exfat, GetFirmwareVariationId()));
    }

    Result SystemUpdateService::SetupUpdateWithVariation(sf::CopyHandle &&transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id) {
        R_RETURN(this->SetupUpdateImpl(std::move(transfer_memory), transfer_memory_size, path, exfat, firmware_variation_id));
    }

    Result SystemUpdateService::RequestPrepareUpdate(sf::OutCopyHandle out_event_handle, sf::Out<sf::SharedPointer<ns::impl::IAsyncResult>> out_async) {
        /* Ensure the update is setup but not prepared. */
        R_UNLESS(m_setup_update,      ns::ResultCardUpdateNotSetup());
        R_UNLESS(!m_requested_update, ns::ResultPrepareCardUpdateAlreadyRequested());

        /* Create the async result. */
        auto async_result = sf::CreateSharedObjectEmplaced<ns::impl::IAsyncResult, AsyncPrepareSdCardUpdateImpl>(std::addressof(*m_update_task));
        R_UNLESS(async_result != nullptr, ns::ResultOutOfMaxRunningTask());

        /* Run the task. */
        R_TRY(async_result.GetImpl().Run());

        /* We prepared the task! */
        m_requested_update = true;
        out_event_handle.SetValue(async_result.GetImpl().GetEvent().GetReadableHandle(), false);
        *out_async = std::move(async_result);

        R_SUCCEED();
    }

    Result SystemUpdateService::GetPrepareUpdateProgress(sf::Out<SystemUpdateProgress> out) {
        /* Ensure the update is setup. */
        R_UNLESS(m_setup_update, ns::ResultCardUpdateNotSetup());

        /* Get the progress. */
        auto install_progress = m_update_task->GetProgress();
        out.SetValue({ .current_size = install_progress.installed_size, .total_size = install_progress.total_size });
        R_SUCCEED();
    }

    Result SystemUpdateService::HasPreparedUpdate(sf::Out<bool> out) {
        /* Ensure the update is setup. */
        R_UNLESS(m_setup_update, ns::ResultCardUpdateNotSetup());

        out.SetValue(m_update_task->GetProgress().state == ncm::InstallProgressState::Downloaded);
        R_SUCCEED();
    }

    Result SystemUpdateService::ApplyPreparedUpdate() {
        /* Ensure the update is setup. */
        R_UNLESS(m_setup_update, ns::ResultCardUpdateNotSetup());

        /* Ensure the update is prepared. */
        R_UNLESS(m_update_task->GetProgress().state == ncm::InstallProgressState::Downloaded, ns::ResultCardUpdateNotPrepared());

        /* Apply the task. */
        R_TRY(m_apply_manager.ApplyPackageTask(std::addressof(*m_update_task)));

        R_SUCCEED();
    }

    Result SystemUpdateService::SetupUpdateImpl(sf::NativeHandle &&transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id) {
        /* Ensure we don't already have an update set up. */
        R_UNLESS(!m_setup_update, ns::ResultCardUpdateAlreadySetup());

        /* Destroy any existing update tasks. */
        nim::SystemUpdateTaskId id;
        auto count = nim::ListSystemUpdateTask(std::addressof(id), 1);
        if (count > 0) {
            R_TRY(nim::DestroySystemUpdateTask(id));
        }

        /* Initialize the update task. */
        R_TRY(InitializeUpdateTask(std::move(transfer_memory), transfer_memory_size, path, exfat, firmware_variation_id));

        /* The update is now set up. */
        m_setup_update = true;
        R_SUCCEED();
    }

    Result SystemUpdateService::InitializeUpdateTask(sf::NativeHandle &&transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id) {
        /* Map the transfer memory. */
        const size_t tmem_buffer_size = static_cast<size_t>(transfer_memory_size);
        m_update_transfer_memory.emplace(tmem_buffer_size, transfer_memory.GetOsHandle(), transfer_memory.IsManaged());
        transfer_memory.Detach();

        void *tmem_buffer;
        R_TRY(m_update_transfer_memory->Map(std::addressof(tmem_buffer), os::MemoryPermission_None));
        auto tmem_guard = SCOPE_GUARD {
            m_update_transfer_memory->Unmap();
            m_update_transfer_memory = util::nullopt;
        };

        /* Adjust the package root. */
        ncm::Path package_root;
        R_TRY(FormatUserPackagePath(std::addressof(package_root), path));

        /* Ensure that we can create an update context. */
        R_TRY(fs::EnsureDirectory("@Sdcard:/atmosphere/update/"));
        const char *context_path = "@Sdcard:/atmosphere/update/cup.ctx";

        /* Create and initialize the update task. */
        m_update_task.emplace();
        R_TRY(m_update_task->Initialize(package_root.str, context_path, tmem_buffer, tmem_buffer_size, exfat, firmware_variation_id));

        /* We successfully setup the update. */
        tmem_guard.Cancel();

        R_SUCCEED();
    }

}
