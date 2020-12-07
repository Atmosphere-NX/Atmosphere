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

namespace ams::fssystem {

    namespace {

        constexpr size_t IdealWorkBufferSize = 0x100000; /* 1 MiB */

        constexpr const char CommittedDirectoryPath[]     = "/0/";
        constexpr const char WorkingDirectoryPath[]       = "/1/";
        constexpr const char SynchronizingDirectoryPath[] = "/_/";

        class DirectorySaveDataFile : public fs::fsa::IFile {
            private:
                std::unique_ptr<fs::fsa::IFile> base_file;
                DirectorySaveDataFileSystem *parent_fs;
                fs::OpenMode open_mode;
            public:
                DirectorySaveDataFile(std::unique_ptr<fs::fsa::IFile> f, DirectorySaveDataFileSystem *p, fs::OpenMode m) : base_file(std::move(f)), parent_fs(p), open_mode(m) {
                    /* ... */
                }

                virtual ~DirectorySaveDataFile() {
                    /* Observe closing of writable file. */
                    if (this->open_mode & fs::OpenMode_Write) {
                        this->parent_fs->OnWritableFileClose();
                    }
                }
            public:
                virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override {
                    return this->base_file->Read(out, offset, buffer, size, option);
                }

                virtual Result DoGetSize(s64 *out) override {
                    return this->base_file->GetSize(out);
                }

                virtual Result DoFlush() override {
                    return this->base_file->Flush();
                }

                virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override {
                    return this->base_file->Write(offset, buffer, size, option);
                }

                virtual Result DoSetSize(s64 size) override {
                    return this->base_file->SetSize(size);
                }

                virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                    return this->base_file->OperateRange(dst, dst_size, op_id, offset, size, src, src_size);
                }
            public:
                virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                    return this->base_file->GetDomainObjectId();
                }
        };

    }

    DirectorySaveDataFileSystem::DirectorySaveDataFileSystem(std::shared_ptr<fs::fsa::IFileSystem> fs)
        : PathResolutionFileSystem(fs), accessor_mutex(false), open_writable_files(0)
    {
        /* ... */
    }

    DirectorySaveDataFileSystem::DirectorySaveDataFileSystem(std::unique_ptr<fs::fsa::IFileSystem> fs)
        : PathResolutionFileSystem(std::move(fs)), accessor_mutex(false), open_writable_files(0)
    {
        /* ... */
    }

    DirectorySaveDataFileSystem::~DirectorySaveDataFileSystem() {
        /* ... */
    }

    Result DirectorySaveDataFileSystem::Initialize() {
        /* Nintendo does not acquire the lock here, but I think we probably should. */
        std::scoped_lock lk(this->accessor_mutex);

        fs::DirectoryEntryType type;

        /* Check that the working directory exists. */
        R_TRY_CATCH(this->base_fs->GetEntryType(&type, WorkingDirectoryPath)) {
            /* If path isn't found, create working directory and committed directory. */
            R_CATCH(fs::ResultPathNotFound) {
                R_TRY(this->base_fs->CreateDirectory(WorkingDirectoryPath));
                R_TRY(this->base_fs->CreateDirectory(CommittedDirectoryPath));
            }
        } R_END_TRY_CATCH;

        /* Now check for the committed directory. */
        R_TRY_CATCH(this->base_fs->GetEntryType(&type, CommittedDirectoryPath)) {
            /* Committed doesn't exist, so synchronize and rename. */
            R_CATCH(fs::ResultPathNotFound) {
                R_TRY(this->SynchronizeDirectory(SynchronizingDirectoryPath, WorkingDirectoryPath));
                R_TRY(this->base_fs->RenameDirectory(SynchronizingDirectoryPath, CommittedDirectoryPath));
                return ResultSuccess();
            }
        } R_END_TRY_CATCH;

        /* The committed directory exists, so synchronize it to the working directory. */
        return this->SynchronizeDirectory(WorkingDirectoryPath, CommittedDirectoryPath);
    }

    Result DirectorySaveDataFileSystem::AllocateWorkBuffer(std::unique_ptr<u8[]> *out, size_t *out_size, size_t size) {
        /* Repeatedly try to allocate until success. */
        while (size > 0x200) {
            /* Allocate the buffer. */
            if (auto mem = new (std::nothrow) u8[size]; mem != nullptr) {
                out->reset(mem);
                *out_size = size;
                return ResultSuccess();
            } else {
                /* Divide size by two. */
                size >>= 1;
            }
        }

        /* TODO: Return a result here? Nintendo does not, but they have other allocation failed results. */
        /* Consider returning ResultFsAllocationFailureInDirectorySaveDataFileSystem? */
        AMS_ABORT_UNLESS(false);
    }

    Result DirectorySaveDataFileSystem::SynchronizeDirectory(const char *dst, const char *src) {
        /* Delete destination dir and recreate it. */
        R_TRY_CATCH(this->base_fs->DeleteDirectoryRecursively(dst)) {
            R_CATCH(fs::ResultPathNotFound) { /* Nintendo returns error unconditionally, but I think that's a bug in their code. */}
        } R_END_TRY_CATCH;

        R_TRY(this->base_fs->CreateDirectory(dst));

        /* Get a work buffer to work with. */
        std::unique_ptr<u8[]> work_buf;
        size_t work_buf_size;
        R_TRY(this->AllocateWorkBuffer(&work_buf, &work_buf_size, IdealWorkBufferSize));

        /* Copy the directory recursively. */
        return fssystem::CopyDirectoryRecursively(this->base_fs, dst, src, work_buf.get(), work_buf_size);
    }

    Result DirectorySaveDataFileSystem::ResolveFullPath(char *out, size_t out_size, const char *relative_path) {
        R_UNLESS(strnlen(relative_path, fs::EntryNameLengthMax + 1) < fs::EntryNameLengthMax + 1, fs::ResultTooLongPath());
        R_UNLESS(fs::PathNormalizer::IsSeparator(relative_path[0]), fs::ResultInvalidPath());

        /* Copy working directory path. */
        std::strncpy(out, WorkingDirectoryPath, out_size);
        out[out_size - 1] = fs::StringTraits::NullTerminator;

        /* Normalize it. */
        constexpr size_t WorkingDirectoryPathLength = sizeof(WorkingDirectoryPath) - 1;
        size_t normalized_length;
        return fs::PathNormalizer::Normalize(out + WorkingDirectoryPathLength - 1, &normalized_length, relative_path, out_size - (WorkingDirectoryPathLength - 1));
    }

    void DirectorySaveDataFileSystem::OnWritableFileClose() {
        std::scoped_lock lk(this->accessor_mutex);
        this->open_writable_files--;

        /* Nintendo does not check this, but I think it's sensible to do so. */
        AMS_ABORT_UNLESS(this->open_writable_files >= 0);
    }

    Result DirectorySaveDataFileSystem::CopySaveFromFileSystem(fs::fsa::IFileSystem *save_fs) {
        /* If the input save is null, there's nothing to copy. */
        R_SUCCEED_IF(save_fs == nullptr);

        /* Get a work buffer to work with. */
        std::unique_ptr<u8[]> work_buf;
        size_t work_buf_size;
        R_TRY(this->AllocateWorkBuffer(&work_buf, &work_buf_size, IdealWorkBufferSize));

        /* Copy the directory recursively. */
        R_TRY(fssystem::CopyDirectoryRecursively(this->base_fs, save_fs, fs::PathNormalizer::RootPath, fs::PathNormalizer::RootPath, work_buf.get(), work_buf_size));

        return this->Commit();
    }

    /* Overridden from IPathResolutionFileSystem */
    Result DirectorySaveDataFileSystem::DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const char *path, fs::OpenMode mode) {
        char full_path[fs::EntryNameLengthMax + 1];
        R_TRY(this->ResolveFullPath(full_path, sizeof(full_path), path));

        std::scoped_lock lk(this->accessor_mutex);
        std::unique_ptr<fs::fsa::IFile> base_file;
        R_TRY(this->base_fs->OpenFile(&base_file, full_path, mode));

        std::unique_ptr<DirectorySaveDataFile> file(new (std::nothrow) DirectorySaveDataFile(std::move(base_file), this, mode));
        R_UNLESS(file != nullptr, fs::ResultAllocationFailureInDirectorySaveDataFileSystem());

        if (mode & fs::OpenMode_Write) {
            this->open_writable_files++;
        }

        *out_file = std::move(file);
        return ResultSuccess();
    }

    Result DirectorySaveDataFileSystem::DoCommit() {
        /* Here, Nintendo does the following (with retries): */
        /* - Rename Committed -> Synchronizing. */
        /* - Synchronize Working -> Synchronizing (deleting Synchronizing). */
        /* - Rename Synchronizing -> Committed. */
        std::scoped_lock lk(this->accessor_mutex);

        R_UNLESS(this->open_writable_files == 0, fs::ResultPreconditionViolation());

        const auto RenameCommitedDir      = [&]() { return this->base_fs->RenameDirectory(CommittedDirectoryPath, SynchronizingDirectoryPath); };
        const auto SynchronizeWorkingDir  = [&]() { return this->SynchronizeDirectory(SynchronizingDirectoryPath, WorkingDirectoryPath); };
        const auto RenameSynchronizingDir = [&]() { return this->base_fs->RenameDirectory(SynchronizingDirectoryPath, CommittedDirectoryPath); };

        /* Rename Committed -> Synchronizing. */
        R_TRY(fssystem::RetryFinitelyForTargetLocked(std::move(RenameCommitedDir)));

        /* - Synchronize Working -> Synchronizing (deleting Synchronizing). */
        R_TRY(fssystem::RetryFinitelyForTargetLocked(std::move(SynchronizeWorkingDir)));

        /* - Rename Synchronizing -> Committed. */
        R_TRY(fssystem::RetryFinitelyForTargetLocked(std::move(RenameSynchronizingDir)));

        /* TODO: Should I call this->base_fs->Commit()? Nintendo does not. */
        return ResultSuccess();
    }

    /* Overridden from IPathResolutionFileSystem but not commands. */
    Result DirectorySaveDataFileSystem::DoCommitProvisionally(s64 counter) {
        /* Nintendo does nothing here. */
        return ResultSuccess();
    }

    Result DirectorySaveDataFileSystem::DoRollback() {
        /* Initialize overwrites the working directory with the committed directory. */
        return this->Initialize();
    }

    /* Explicitly overridden to be not implemented. */
    Result DirectorySaveDataFileSystem::DoGetFreeSpaceSize(s64 *out, const char *path) {
        return fs::ResultNotImplemented();
    }

    Result DirectorySaveDataFileSystem::DoGetTotalSpaceSize(s64 *out, const char *path) {
        return fs::ResultNotImplemented();
    }

    Result DirectorySaveDataFileSystem::DoGetFileTimeStampRaw(fs::FileTimeStampRaw *out, const char *path) {
        return fs::ResultNotImplemented();
    }

    Result DirectorySaveDataFileSystem::DoQueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fs::fsa::QueryId query, const char *path) {
        return fs::ResultNotImplemented();
    }

    Result DirectorySaveDataFileSystem::DoFlush() {
        return fs::ResultNotImplemented();
    }

}
