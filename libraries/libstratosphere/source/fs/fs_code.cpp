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
#include "fsa/fs_mount_utils.hpp"
#include "impl/fs_file_system_service_object_adapter.hpp"
#include "impl/fs_file_system_proxy_service_object.hpp"

namespace ams::fs {

    namespace {

        constinit os::SdkMutex g_mount_stratosphere_romfs_lock;
        constinit bool g_mounted_stratosphere_romfs = false;

        constinit util::TypedStorage<FileHandleStorage> g_stratosphere_romfs_storage = {};
        constinit util::TypedStorage<RomFsFileSystem> g_stratosphere_romfs_fs = {};

        Result EnsureStratosphereRomfsMounted() {
            std::scoped_lock lk(g_mount_stratosphere_romfs_lock);

            if (AMS_UNLIKELY(!g_mounted_stratosphere_romfs)) {
                /* Mount the SD card. */
                R_TRY(fs::MountSdCard("#strat-romfs-sd"));
                auto sd_guard = SCOPE_GUARD { fs::Unmount("#strat-romfs-sd"); };

                /* Open sd:/atmosphere/stratosphere.romfs. */
                fs::FileHandle stratosphere_romfs_file;
                R_TRY(fs::OpenFile(std::addressof(stratosphere_romfs_file), "#strat-romfs-sd:/atmosphere/stratosphere.romfs", fs::OpenMode_Read));

                /* Setup the storage. */
                /* NOTE: This owns the file, and so on failure it will be closed appropriately. */
                auto storage_guard = util::ConstructAtGuarded(g_stratosphere_romfs_storage, stratosphere_romfs_file, true);

                /* Create the filesystem. */
                auto fs_guard = util::ConstructAtGuarded(g_stratosphere_romfs_fs);

                /* Initialize the filesystem. */
                R_TRY(GetReference(g_stratosphere_romfs_fs).Initialize(GetPointer(g_stratosphere_romfs_storage), nullptr, 0, false));

                /* We succeeded, and so stratosphere.romfs is mounted. */
                fs_guard.Cancel();
                storage_guard.Cancel();
                sd_guard.Cancel();

                g_mounted_stratosphere_romfs = true;
            }

            R_SUCCEED();
        }

        fsa::IFileSystem &GetStratosphereRomFsFileSystem() {
            /* Ensure that stratosphere.romfs is mounted. */
            /* NOTE: Abort is used here to ensure that atmosphere's filesystem is structurally valid. */
            R_ABORT_UNLESS(EnsureStratosphereRomfsMounted());

            return GetReference(g_stratosphere_romfs_fs);
        }

        Result OpenCodeFileSystemImpl(CodeVerificationData *out_verification_data, std::unique_ptr<fsa::IFileSystem> *out, const char *path, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id) {
            /* Print a path suitable for the remote service. */
            fssrv::sf::Path sf_path;
            R_TRY(FormatToFspPath(std::addressof(sf_path), "%s", path));

            /* Open the filesystem. */
            auto fsp = impl::GetFileSystemProxyForLoaderServiceObject();
            R_TRY(fsp->SetCurrentProcess({}));

            sf::SharedPointer<fssrv::sf::IFileSystem> fs;
            if (hos::GetVersion() >= hos::Version_20_0_0) {
                R_TRY(fsp->OpenCodeFileSystem(std::addressof(fs), ams::sf::OutBuffer(out_verification_data, sizeof(*out_verification_data)), attr, program_id, storage_id));
            } else {
                R_TRY(fsp->OpenCodeFileSystemDeprecated4(std::addressof(fs), ams::sf::OutBuffer(out_verification_data, sizeof(*out_verification_data)), sf_path, attr, program_id));
            }

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<impl::FileSystemServiceObjectAdapter>(std::move(fs));
            R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInCodeA());

            *out = std::move(fsa);
            R_SUCCEED();
        }

        Result OpenPackageFileSystemImpl(std::unique_ptr<fsa::IFileSystem> *out, const fssrv::sf::FspPath &path) {
            /* Open the filesystem. */
            auto fsp = impl::GetFileSystemProxyServiceObject();
            sf::SharedPointer<fssrv::sf::IFileSystem> fs;
            R_TRY(fsp->OpenFileSystemWithId(std::addressof(fs), path, fs::ContentAttributes_None, ncm::InvalidProgramId.value, impl::FileSystemProxyType_Package));

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<impl::FileSystemServiceObjectAdapter>(std::move(fs));
            R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInCodeA());

            *out = std::move(fsa);
            R_SUCCEED();
        }

        Result OpenSdCardCodeFileSystemImpl(std::unique_ptr<fsa::IFileSystem> *out, ncm::ProgramId program_id) {
            /* Ensure we don't access the SD card too early. */
            R_UNLESS(cfg::IsSdCardInitialized(), fs::ResultSdCardNotPresent());

            /* Print a path to the program's package. */
            fssrv::sf::Path sf_path;
            R_TRY(FormatToFspPath(std::addressof(sf_path), "%s:/atmosphere/contents/%016" PRIx64 "/exefs.nsp", impl::SdCardFileSystemMountName, program_id.value));

            R_RETURN(OpenPackageFileSystemImpl(out, sf_path));
        }

        Result OpenStratosphereCodeFileSystemImpl(std::unique_ptr<fsa::IFileSystem> *out, ncm::ProgramId program_id) {
            /* Ensure we don't access the SD card too early. */
            R_UNLESS(cfg::IsSdCardInitialized(), fs::ResultSdCardNotPresent());

            /* Open the program's package. */
            std::unique_ptr<fsa::IFile> package_file;
            {
                /* Get the stratosphere.romfs filesystem. */
                auto &romfs_fs = GetStratosphereRomFsFileSystem();

                /* Print a path to the program's package. */
                fs::Path path;
                R_TRY(path.InitializeWithFormat("/atmosphere/contents/%016" PRIx64 "/exefs.nsp", program_id.value));
                R_TRY(path.Normalize(fs::PathFlags{}));

                /* Open the package within stratosphere.romfs. */
                R_TRY(romfs_fs.OpenFile(std::addressof(package_file), path, fs::OpenMode_Read));
            }

            /* Create a file storage for the program's package. */
            auto package_storage = fs::AllocateShared<FileStorage>(std::move(package_file));
            R_UNLESS(package_storage != nullptr, fs::ResultAllocationMemoryFailedInCodeA());

            /* Create a partition filesystem. */
            auto package_fs = std::make_unique<fssystem::PartitionFileSystem>();
            R_UNLESS(package_fs != nullptr, fs::ResultAllocationMemoryFailedInCodeA());

            /* Initialize the partition filesystem. */
            R_TRY(package_fs->Initialize(package_storage));

            *out = std::move(package_fs);
            R_SUCCEED();
        }

        Result OpenSdCardCodeOrStratosphereCodeOrCodeFileSystemImpl(CodeVerificationData *out_verification_data, std::unique_ptr<fsa::IFileSystem> *out, const char *path, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id) {
            /* If we can open an sd card code fs, use it. */
            R_SUCCEED_IF(R_SUCCEEDED(OpenSdCardCodeFileSystemImpl(out, program_id)));

            /* If we can open a stratosphere code fs, use it. */
            R_SUCCEED_IF(R_SUCCEEDED(OpenStratosphereCodeFileSystemImpl(out, program_id)));

            /* Otherwise, fall back to a normal code fs. */
            R_RETURN(OpenCodeFileSystemImpl(out_verification_data, out, path, attr, program_id, storage_id));
        }

        Result OpenHblCodeFileSystemImpl(std::unique_ptr<fsa::IFileSystem> *out) {
            /* Get the HBL path. */
            const char *hbl_path = cfg::GetHblPath();

            /* Print a path to the hbl package. */
            fssrv::sf::Path sf_path;
            R_TRY(FormatToFspPath(std::addressof(sf_path), "%s:/%s", impl::SdCardFileSystemMountName, hbl_path[0] == '/' ? hbl_path + 1 : hbl_path));

            R_RETURN(OpenPackageFileSystemImpl(out, sf_path));
        }

        Result OpenSdCardFileSystemImpl(std::shared_ptr<fsa::IFileSystem> *out) {
            /* Open the SD card filesystem. */
            auto fsp = impl::GetFileSystemProxyServiceObject();
            sf::SharedPointer<fssrv::sf::IFileSystem> fs;
            R_TRY(fsp->OpenSdCardFileSystem(std::addressof(fs)));

            /* Allocate a new filesystem wrapper. */
            auto fsa = fs::AllocateShared<impl::FileSystemServiceObjectAdapter>(std::move(fs));
            R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInCodeA());

            *out = std::move(fsa);
            R_SUCCEED();
        }

        class OpenFileOnlyFileSystem : public fsa::IFileSystem, public impl::Newable {
            private:
                virtual Result DoCommit() override final {
                    R_SUCCEED();
                }

                virtual Result DoOpenDirectory(std::unique_ptr<fsa::IDirectory> *out_dir, const fs::Path &path, OpenDirectoryMode mode) override final {
                    AMS_UNUSED(out_dir, path, mode);
                    R_THROW(fs::ResultUnsupportedOperation());
                }

                virtual Result DoGetEntryType(DirectoryEntryType *out, const fs::Path &path) override final {
                    AMS_UNUSED(out, path);
                    R_THROW(fs::ResultUnsupportedOperation());
                }

                virtual Result DoCreateFile(const fs::Path &path, s64 size, int flags) override final {
                    AMS_UNUSED(path, size, flags);
                    R_THROW(fs::ResultUnsupportedOperation());
                }

                virtual Result DoDeleteFile(const fs::Path &path) override final {
                    AMS_UNUSED(path);
                    R_THROW(fs::ResultUnsupportedOperation());
                }

                virtual Result DoCreateDirectory(const fs::Path &path) override final {
                    AMS_UNUSED(path);
                    R_THROW(fs::ResultUnsupportedOperation());
                }

                virtual Result DoDeleteDirectory(const fs::Path &path) override final {
                    AMS_UNUSED(path);
                    R_THROW(fs::ResultUnsupportedOperation());
                }

                virtual Result DoDeleteDirectoryRecursively(const fs::Path &path) override final {
                    AMS_UNUSED(path);
                    R_THROW(fs::ResultUnsupportedOperation());
                }

                virtual Result DoRenameFile(const fs::Path &old_path, const fs::Path &new_path) override final {
                    AMS_UNUSED(old_path, new_path);
                    R_THROW(fs::ResultUnsupportedOperation());
                }

                virtual Result DoRenameDirectory(const fs::Path &old_path, const fs::Path &new_path) override final {
                    AMS_UNUSED(old_path, new_path);
                    R_THROW(fs::ResultUnsupportedOperation());
                }

                virtual Result DoCleanDirectoryRecursively(const fs::Path &path) override final {
                    AMS_UNUSED(path);
                    R_THROW(fs::ResultUnsupportedOperation());
                }

                virtual Result DoGetFreeSpaceSize(s64 *out, const fs::Path &path) override final {
                    AMS_UNUSED(out, path);
                    R_THROW(fs::ResultUnsupportedOperation());
                }

                virtual Result DoGetTotalSpaceSize(s64 *out, const fs::Path &path) override final {
                    AMS_UNUSED(out, path);
                    R_THROW(fs::ResultUnsupportedOperation());
                }

                virtual Result DoCommitProvisionally(s64 counter) override final {
                    AMS_UNUSED(counter);
                    R_THROW(fs::ResultUnsupportedOperation());
                }
        };

        class SdCardRedirectionCodeFileSystem : public OpenFileOnlyFileSystem {
            private:
                util::optional<ReadOnlyFileSystem> m_sd_content_fs;
                ReadOnlyFileSystem m_code_fs;
                bool m_is_redirect;
            public:
                SdCardRedirectionCodeFileSystem(std::unique_ptr<fsa::IFileSystem> &&code, ncm::ProgramId program_id, bool redirect) : m_code_fs(std::move(code)), m_is_redirect(redirect) {
                    if (!cfg::IsSdCardInitialized()) {
                        return;
                    }

                    /* Open an SD card filesystem. */
                    std::shared_ptr<fsa::IFileSystem> sd_fs;
                    if (R_FAILED(OpenSdCardFileSystemImpl(std::addressof(sd_fs)))) {
                        return;
                    }

                    /* Create a redirection filesystem to the relevant content folder. */
                    auto subdir_fs = std::make_unique<fssystem::SubDirectoryFileSystem>(std::move(sd_fs));
                    if (subdir_fs == nullptr) {
                        return;
                    }

                    fs::Path path;
                    R_ABORT_UNLESS(path.InitializeWithFormat("/atmosphere/contents/%016" PRIx64 "/exefs", program_id.value));
                    R_ABORT_UNLESS(path.Normalize(fs::PathFlags{}));

                    R_ABORT_UNLESS(subdir_fs->Initialize(path));

                    m_sd_content_fs.emplace(std::move(subdir_fs));
                }
            private:
                bool IsFileStubbed(const fs::Path &path) {
                    /* If we don't have an sd content fs, nothing is stubbed. */
                    if (!m_sd_content_fs) {
                        return false;
                    }

                    /* Create a path representing the stub. */
                    fs::Path stub_path;
                    if (R_FAILED(stub_path.InitializeWithFormat("%s.stub", path.GetString()))) {
                        return false;
                    }
                    if (R_FAILED(stub_path.Normalize(fs::PathFlags{}))) {
                        return false;
                    }

                    /* Query whether we have the file. */
                    bool has_file;
                    if (R_FAILED(fssystem::HasFile(std::addressof(has_file), std::addressof(*m_sd_content_fs), stub_path))) {
                        return false;
                    }

                    return has_file;
                }

                virtual Result DoOpenFile(std::unique_ptr<fsa::IFile> *out_file, const fs::Path &path, OpenMode mode) override final {
                    /* Only allow opening files with mode = read. */
                    R_UNLESS((mode & fs::OpenMode_All) == fs::OpenMode_Read, fs::ResultInvalidOpenMode());

                    /* If we support redirection, we'd like to prefer a file from the sd card. */
                    if (m_is_redirect) {
                        R_SUCCEED_IF(R_SUCCEEDED(m_sd_content_fs->OpenFile(out_file, path, mode)));
                    }

                    /* Otherwise, check if the file is stubbed. */
                    R_UNLESS(!this->IsFileStubbed(path), fs::ResultPathNotFound());

                    /* Open a file from the base code fs. */
                    R_RETURN(m_code_fs.OpenFile(out_file, path, mode));
                }
        };

        class AtmosphereCodeFileSystem : public OpenFileOnlyFileSystem {
            private:
                util::optional<SdCardRedirectionCodeFileSystem> m_code_fs;
                util::optional<ReadOnlyFileSystem> m_hbl_fs;
                ncm::ProgramId m_program_id;
                ncm::StorageId m_storage_id;
                bool m_initialized;
            public:
                AtmosphereCodeFileSystem() : m_initialized(false) { /* ... */ }

                Result Initialize(CodeVerificationData *out_verification_data, const char *path, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id, bool is_hbl, bool is_specific) {
                    AMS_ABORT_UNLESS(!m_initialized);

                    /* If we're hbl, we need to open a hbl fs. */
                    if (is_hbl) {
                        std::unique_ptr<fsa::IFileSystem> fsa;
                        R_TRY(OpenHblCodeFileSystemImpl(std::addressof(fsa)));
                        m_hbl_fs.emplace(std::move(fsa));
                    }

                    /* Open the code filesystem. */
                    std::unique_ptr<fsa::IFileSystem> fsa;
                    R_TRY(OpenSdCardCodeOrStratosphereCodeOrCodeFileSystemImpl(out_verification_data, std::addressof(fsa), path, attr, program_id, storage_id));
                    m_code_fs.emplace(std::move(fsa), program_id, is_specific);

                    m_program_id  = program_id;
                    m_storage_id  = storage_id;
                    m_initialized = true;

                    R_SUCCEED();
                }
            private:
                virtual Result DoOpenFile(std::unique_ptr<fsa::IFile> *out_file, const fs::Path &path, OpenMode mode) override final {
                    /* Ensure that we're initialized. */
                    R_UNLESS(m_initialized, fs::ResultNotInitialized());

                    /* Only allow opening files with mode = read. */
                    R_UNLESS((mode & fs::OpenMode_All) == fs::OpenMode_Read, fs::ResultInvalidOpenMode());

                    /* First, check if there's an external code. */
                    {
                        fsa::IFileSystem *ecs = fssystem::GetExternalCodeFileSystem(m_program_id);
                        if (ecs != nullptr) {
                            R_RETURN(ecs->OpenFile(out_file, path, mode));
                        }
                    }

                    /* If we're hbl, open from the hbl fs. */
                    if (m_hbl_fs) {
                        R_RETURN(m_hbl_fs->OpenFile(out_file, path, mode));
                    }

                    /* If we're not hbl, fall back to our code filesystem. */
                    R_RETURN(m_code_fs->OpenFile(out_file, path, mode));
                }
        };

    }

    Result MountCode(CodeVerificationData *out, const char *name, const char *path, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id) {
        auto mount_impl = [=]() -> Result {
            /* Clear the output. */
            std::memset(out, 0, sizeof(*out));

            /* Validate the mount name. */
            R_TRY(impl::CheckMountName(name));

            /* Validate the path isn't null. */
            R_UNLESS(path != nullptr, fs::ResultInvalidPath());

            /* Open the code file system. */
            std::unique_ptr<fsa::IFileSystem> fsa;
            R_TRY(OpenCodeFileSystemImpl(out, std::addressof(fsa), path, attr, program_id, storage_id));

            /* Register. */
            R_RETURN(fsa::Register(name, std::move(fsa)));
        };

        /* Perform the mount. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(mount_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CODE(name, path, program_id)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

    Result MountCodeForAtmosphereWithRedirection(CodeVerificationData *out, const char *name, const char *path, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id, bool is_hbl, bool is_specific) {
        auto mount_impl = [=]() -> Result {
            /* Clear the output. */
            std::memset(out, 0, sizeof(*out));

            /* Validate the mount name. */
            R_TRY(impl::CheckMountName(name));

            /* Validate the path isn't null. */
            R_UNLESS(path != nullptr, fs::ResultInvalidPath());

            /* Create an AtmosphereCodeFileSystem. */
            auto ams_code_fs = std::make_unique<AtmosphereCodeFileSystem>();
            R_UNLESS(ams_code_fs != nullptr, fs::ResultAllocationMemoryFailedInCodeA());

            /* Initialize the code file system. */
            R_TRY(ams_code_fs->Initialize(out, path, attr, program_id, storage_id, is_hbl, is_specific));

            /* Register. */
            R_RETURN(fsa::Register(name, std::move(ams_code_fs)));
        };

        /* Perform the mount. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(mount_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CODE(name, path, program_id)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

    Result MountCodeForAtmosphere(CodeVerificationData *out, const char *name, const char *path, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id) {
        auto mount_impl = [=]() -> Result {
            /* Clear the output. */
            std::memset(out, 0, sizeof(*out));

            /* Validate the mount name. */
            R_TRY(impl::CheckMountName(name));

            /* Validate the path isn't null. */
            R_UNLESS(path != nullptr, fs::ResultInvalidPath());

            /* Open the code file system. */
            std::unique_ptr<fsa::IFileSystem> fsa;
            R_TRY(OpenSdCardCodeOrStratosphereCodeOrCodeFileSystemImpl(out, std::addressof(fsa), path, attr, program_id, storage_id));

            /* Create a wrapper fs. */
            auto wrap_fsa = std::make_unique<SdCardRedirectionCodeFileSystem>(std::move(fsa), program_id, false);
            R_UNLESS(wrap_fsa != nullptr, fs::ResultAllocationMemoryFailedInCodeA());

            /* Register. */
            R_RETURN(fsa::Register(name, std::move(wrap_fsa)));
        };

        /* Perform the mount. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(mount_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CODE(name, path, program_id)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

}
