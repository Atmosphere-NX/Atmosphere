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
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>

namespace ams::fssrv::impl {

    FileInterfaceAdapter::FileInterfaceAdapter(std::unique_ptr<fs::fsa::IFile> &&file, std::shared_ptr<FileSystemInterfaceAdapter> &&parent, std::unique_lock<fssystem::SemaphoreAdapter> &&sema)
        : parent_filesystem(std::move(parent)), base_file(std::move(file)), open_count_semaphore(std::move(sema))
    {
        /* ... */
    }

    FileInterfaceAdapter::~FileInterfaceAdapter() {
        /* ... */
    }

    void FileInterfaceAdapter::InvalidateCache() {
        AMS_ABORT_UNLESS(this->parent_filesystem->IsDeepRetryEnabled());
        std::scoped_lock<os::ReadWriteLock> scoped_write_lock(this->parent_filesystem->GetReadWriteLockForCacheInvalidation());
        this->base_file->OperateRange(nullptr, 0, fs::OperationId::InvalidateCache, 0, std::numeric_limits<s64>::max(), nullptr, 0);
    }

    Result FileInterfaceAdapter::Read(ams::sf::Out<s64> out, s64 offset, const ams::sf::OutNonSecureBuffer &buffer, s64 size, fs::ReadOption option) {
        /* TODO: N retries on ResultDataCorrupted, we may want to eventually. */
        /* TODO: Deep retry */
        R_UNLESS(offset >= 0, fs::ResultInvalidOffset());
        R_UNLESS(size >= 0,   fs::ResultInvalidSize());

        size_t read_size = 0;
        R_TRY(this->base_file->Read(&read_size, offset, buffer.GetPointer(), static_cast<size_t>(size), option));

        out.SetValue(read_size);
        return ResultSuccess();
    }

    Result FileInterfaceAdapter::Write(s64 offset, const ams::sf::InNonSecureBuffer &buffer, s64 size, fs::WriteOption option) {
        /* TODO: N increases thread priority temporarily when writing. We may want to eventually. */
        R_UNLESS(offset >= 0, fs::ResultInvalidOffset());
        R_UNLESS(size >= 0,   fs::ResultInvalidSize());

        auto read_lock = this->parent_filesystem->AcquireCacheInvalidationReadLock();
        return this->base_file->Write(offset, buffer.GetPointer(), size, option);
    }

    Result FileInterfaceAdapter::Flush() {
        auto read_lock = this->parent_filesystem->AcquireCacheInvalidationReadLock();
        return this->base_file->Flush();
    }

    Result FileInterfaceAdapter::SetSize(s64 size) {
        R_UNLESS(size >= 0, fs::ResultInvalidSize());
        auto read_lock = this->parent_filesystem->AcquireCacheInvalidationReadLock();
        return this->base_file->SetSize(size);
    }

    Result FileInterfaceAdapter::GetSize(ams::sf::Out<s64> out) {
        auto read_lock = this->parent_filesystem->AcquireCacheInvalidationReadLock();
        return this->base_file->GetSize(out.GetPointer());
    }

    Result FileInterfaceAdapter::OperateRange(ams::sf::Out<fs::FileQueryRangeInfo> out, s32 op_id, s64 offset, s64 size) {
        /* N includes this redundant check, so we will too. */
        R_UNLESS(out.GetPointer() != nullptr, fs::ResultNullptrArgument());

        out->Clear();
        if (op_id == static_cast<s32>(fs::OperationId::QueryRange)) {
            auto read_lock = this->parent_filesystem->AcquireCacheInvalidationReadLock();

            fs::FileQueryRangeInfo info;
            R_TRY(this->base_file->OperateRange(&info, sizeof(info), fs::OperationId::QueryRange, offset, size, nullptr, 0));
            out->Merge(info);
        }

        return ResultSuccess();
    }

    DirectoryInterfaceAdapter::DirectoryInterfaceAdapter(std::unique_ptr<fs::fsa::IDirectory> &&dir, std::shared_ptr<FileSystemInterfaceAdapter> &&parent, std::unique_lock<fssystem::SemaphoreAdapter> &&sema)
        : parent_filesystem(std::move(parent)), base_dir(std::move(dir)), open_count_semaphore(std::move(sema))
    {
        /* ... */
    }

    DirectoryInterfaceAdapter::~DirectoryInterfaceAdapter() {
        /* ... */
    }

    Result DirectoryInterfaceAdapter::Read(ams::sf::Out<s64> out, const ams::sf::OutBuffer &out_entries) {
        auto read_lock = this->parent_filesystem->AcquireCacheInvalidationReadLock();

        const size_t max_num_entries = out_entries.GetSize() / sizeof(fs::DirectoryEntry);
        R_UNLESS(max_num_entries >= 0, fs::ResultInvalidSize());

        /* TODO: N retries on ResultDataCorrupted, we may want to eventually. */
        return this->base_dir->Read(out.GetPointer(), reinterpret_cast<fs::DirectoryEntry *>(out_entries.GetPointer()), max_num_entries);
    }

    Result DirectoryInterfaceAdapter::GetEntryCount(ams::sf::Out<s64> out) {
        auto read_lock = this->parent_filesystem->AcquireCacheInvalidationReadLock();
        return this->base_dir->GetEntryCount(out.GetPointer());
    }

    FileSystemInterfaceAdapter::FileSystemInterfaceAdapter(std::shared_ptr<fs::fsa::IFileSystem> &&fs, bool open_limited)
        : base_fs(std::move(fs)), open_count_limited(open_limited), deep_retry_enabled(false)
    {
        /* ... */
    }

    FileSystemInterfaceAdapter::~FileSystemInterfaceAdapter() {
        /* ... */
    }

    bool FileSystemInterfaceAdapter::IsDeepRetryEnabled() const {
        return this->deep_retry_enabled;
    }

    bool FileSystemInterfaceAdapter::IsAccessFailureDetectionObserved() const {
        /* TODO: This calls into fssrv::FileSystemProxyImpl, which we don't have yet. */
        AMS_ABORT_UNLESS(false);
    }

    std::optional<std::shared_lock<os::ReadWriteLock>> FileSystemInterfaceAdapter::AcquireCacheInvalidationReadLock() {
        std::optional<std::shared_lock<os::ReadWriteLock>> lock;
        if (this->deep_retry_enabled) {
            lock.emplace(this->invalidation_lock);
        }
        return lock;
    }

    os::ReadWriteLock &FileSystemInterfaceAdapter::GetReadWriteLockForCacheInvalidation() {
        return this->invalidation_lock;
    }

    Result FileSystemInterfaceAdapter::CreateFile(const fssrv::sf::Path &path, s64 size, s32 option) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        R_UNLESS(size >= 0, fs::ResultInvalidSize());

        PathNormalizer normalizer(path.str);
        R_UNLESS(normalizer.GetPath() != nullptr, normalizer.GetResult());

        return this->base_fs->CreateFile(normalizer.GetPath(), size, option);
    }

    Result FileSystemInterfaceAdapter::DeleteFile(const fssrv::sf::Path &path) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        PathNormalizer normalizer(path.str);
        R_UNLESS(normalizer.GetPath() != nullptr, normalizer.GetResult());

        return this->base_fs->DeleteFile(normalizer.GetPath());
    }

    Result FileSystemInterfaceAdapter::CreateDirectory(const fssrv::sf::Path &path) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        PathNormalizer normalizer(path.str);
        R_UNLESS(normalizer.GetPath() != nullptr, normalizer.GetResult());

        R_UNLESS(strncmp(normalizer.GetPath(), "/", 2) != 0, fs::ResultPathAlreadyExists());

        return this->base_fs->CreateDirectory(normalizer.GetPath());
    }

    Result FileSystemInterfaceAdapter::DeleteDirectory(const fssrv::sf::Path &path) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        PathNormalizer normalizer(path.str);
        R_UNLESS(normalizer.GetPath() != nullptr, normalizer.GetResult());

        R_UNLESS(strncmp(normalizer.GetPath(), "/", 2) != 0, fs::ResultDirectoryNotDeletable());

        return this->base_fs->DeleteDirectory(normalizer.GetPath());
    }

    Result FileSystemInterfaceAdapter::DeleteDirectoryRecursively(const fssrv::sf::Path &path) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        PathNormalizer normalizer(path.str);
        R_UNLESS(normalizer.GetPath() != nullptr, normalizer.GetResult());

        R_UNLESS(strncmp(normalizer.GetPath(), "/", 2) != 0, fs::ResultDirectoryNotDeletable());

        return this->base_fs->DeleteDirectoryRecursively(normalizer.GetPath());
    }

    Result FileSystemInterfaceAdapter::RenameFile(const fssrv::sf::Path &old_path, const fssrv::sf::Path &new_path) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        PathNormalizer old_normalizer(old_path.str);
        PathNormalizer new_normalizer(new_path.str);
        R_UNLESS(old_normalizer.GetPath() != nullptr, old_normalizer.GetResult());
        R_UNLESS(new_normalizer.GetPath() != nullptr, new_normalizer.GetResult());

        return this->base_fs->RenameFile(old_normalizer.GetPath(), new_normalizer.GetPath());
    }

    Result FileSystemInterfaceAdapter::RenameDirectory(const fssrv::sf::Path &old_path, const fssrv::sf::Path &new_path) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        PathNormalizer old_normalizer(old_path.str);
        PathNormalizer new_normalizer(new_path.str);
        R_UNLESS(old_normalizer.GetPath() != nullptr, old_normalizer.GetResult());
        R_UNLESS(new_normalizer.GetPath() != nullptr, new_normalizer.GetResult());

        const bool is_subpath = fs::IsSubPath(old_normalizer.GetPath(), new_normalizer.GetPath());
        R_UNLESS(!is_subpath, fs::ResultDirectoryNotRenamable());

        return this->base_fs->RenameFile(old_normalizer.GetPath(), new_normalizer.GetPath());
    }

    Result FileSystemInterfaceAdapter::GetEntryType(ams::sf::Out<u32> out, const fssrv::sf::Path &path) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        PathNormalizer normalizer(path.str);
        R_UNLESS(normalizer.GetPath() != nullptr, normalizer.GetResult());

        static_assert(sizeof(*out.GetPointer()) == sizeof(fs::DirectoryEntryType));
        return this->base_fs->GetEntryType(reinterpret_cast<fs::DirectoryEntryType *>(out.GetPointer()), normalizer.GetPath());
    }

    Result FileSystemInterfaceAdapter::OpenFile(ams::sf::Out<std::shared_ptr<fssrv::sf::IFile>> out, const fssrv::sf::Path &path, u32 mode) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        std::unique_lock<fssystem::SemaphoreAdapter> open_count_semaphore;
        if (this->open_count_limited) {
            /* TODO: This calls into fssrv::FileSystemProxyImpl, which we don't have yet. */
            AMS_ABORT_UNLESS(false);
        }

        PathNormalizer normalizer(path.str);
        R_UNLESS(normalizer.GetPath() != nullptr, normalizer.GetResult());

        /* TODO: N retries on ResultDataCorrupted, we may want to eventually. */
        std::unique_ptr<fs::fsa::IFile> file;
        R_TRY(this->base_fs->OpenFile(&file, normalizer.GetPath(), static_cast<fs::OpenMode>(mode)));

        /* TODO: This is a hack to get the mitm API to work. Better solution? */
        const auto target_object_id = file->GetDomainObjectId();

        /* TODO: N creates an nn::fssystem::AsynchronousAccessFile here. */

        std::shared_ptr<FileSystemInterfaceAdapter> shared_this = this->shared_from_this();
        auto file_intf = ams::sf::MakeShared<fssrv::sf::IFile, FileInterfaceAdapter>(std::move(file), std::move(shared_this), std::move(open_count_semaphore));
        R_UNLESS(file_intf != nullptr, fs::ResultAllocationFailureInFileSystemInterfaceAdapter());

        out.SetValue(std::move(file_intf), target_object_id);
        return ResultSuccess();
    }

    Result FileSystemInterfaceAdapter::OpenDirectory(ams::sf::Out<std::shared_ptr<fssrv::sf::IDirectory>> out, const fssrv::sf::Path &path, u32 mode) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        std::unique_lock<fssystem::SemaphoreAdapter> open_count_semaphore;
        if (this->open_count_limited) {
            /* TODO: This calls into fssrv::FileSystemProxyImpl, which we don't have yet. */
            AMS_ABORT_UNLESS(false);
        }

        PathNormalizer normalizer(path.str);
        R_UNLESS(normalizer.GetPath() != nullptr, normalizer.GetResult());

        /* TODO: N retries on ResultDataCorrupted, we may want to eventually. */
        std::unique_ptr<fs::fsa::IDirectory> dir;
        R_TRY(this->base_fs->OpenDirectory(&dir, normalizer.GetPath(), static_cast<fs::OpenDirectoryMode>(mode)));

        /* TODO: This is a hack to get the mitm API to work. Better solution? */
        const auto target_object_id = dir->GetDomainObjectId();

        std::shared_ptr<FileSystemInterfaceAdapter> shared_this = this->shared_from_this();
        auto dir_intf = ams::sf::MakeShared<fssrv::sf::IDirectory, DirectoryInterfaceAdapter>(std::move(dir), std::move(shared_this), std::move(open_count_semaphore));
        R_UNLESS(dir_intf != nullptr, fs::ResultAllocationFailureInFileSystemInterfaceAdapter());

        out.SetValue(std::move(dir_intf), target_object_id);
        return ResultSuccess();
    }

    Result FileSystemInterfaceAdapter::Commit() {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        return this->base_fs->Commit();
    }

    Result FileSystemInterfaceAdapter::GetFreeSpaceSize(ams::sf::Out<s64> out, const fssrv::sf::Path &path) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        PathNormalizer normalizer(path.str);
        R_UNLESS(normalizer.GetPath() != nullptr, normalizer.GetResult());

        return this->base_fs->GetFreeSpaceSize(out.GetPointer(), normalizer.GetPath());
    }

    Result FileSystemInterfaceAdapter::GetTotalSpaceSize(ams::sf::Out<s64> out, const fssrv::sf::Path &path) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        PathNormalizer normalizer(path.str);
        R_UNLESS(normalizer.GetPath() != nullptr, normalizer.GetResult());

        return this->base_fs->GetTotalSpaceSize(out.GetPointer(), normalizer.GetPath());
    }

    Result FileSystemInterfaceAdapter::CleanDirectoryRecursively(const fssrv::sf::Path &path) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        PathNormalizer normalizer(path.str);
        R_UNLESS(normalizer.GetPath() != nullptr, normalizer.GetResult());

        return this->base_fs->CleanDirectoryRecursively(normalizer.GetPath());
    }

    Result FileSystemInterfaceAdapter::GetFileTimeStampRaw(ams::sf::Out<fs::FileTimeStampRaw> out, const fssrv::sf::Path &path) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        PathNormalizer normalizer(path.str);
        R_UNLESS(normalizer.GetPath() != nullptr, normalizer.GetResult());

        return this->base_fs->GetFileTimeStampRaw(out.GetPointer(), normalizer.GetPath());
    }

    Result FileSystemInterfaceAdapter::QueryEntry(const ams::sf::OutBuffer &out_buf, const ams::sf::InBuffer &in_buf, s32 query_id, const fssrv::sf::Path &path) {
        auto read_lock = this->AcquireCacheInvalidationReadLock();

        /* TODO: Nintendo does not normalize the path. Should we? */

        char *dst       = reinterpret_cast<      char *>(out_buf.GetPointer());
        const char *src = reinterpret_cast<const char *>(in_buf.GetPointer());
        return this->base_fs->QueryEntry(dst, out_buf.GetSize(), src, in_buf.GetSize(), static_cast<fs::fsa::QueryId>(query_id), path.str);
    }

}
