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

            return ResultSuccess();
        }

        fsa::IFileSystem &GetStratosphereRomFsFileSystem() {
            /* Ensure that stratosphere.romfs is mounted. */
            /* NOTE: Abort is used here to ensure that atmosphere's filesystem is structurally valid. */
            R_ABORT_UNLESS(EnsureStratosphereRomfsMounted());

            return GetReference(g_stratosphere_romfs_fs);
        }

        Result OpenCodeFileSystemImpl(CodeVerificationData *out_verification_data, std::unique_ptr<fsa::IFileSystem> *out, const char *path, ncm::ProgramId program_id) {
            /* Print a path suitable for the remote service. */
            fssrv::sf::Path sf_path;
            R_TRY(FspPathPrintf(std::addressof(sf_path), "%s", path));

            /* Open the filesystem using libnx bindings. */
            static_assert(sizeof(CodeVerificationData) == sizeof(::FsCodeInfo));
            ::FsFileSystem fs;
            R_TRY(fsldrOpenCodeFileSystem(reinterpret_cast<::FsCodeInfo *>(out_verification_data), program_id.value, sf_path.str, std::addressof(fs)));

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<RemoteFileSystem>(fs);
            R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInCodeA());

            *out = std::move(fsa);
            return ResultSuccess();
        }

        Result OpenPackageFileSystemImpl(std::unique_ptr<fsa::IFileSystem> *out, const char *common_path) {
            /* Open a filesystem using libnx bindings. */
            FsFileSystem fs;
            R_TRY(fsOpenFileSystemWithId(std::addressof(fs), ncm::InvalidProgramId.value, static_cast<::FsFileSystemType>(impl::FileSystemProxyType_Package), common_path));

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<RemoteFileSystem>(fs);
            R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInCodeA());

            *out = std::move(fsa);
            return ResultSuccess();
        }

        Result OpenSdCardCodeFileSystemImpl(std::unique_ptr<fsa::IFileSystem> *out, ncm::ProgramId program_id) {
            /* Ensure we don't access the SD card too early. */
            R_UNLESS(cfg::IsSdCardInitialized(), fs::ResultSdCardNotPresent());

            /* Print a path to the program's package. */
            fssrv::sf::Path sf_path;
            R_TRY(FspPathPrintf(std::addressof(sf_path), "%s:/atmosphere/contents/%016lx/exefs.nsp", impl::SdCardFileSystemMountName, program_id.value));

            return OpenPackageFileSystemImpl(out, sf_path.str);
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
                fssrv::sf::Path sf_path;
                R_TRY(FspPathPrintf(std::addressof(sf_path), "/atmosphere/contents/%016lX/exefs.nsp", program_id.value));

                /* Open the package within stratosphere.romfs. */
                R_TRY(romfs_fs.OpenFile(std::addressof(package_file), sf_path.str, fs::OpenMode_Read));
            }

            /* Create a file storage for the program's package. */
            auto package_storage = std::make_shared<FileStorage>(std::move(package_file));
            R_UNLESS(package_storage != nullptr, fs::ResultAllocationFailureInCodeA());

            /* Create a partition filesystem. */
            auto package_fs = std::make_unique<fssystem::PartitionFileSystem>();
            R_UNLESS(package_fs != nullptr, fs::ResultAllocationFailureInCodeA());

            /* Initialize the partition filesystem. */
            R_TRY(package_fs->Initialize(package_storage));

            *out = std::move(package_fs);
            return ResultSuccess();
        }

        Result OpenSdCardCodeOrStratosphereCodeOrCodeFileSystemImpl(CodeVerificationData *out_verification_data, std::unique_ptr<fsa::IFileSystem> *out, const char *path, ncm::ProgramId program_id) {
            /* If we can open an sd card code fs, use it. */
            R_SUCCEED_IF(R_SUCCEEDED(OpenSdCardCodeFileSystemImpl(out, program_id)));

            /* If we can open a stratosphere code fs, use it. */
            R_SUCCEED_IF(R_SUCCEEDED(OpenStratosphereCodeFileSystemImpl(out, program_id)));

            /* Otherwise, fall back to a normal code fs. */
            return OpenCodeFileSystemImpl(out_verification_data, out, path, program_id);
        }

        Result OpenHblCodeFileSystemImpl(std::unique_ptr<fsa::IFileSystem> *out) {
            /* Get the HBL path. */
            const char *hbl_path = cfg::GetHblPath();

            /* Print a path to the hbl package. */
            fssrv::sf::Path sf_path;
            R_TRY(FspPathPrintf(std::addressof(sf_path), "%s:/%s", impl::SdCardFileSystemMountName, hbl_path[0] == '/' ? hbl_path + 1 : hbl_path));

            return OpenPackageFileSystemImpl(out, sf_path.str);
        }

        Result OpenSdCardFileSystemImpl(std::unique_ptr<fsa::IFileSystem> *out) {
            /* Open the SD card. This uses libnx bindings. */
            FsFileSystem fs;
            R_TRY(fsOpenSdCardFileSystem(std::addressof(fs)));

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<RemoteFileSystem>(fs);
            R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInCodeA());

            *out = std::move(fsa);
            return ResultSuccess();
        }

        class OpenFileOnlyFileSystem : public fsa::IFileSystem, public impl::Newable {
            private:
                virtual Result DoCommit() override final {
                    return ResultSuccess();
                }

                virtual Result DoOpenDirectory(std::unique_ptr<fsa::IDirectory> *out_dir, const char *path, OpenDirectoryMode mode) override final {
                    AMS_UNUSED(out_dir, path, mode);
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result DoGetEntryType(DirectoryEntryType *out, const char *path) override final {
                    AMS_UNUSED(out, path);
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result DoCreateFile(const char *path, s64 size, int flags) override final {
                    AMS_UNUSED(path, size, flags);
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result DoDeleteFile(const char *path) override final {
                    AMS_UNUSED(path);
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result DoCreateDirectory(const char *path) override final {
                    AMS_UNUSED(path);
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result DoDeleteDirectory(const char *path) override final {
                    AMS_UNUSED(path);
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result DoDeleteDirectoryRecursively(const char *path) override final {
                    AMS_UNUSED(path);
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result DoRenameFile(const char *old_path, const char *new_path) override final {
                    AMS_UNUSED(old_path, new_path);
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result DoRenameDirectory(const char *old_path, const char *new_path) override final {
                    AMS_UNUSED(old_path, new_path);
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result DoCleanDirectoryRecursively(const char *path) override final {
                    AMS_UNUSED(path);
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result DoGetFreeSpaceSize(s64 *out, const char *path) override final {
                    AMS_UNUSED(out, path);
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result DoGetTotalSpaceSize(s64 *out, const char *path) override final {
                    AMS_UNUSED(out, path);
                    return fs::ResultUnsupportedOperation();
                }

                virtual Result DoCommitProvisionally(s64 counter) override final {
                    AMS_UNUSED(counter);
                    return fs::ResultUnsupportedOperation();
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
                    std::unique_ptr<fsa::IFileSystem> sd_fs;
                    if (R_FAILED(OpenSdCardFileSystemImpl(std::addressof(sd_fs)))) {
                        return;
                    }

                    /* Create a redirection filesystem to the relevant content folder. */
                    char path[fs::EntryNameLengthMax + 1];
                    util::SNPrintf(path, sizeof(path), "/atmosphere/contents/%016lx/exefs", program_id.value);

                    auto subdir_fs = std::make_unique<fssystem::SubDirectoryFileSystem>(std::move(sd_fs), path);
                    if (subdir_fs == nullptr) {
                        return;
                    }

                    m_sd_content_fs.emplace(std::move(subdir_fs));
                }
            private:
                bool IsFileStubbed(const char *path) {
                    /* If we don't have an sd content fs, nothing is stubbed. */
                    if (!m_sd_content_fs) {
                        return false;
                    }

                    /* Create a path representing the stub. */
                    char stub_path[fs::EntryNameLengthMax + 1];
                    util::SNPrintf(stub_path, sizeof(stub_path), "%s.stub", path);

                    /* Query whether we have the file. */
                    bool has_file;
                    if (R_FAILED(fssystem::HasFile(std::addressof(has_file), std::addressof(*m_sd_content_fs), stub_path))) {
                        return false;
                    }

                    return has_file;
                }

                virtual Result DoOpenFile(std::unique_ptr<fsa::IFile> *out_file, const char *path, OpenMode mode) override final {
                    /* Only allow opening files with mode = read. */
                    R_UNLESS((mode & fs::OpenMode_All) == fs::OpenMode_Read, fs::ResultInvalidOpenMode());

                    /* If we support redirection, we'd like to prefer a file from the sd card. */
                    if (m_is_redirect) {
                        R_SUCCEED_IF(R_SUCCEEDED(m_sd_content_fs->OpenFile(out_file, path, mode)));
                    }

                    /* Otherwise, check if the file is stubbed. */
                    R_UNLESS(!this->IsFileStubbed(path), fs::ResultPathNotFound());

                    /* Open a file from the base code fs. */
                    return m_code_fs.OpenFile(out_file, path, mode);
                }
        };

        class AtmosphereCodeFileSystem : public OpenFileOnlyFileSystem {
            private:
                util::optional<SdCardRedirectionCodeFileSystem> m_code_fs;
                util::optional<ReadOnlyFileSystem> m_hbl_fs;
                ncm::ProgramId m_program_id;
                bool m_initialized;
            public:
                AtmosphereCodeFileSystem() : m_initialized(false) { /* ... */ }

                Result Initialize(CodeVerificationData *out_verification_data, const char *path, ncm::ProgramId program_id, bool is_hbl, bool is_specific) {
                    AMS_ABORT_UNLESS(!m_initialized);

                    /* If we're hbl, we need to open a hbl fs. */
                    if (is_hbl) {
                        std::unique_ptr<fsa::IFileSystem> fsa;
                        R_TRY(OpenHblCodeFileSystemImpl(std::addressof(fsa)));
                        m_hbl_fs.emplace(std::move(fsa));
                    }

                    /* Open the code filesystem. */
                    std::unique_ptr<fsa::IFileSystem> fsa;
                    R_TRY(OpenSdCardCodeOrStratosphereCodeOrCodeFileSystemImpl(out_verification_data, std::addressof(fsa), path, program_id));
                    m_code_fs.emplace(std::move(fsa), program_id, is_specific);

                    m_program_id = program_id;
                    m_initialized = true;

                    return ResultSuccess();
                }
            private:
                virtual Result DoOpenFile(std::unique_ptr<fsa::IFile> *out_file, const char *path, OpenMode mode) override final {
                    /* Ensure that we're initialized. */
                    R_UNLESS(m_initialized, fs::ResultNotInitialized());

                    /* Only allow opening files with mode = read. */
                    R_UNLESS((mode & fs::OpenMode_All) == fs::OpenMode_Read, fs::ResultInvalidOpenMode());

                    /* First, check if there's an external code. */
                    {
                        fsa::IFileSystem *ecs = fssystem::GetExternalCodeFileSystem(m_program_id);
                        if (ecs != nullptr) {
                            return ecs->OpenFile(out_file, path, mode);
                        }
                    }

                    /* If we're hbl, open from the hbl fs. */
                    if (m_hbl_fs) {
                        return m_hbl_fs->OpenFile(out_file, path, mode);
                    }

                    /* If we're not hbl, fall back to our code filesystem. */
                    return m_code_fs->OpenFile(out_file, path, mode);
                }
        };

    }

    Result MountCode(CodeVerificationData *out, const char *name, const char *path, ncm::ProgramId program_id) {
        /* Clear the output. */
        std::memset(out, 0, sizeof(*out));

        /* Validate the mount name. */
        R_TRY(impl::CheckMountName(name));

        /* Validate the path isn't null. */
        R_UNLESS(path != nullptr, fs::ResultInvalidPath());

        /* Open the code file system. */
        std::unique_ptr<fsa::IFileSystem> fsa;
        R_TRY(OpenCodeFileSystemImpl(out, std::addressof(fsa), path, program_id));

        /* Register. */
        return fsa::Register(name, std::move(fsa));
    }

    Result MountCodeForAtmosphereWithRedirection(CodeVerificationData *out, const char *name, const char *path, ncm::ProgramId program_id, bool is_hbl, bool is_specific) {
        /* Clear the output. */
        std::memset(out, 0, sizeof(*out));

        /* Validate the mount name. */
        R_TRY(impl::CheckMountName(name));

        /* Validate the path isn't null. */
        R_UNLESS(path != nullptr, fs::ResultInvalidPath());

        /* Create an AtmosphereCodeFileSystem. */
        auto ams_code_fs = std::make_unique<AtmosphereCodeFileSystem>();
        R_UNLESS(ams_code_fs != nullptr, fs::ResultAllocationFailureInCodeA());

        /* Initialize the code file system. */
        R_TRY(ams_code_fs->Initialize(out, path, program_id, is_hbl, is_specific));

        /* Register. */
        return fsa::Register(name, std::move(ams_code_fs));
    }

    Result MountCodeForAtmosphere(CodeVerificationData *out, const char *name, const char *path, ncm::ProgramId program_id) {
        /* Clear the output. */
        std::memset(out, 0, sizeof(*out));

        /* Validate the mount name. */
        R_TRY(impl::CheckMountName(name));

        /* Validate the path isn't null. */
        R_UNLESS(path != nullptr, fs::ResultInvalidPath());

        /* Open the code file system. */
        std::unique_ptr<fsa::IFileSystem> fsa;
        R_TRY(OpenSdCardCodeOrStratosphereCodeOrCodeFileSystemImpl(out, std::addressof(fsa), path, program_id));

        /* Create a wrapper fs. */
        auto wrap_fsa = std::make_unique<SdCardRedirectionCodeFileSystem>(std::move(fsa), program_id, false);
        R_UNLESS(wrap_fsa != nullptr, fs::ResultAllocationFailureInCodeA());

        /* Register. */
        return fsa::Register(name, std::move(wrap_fsa));
    }

}
