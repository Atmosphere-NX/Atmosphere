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

namespace ams::fssystem {

    namespace {

        constexpr size_t IdealWorkBufferSize   = 1_MB;
        constexpr size_t MinimumWorkBufferSize = 1_KB;

        constexpr const fs::Path CommittedDirectoryPath     = fs::MakeConstantPath("/0");
        constexpr const fs::Path WorkingDirectoryPath       = fs::MakeConstantPath("/1");
        constexpr const fs::Path SynchronizingDirectoryPath = fs::MakeConstantPath("/_");
        constexpr const fs::Path LockFilePath               = fs::MakeConstantPath("/.lock");

        class DirectorySaveDataFile : public fs::fsa::IFile, public fs::impl::Newable {
            private:
                std::unique_ptr<fs::fsa::IFile> m_base_file;
                DirectorySaveDataFileSystem *m_parent_fs;
                fs::OpenMode m_open_mode;
            public:
                DirectorySaveDataFile(std::unique_ptr<fs::fsa::IFile> f, DirectorySaveDataFileSystem *p, fs::OpenMode m) : m_base_file(std::move(f)), m_parent_fs(p), m_open_mode(m) {
                    /* ... */
                }

                virtual ~DirectorySaveDataFile() {
                    /* Observe closing of writable file. */
                    if (m_open_mode & fs::OpenMode_Write) {
                        m_parent_fs->DecrementWriteOpenFileCount();
                    }
                }
            public:
                virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override {
                    R_RETURN(m_base_file->Read(out, offset, buffer, size, option));
                }

                virtual Result DoGetSize(s64 *out) override {
                    R_RETURN(m_base_file->GetSize(out));
                }

                virtual Result DoFlush() override {
                    R_RETURN(m_base_file->Flush());
                }

                virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override {
                    R_RETURN(m_base_file->Write(offset, buffer, size, option));
                }

                virtual Result DoSetSize(s64 size) override {
                    R_RETURN(m_base_file->SetSize(size));
                }

                virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                    R_RETURN(m_base_file->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                }
            public:
                virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                    return m_base_file->GetDomainObjectId();
                }
        };

    }

    Result DirectorySaveDataFileSystem::Initialize(bool journaling_supported, bool multi_commit_supported, bool journaling_enabled) {
        /* Configure ourselves. */
        m_is_journaling_supported   = journaling_supported;
        m_is_multi_commit_supported = multi_commit_supported;
        m_is_journaling_enabled     = journaling_enabled;

        /* Ensure that we can initialize further by acquiring a lock on the filesystem. */
        R_TRY(this->AcquireLockFile());

        fs::DirectoryEntryType type;

        /* Check that the working directory exists. */
        R_TRY_CATCH(m_base_fs->GetEntryType(std::addressof(type), WorkingDirectoryPath)) {
            /* If path isn't found, create working directory and committed directory. */
            R_CATCH(fs::ResultPathNotFound) {
                R_TRY(m_base_fs->CreateDirectory(WorkingDirectoryPath));

                if (m_is_journaling_supported) {
                    R_TRY(m_base_fs->CreateDirectory(CommittedDirectoryPath));
                }
            }
        } R_END_TRY_CATCH;

        /* If we support journaling, we need to set up the committed directory. */
        if (m_is_journaling_supported) {
            /* Now check for the committed directory. */
            R_TRY_CATCH(m_base_fs->GetEntryType(std::addressof(type), CommittedDirectoryPath)) {
                /* Committed doesn't exist, so synchronize and rename. */
                R_CATCH(fs::ResultPathNotFound) {
                    R_TRY(this->SynchronizeDirectory(SynchronizingDirectoryPath, WorkingDirectoryPath));
                    R_TRY(m_base_fs->RenameDirectory(SynchronizingDirectoryPath, CommittedDirectoryPath));
                    R_SUCCEED();
                }
            } R_END_TRY_CATCH;

            /* The committed directory exists, so if we should, synchronize it to the working directory. */
            if (m_is_journaling_enabled) {
                R_TRY(this->SynchronizeDirectory(WorkingDirectoryPath, CommittedDirectoryPath));
            }
        }

        R_SUCCEED();
    }

    Result DirectorySaveDataFileSystem::SynchronizeDirectory(const fs::Path &dst, const fs::Path &src) {
        /* Delete destination dir and recreate it. */
        R_TRY_CATCH(m_base_fs->DeleteDirectoryRecursively(dst)) {
            R_CATCH(fs::ResultPathNotFound) { /* Nintendo returns error unconditionally, but I think that's a bug in their code. */}
        } R_END_TRY_CATCH;

        R_TRY(m_base_fs->CreateDirectory(dst));

        /* Get a work buffer to work with. */
        fssystem::PooledBuffer buffer;
        buffer.AllocateParticularlyLarge(IdealWorkBufferSize, MinimumWorkBufferSize);

        /* Copy the directory recursively. */
        fs::DirectoryEntry dir_entry_buffer = {};
        R_RETURN(fssystem::CopyDirectoryRecursively(m_base_fs, dst, src, std::addressof(dir_entry_buffer), buffer.GetBuffer(), buffer.GetSize()));
    }

    Result DirectorySaveDataFileSystem::ResolvePath(fs::Path *out, const fs::Path &path) {
        const fs::Path &directory = (m_is_journaling_supported && !m_is_journaling_enabled) ? CommittedDirectoryPath : WorkingDirectoryPath;
        R_RETURN(out->Combine(directory, path));
    }

    Result DirectorySaveDataFileSystem::AcquireLockFile() {
        /* If we already have a lock file, we don't need to lock again. */
        R_SUCCEED_IF(m_lock_file != nullptr);

        /* Open the lock file. */
        std::unique_ptr<fs::fsa::IFile> file;
        R_TRY_CATCH(m_base_fs->OpenFile(std::addressof(file), LockFilePath, fs::OpenMode_ReadWrite)) {
            /* If the lock file doesn't yet exist, we may need to create it. */
            R_CATCH(fs::ResultPathNotFound) {
                R_TRY(m_base_fs->CreateFile(LockFilePath, 0));
                R_TRY(m_base_fs->OpenFile(std::addressof(file), LockFilePath, fs::OpenMode_ReadWrite));
            }
        } R_END_TRY_CATCH;

        /* Set our lock file. */
        m_lock_file = std::move(file);
        R_SUCCEED();
    }

    void DirectorySaveDataFileSystem::DecrementWriteOpenFileCount() {
        std::scoped_lock lk(m_accessor_mutex);

        --m_open_writable_files;
    }

    Result DirectorySaveDataFileSystem::DoCreateFile(const fs::Path &path, s64 size, int option) {
        /* Resolve the final path. */
        fs::Path resolved;
        R_TRY(this->ResolvePath(std::addressof(resolved), path));

        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        R_RETURN(m_base_fs->CreateFile(resolved, size, option));
    }
    Result DirectorySaveDataFileSystem::DoDeleteFile(const fs::Path &path) {
        /* Resolve the final path. */
        fs::Path resolved;
        R_TRY(this->ResolvePath(std::addressof(resolved), path));

        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        R_RETURN(m_base_fs->DeleteFile(resolved));
    }

    Result DirectorySaveDataFileSystem::DoCreateDirectory(const fs::Path &path) {
        /* Resolve the final path. */
        fs::Path resolved;
        R_TRY(this->ResolvePath(std::addressof(resolved), path));

        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        R_RETURN(m_base_fs->CreateDirectory(resolved));
    }

    Result DirectorySaveDataFileSystem::DoDeleteDirectory(const fs::Path &path) {
        /* Resolve the final path. */
        fs::Path resolved;
        R_TRY(this->ResolvePath(std::addressof(resolved), path));

        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        R_RETURN(m_base_fs->DeleteDirectory(resolved));
    }

    Result DirectorySaveDataFileSystem::DoDeleteDirectoryRecursively(const fs::Path &path) {
        /* Resolve the final path. */
        fs::Path resolved;
        R_TRY(this->ResolvePath(std::addressof(resolved), path));

        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        R_RETURN(m_base_fs->DeleteDirectoryRecursively(resolved));
    }

    Result DirectorySaveDataFileSystem::DoRenameFile(const fs::Path &old_path, const fs::Path &new_path) {
        /* Resolve the final paths. */
        fs::Path old_resolved;
        fs::Path new_resolved;
        R_TRY(this->ResolvePath(std::addressof(old_resolved), old_path));
        R_TRY(this->ResolvePath(std::addressof(new_resolved), new_path));

        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        R_RETURN(m_base_fs->RenameFile(old_resolved, new_resolved));
    }

    Result DirectorySaveDataFileSystem::DoRenameDirectory(const fs::Path &old_path, const fs::Path &new_path) {
        /* Resolve the final paths. */
        fs::Path old_resolved;
        fs::Path new_resolved;
        R_TRY(this->ResolvePath(std::addressof(old_resolved), old_path));
        R_TRY(this->ResolvePath(std::addressof(new_resolved), new_path));

        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        R_RETURN(m_base_fs->RenameDirectory(old_resolved, new_resolved));
    }

    Result DirectorySaveDataFileSystem::DoGetEntryType(fs::DirectoryEntryType *out, const fs::Path &path) {
        /* Resolve the final path. */
        fs::Path resolved;
        R_TRY(this->ResolvePath(std::addressof(resolved), path));

        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        R_RETURN(m_base_fs->GetEntryType(out, resolved));
    }

    Result DirectorySaveDataFileSystem::DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const fs::Path &path, fs::OpenMode mode) {
        /* Resolve the final path. */
        fs::Path resolved;
        R_TRY(this->ResolvePath(std::addressof(resolved), path));

        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        /* Open base file. */
        std::unique_ptr<fs::fsa::IFile> base_file;
        R_TRY(m_base_fs->OpenFile(std::addressof(base_file), resolved, mode));

        /* Make DirectorySaveDataFile. */
        std::unique_ptr<fs::fsa::IFile> file = std::make_unique<DirectorySaveDataFile>(std::move(base_file), this, mode);
        R_UNLESS(file != nullptr, fs::ResultAllocationMemoryFailedInDirectorySaveDataFileSystemA());

        /* Increment our open writable files, if the file is writable. */
        if (mode & fs::OpenMode_Write) {
            ++m_open_writable_files;
        }

        /* Set the output. */
        *out_file = std::move(file);
        R_SUCCEED();
    }

    Result DirectorySaveDataFileSystem::DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const fs::Path &path, fs::OpenDirectoryMode mode) {
        /* Resolve the final path. */
        fs::Path resolved;
        R_TRY(this->ResolvePath(std::addressof(resolved), path));

        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        R_RETURN(m_base_fs->OpenDirectory(out_dir, resolved, mode));
    }

    Result DirectorySaveDataFileSystem::DoCommit() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        /* If we aren't journaling, we don't need to do anything. */
        R_SUCCEED_IF(!m_is_journaling_enabled);
        R_SUCCEED_IF(!m_is_journaling_supported);

        /* Check that there are no open files blocking the commit. */
        R_UNLESS(m_open_writable_files == 0, fs::ResultWriteModeFileNotClosed());

        /* Remove the previous commit by renaming the folder. */
        R_TRY(fssystem::RetryFinitelyForTargetLocked([&] () ALWAYS_INLINE_LAMBDA { R_RETURN(m_base_fs->RenameDirectory(CommittedDirectoryPath, SynchronizingDirectoryPath)); }));

        /* Synchronize the working directory to the synchronizing directory. */
        R_TRY(fssystem::RetryFinitelyForTargetLocked([&] () ALWAYS_INLINE_LAMBDA { R_RETURN(this->SynchronizeDirectory(SynchronizingDirectoryPath, WorkingDirectoryPath)); }));

        /* Rename the synchronized directory to commit it. */
        R_TRY(fssystem::RetryFinitelyForTargetLocked([&] () ALWAYS_INLINE_LAMBDA { R_RETURN(m_base_fs->RenameDirectory(SynchronizingDirectoryPath, CommittedDirectoryPath)); }));

        R_SUCCEED();
    }

    Result DirectorySaveDataFileSystem::DoGetFreeSpaceSize(s64 *out, const fs::Path &path) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        /* Get the free space size in our working directory. */
        AMS_UNUSED(path);
        R_RETURN(m_base_fs->GetFreeSpaceSize(out, WorkingDirectoryPath));
    }

    Result DirectorySaveDataFileSystem::DoGetTotalSpaceSize(s64 *out, const fs::Path &path) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        /* Get the free space size in our working directory. */
        AMS_UNUSED(path);
        R_RETURN(m_base_fs->GetTotalSpaceSize(out, WorkingDirectoryPath));
    }

    Result DirectorySaveDataFileSystem::DoCleanDirectoryRecursively(const fs::Path &path) {
        /* Resolve the final path. */
        fs::Path resolved;
        R_TRY(this->ResolvePath(std::addressof(resolved), path));

        /* Lock ourselves. */
        std::scoped_lock lk(m_accessor_mutex);

        R_RETURN(m_base_fs->CleanDirectoryRecursively(resolved));
    }

    Result DirectorySaveDataFileSystem::DoCommitProvisionally(s64 counter) {
        /* Check that we support multi-commit. */
        R_UNLESS(m_is_multi_commit_supported, fs::ResultUnsupportedCommitProvisionallyForDirectorySaveDataFileSystem());

        /* Do nothing. */
        AMS_UNUSED(counter);
        R_SUCCEED();
    }

    Result DirectorySaveDataFileSystem::DoRollback() {
        /* On non-journaled savedata, there's nothing to roll back to. */
        R_SUCCEED_IF(!m_is_journaling_supported);

        /* Perform a re-initialize. */
        R_RETURN(this->Initialize(m_is_journaling_supported, m_is_multi_commit_supported, m_is_journaling_enabled));
    }

}
