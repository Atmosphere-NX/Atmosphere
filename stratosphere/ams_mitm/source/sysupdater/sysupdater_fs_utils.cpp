/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "sysupdater_fs_utils.hpp"

namespace ams::mitm::sysupdater {

    namespace {

        constexpr inline const char * const NcaExtension = ".nca";
        constexpr inline const char * const NspExtension = ".nsp";
        constexpr inline const size_t NcaExtensionSize = 4;
        constexpr inline const size_t NspExtensionSize = 4;

        static_assert(NcaExtensionSize == NspExtensionSize);
        constexpr inline const size_t NcaNspExtensionSize = NcaExtensionSize;

        constexpr inline std::underlying_type<fssrv::PathNormalizer::Option>::type SdCardContentMetaPathNormalizeOption = fssrv::PathNormalizer::Option_PreserveTailSeparator |
                                                                                                                          fssrv::PathNormalizer::Option_HasMountName;

        Result CheckNcaOrNsp(const char **path) {
            /* Ensure that the path is currently at the mount name delimeter. */
            R_UNLESS(std::strncmp(*path, ams::fs::impl::MountNameDelimiter, strnlen(ams::fs::impl::MountNameDelimiter, ams::fs::EntryNameLengthMax)) == 0, fs::ResultPathNotFound());

            /* Advance past the :. */
            static_assert(ams::fs::impl::MountNameDelimiter[0] == ':');
            *path += 1;

            /* Ensure path is long enough for the extension. */
            const auto path_len = strnlen(*path, ams::fs::EntryNameLengthMax);
            R_UNLESS(path_len > NcaNspExtensionSize, fs::ResultPathNotFound());

            /* Get the extension. */
            const char * const extension = *path + path_len - NcaNspExtensionSize;

            /* Ensure nca or nsp. */
            const bool is_nca = util::Strnicmp(extension, NcaExtension, NcaNspExtensionSize) == 0;
            const bool is_nsp = util::Strnicmp(extension, NspExtension, NcaNspExtensionSize) == 0;
            R_UNLESS(is_nca || is_nsp, fs::ResultPathNotFound());

            return ResultSuccess();
        }

        Result ParseMountName(const char **path, std::shared_ptr<ams::fs::fsa::IFileSystem> *out) {
            /* The equivalent function here supports all the common mount names; we'll only support the SD card, system content storage. */
            if (const auto mount_len = strnlen(ams::fs::impl::SdCardFileSystemMountName, ams::fs::MountNameLengthMax); std::strncmp(*path, ams::fs::impl::SdCardFileSystemMountName, mount_len) == 0) {
                /* Advance the path. */
                *path += mount_len;

                /* Open the SD card. This uses libnx bindings. */
                FsFileSystem fs;
                R_TRY(fsOpenSdCardFileSystem(std::addressof(fs)));

                /* Allocate a new filesystem wrapper. */
                auto fsa = std::make_shared<ams::fs::RemoteFileSystem>(fs);
                R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInSdCardA());

                /* Set the output fs. */
                *out = std::move(fsa);
            } else if (const auto mount_len = strnlen(ams::fs::impl::ContentStorageSystemMountName, ams::fs::MountNameLengthMax); std::strncmp(*path, ams::fs::impl::ContentStorageSystemMountName, mount_len) == 0) {
                /* Advance the path. */
                *path += mount_len;

                /* Open the system content storage. This uses libnx bindings. */
                FsFileSystem fs;
                R_TRY(fsOpenContentStorageFileSystem(std::addressof(fs), FsContentStorageId_System));

                /* Allocate a new filesystem wrapper. */
                auto fsa = std::make_shared<ams::fs::RemoteFileSystem>(fs);
                R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInContentStorageA());

                /* Set the output fs. */
                *out = std::move(fsa);
            } else {
                return fs::ResultPathNotFound();
            }

            /* Ensure that there's something that could be a mount name delimiter. */
            R_UNLESS(strnlen(*path, fs::EntryNameLengthMax) != 0, fs::ResultPathNotFound());

            return ResultSuccess();
        }

        Result ParseNsp(const char **path, std::shared_ptr<ams::fs::fsa::IFileSystem> *out, std::shared_ptr<ams::fs::fsa::IFileSystem> base_fs) {
            const char *work_path = *path;

            /* Advance to the nsp extension. */
            while (true) {
                if (util::Strnicmp(work_path, NspExtension, NspExtensionSize) == 0) {
                    if (work_path[NspExtensionSize] == '\x00' || work_path[NspExtensionSize] == '/') {
                        break;
                    }
                    work_path += NspExtensionSize;
                } else {
                    R_UNLESS(*work_path != '\x00', fs::ResultPathNotFound());

                    work_path += 1;
                }
            }

            /* Advance past the extension. */
            work_path += NspExtensionSize;

            /* Get the nsp path. */
            char nsp_path[fs::EntryNameLengthMax + 1];
            R_UNLESS(static_cast<size_t>(work_path - *path) <= sizeof(nsp_path), fs::ResultTooLongPath());
            std::memcpy(nsp_path, *path, work_path - *path);
            nsp_path[work_path - *path] = '\x00';

            /* Open the file storage. */
            std::shared_ptr<ams::fs::FileStorageBasedFileSystem> file_storage = fssystem::AllocateShared<ams::fs::FileStorageBasedFileSystem>();
            R_UNLESS(file_storage != nullptr, fs::ResultAllocationFailureInFileSystemProxyCoreImplD());
            R_TRY(file_storage->Initialize(std::move(base_fs), nsp_path, ams::fs::OpenMode_Read));

            /* Create a partition fs. */
            R_TRY(fssystem::GetFileSystemCreatorInterfaces()->partition_fs_creator->Create(out, std::move(file_storage)));

            /* Update the path. */
            *path = work_path;

            return ResultSuccess();
        }

        Result ParseNca(const char **path, std::shared_ptr<fssystem::NcaReader> *out, std::shared_ptr<ams::fs::fsa::IFileSystem> base_fs) {
            /* Open the file storage. */
            std::shared_ptr<ams::fs::FileStorageBasedFileSystem> file_storage = fssystem::AllocateShared<ams::fs::FileStorageBasedFileSystem>();
            R_UNLESS(file_storage != nullptr, fs::ResultAllocationFailureInFileSystemProxyCoreImplE());
            R_TRY(file_storage->Initialize(std::move(base_fs), *path, ams::fs::OpenMode_Read));

            /* Create the nca reader. */
            std::shared_ptr<fssystem::NcaReader> nca_reader;
            R_TRY(fssystem::GetFileSystemCreatorInterfaces()->storage_on_nca_creator->CreateNcaReader(std::addressof(nca_reader), file_storage));

            /* NOTE: Here Nintendo validates program ID, but this does not need checking in the meta case. */

            /* Set output reader. */
            *out = std::move(nca_reader);
            return ResultSuccess();
        }

        Result OpenMetaStorage(std::shared_ptr<ams::fs::IStorage> *out, std::shared_ptr<fssystem::NcaReader> nca_reader, fssystem::NcaFsHeader::FsType *out_fs_type) {
            /* Ensure the nca is a meta nca. */
            R_UNLESS(nca_reader->GetContentType() == fssystem::NcaHeader::ContentType::Meta, fs::ResultPreconditionViolation());

            /* We only support SD card ncas, so ensure this isn't a gamecard nca. */
            R_UNLESS(nca_reader->GetDistributionType() != fssystem::NcaHeader::DistributionType::GameCard, fs::ResultPermissionDenied());

            /* Here Nintendo would call GetPartitionIndex(), but we don't need to, because it's meta. */
            constexpr int MetaPartitionIndex = 0;

            /* Open fs header reader. */
            fssystem::NcaFsHeaderReader fs_header_reader;
            R_TRY(fssystem::GetFileSystemCreatorInterfaces()->storage_on_nca_creator->Create(out, std::addressof(fs_header_reader), std::move(nca_reader), MetaPartitionIndex, false));

            /* Set the output fs type. */
            *out_fs_type = fs_header_reader.GetFsType();
            return ResultSuccess();
        }

        Result OpenContentMetaFileSystem(std::shared_ptr<ams::fs::fsa::IFileSystem> *out, const char *path) {
            /* Parse the mount name to get a filesystem. */
            const char *cur_path = path;
            std::shared_ptr<ams::fs::fsa::IFileSystem> base_fs;
            R_TRY(ParseMountName(std::addressof(cur_path), std::addressof(base_fs)));

            /* Ensure the path is an nca or nsp. */
            R_TRY(CheckNcaOrNsp(std::addressof(cur_path)));

            /* Try to parse as nsp. */
            std::shared_ptr<ams::fs::fsa::IFileSystem> nsp_fs;
            if (R_SUCCEEDED(ParseNsp(std::addressof(cur_path), std::addressof(nsp_fs), base_fs))) {
                /* nsp target is only allowed for type package, and we're assuming type meta. */
                R_UNLESS(*path != '\x00', fs::ResultInvalidArgument());

                /* Use the nsp fs as the base fs. */
                base_fs = std::move(nsp_fs);
            }

            /* Parse as nca. */
            std::shared_ptr<fssystem::NcaReader> nca_reader;
            R_TRY(ParseNca(std::addressof(cur_path), std::addressof(nca_reader), std::move(base_fs)));

            /* Open meta storage. */
            std::shared_ptr<ams::fs::IStorage> storage;
            fssystem::NcaFsHeader::FsType fs_type;
            R_TRY(OpenMetaStorage(std::addressof(storage), std::move(nca_reader), std::addressof(fs_type)));

            /* Open the appropriate interface. */
            const auto * const creator_intfs = fssystem::GetFileSystemCreatorInterfaces();
            switch (fs_type) {
                case fssystem::NcaFsHeader::FsType::PartitionFs: return creator_intfs->partition_fs_creator->Create(out, std::move(storage));
                case fssystem::NcaFsHeader::FsType::RomFs:       return creator_intfs->rom_fs_creator->Create(out, std::move(storage));
                default:
                    return fs::ResultInvalidNcaFileSystemType();
            }
        }

    }

    bool PathView::HasPrefix(std::string_view prefix) const {
        return this->path.compare(0, prefix.length(), prefix) == 0;
    }

    bool PathView::HasSuffix(std::string_view suffix) const {
        return this->path.compare(this->path.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    std::string_view PathView::GetFileName() const {
        auto pos = this->path.find_last_of("/");
        return pos != std::string_view::npos ? this->path.substr(pos + 1) : this->path;
    }

    Result MountSdCardContentMeta(const char *mount_name, const char *path) {
        /* Sanitize input. */
        /* NOTE: This is an internal API, so we won't bother with mount name sanitization. */
        R_UNLESS(path != nullptr, fs::ResultInvalidPath());

        /* Normalize the path. */
        fssrv::PathNormalizer normalized_path(path, SdCardContentMetaPathNormalizeOption);
        R_TRY(normalized_path.GetResult());

        /* Open the filesystem. */
        std::shared_ptr<ams::fs::fsa::IFileSystem> fs;
        R_TRY(OpenContentMetaFileSystem(std::addressof(fs), normalized_path.GetPath()));

        /* Create a holder for the fs. */
        std::unique_ptr unique_fs = std::make_unique<ams::fs::SharedFileSystemHolder>(std::move(fs));
        R_UNLESS(unique_fs != nullptr, fs::ResultAllocationFailureInNew());

        /* Register the fs. */
        return ams::fs::fsa::Register(mount_name, std::move(unique_fs));
    }

}
