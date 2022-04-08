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
#include "fssrv_retry_utility.hpp"

namespace ams::fssrv::impl {

    namespace {

        constexpr const char *RootDirectory = "/";

    }

    FileInterfaceAdapter::FileInterfaceAdapter(std::unique_ptr<fs::fsa::IFile> &&file, FileSystemInterfaceAdapter *parent, bool allow_all)
        : m_parent_filesystem(parent, true), m_base_file(std::move(file)), m_allow_all_operations(allow_all)
    {
        /* ... */
    }

    Result FileInterfaceAdapter::Read(ams::sf::Out<s64> out, s64 offset, const ams::sf::OutNonSecureBuffer &buffer, s64 size, fs::ReadOption option) {
        /* Check pre-conditions. */
        R_UNLESS(0 <= offset,                                fs::ResultInvalidOffset());
        R_UNLESS(0 <= size,                                  fs::ResultInvalidSize());
        R_UNLESS(size <= static_cast<s64>(buffer.GetSize()), fs::ResultInvalidSize());

        /* Read the data, retrying on corruption. */
        size_t read_size = 0;
        R_TRY(RetryFinitelyForDataCorrupted([&] () ALWAYS_INLINE_LAMBDA {
            R_RETURN(m_base_file->Read(std::addressof(read_size), offset, buffer.GetPointer(), static_cast<size_t>(size), option));
        }));

        /* Set the output size. */
        *out = read_size;
        R_SUCCEED();
    }

    Result FileInterfaceAdapter::Write(s64 offset, const ams::sf::InNonSecureBuffer &buffer, s64 size, fs::WriteOption option) {
        /* Check pre-conditions. */
        R_UNLESS(0 <= offset,                                fs::ResultInvalidOffset());
        R_UNLESS(0 <= size,                                  fs::ResultInvalidSize());
        R_UNLESS(size <= static_cast<s64>(buffer.GetSize()), fs::ResultInvalidSize());

        /* Temporarily increase our thread's priority. */
        fssystem::ScopedThreadPriorityChangerByAccessPriority cp(fssystem::ScopedThreadPriorityChangerByAccessPriority::AccessMode::Write);

        R_RETURN(m_base_file->Write(offset, buffer.GetPointer(), size, option));
    }

    Result FileInterfaceAdapter::Flush() {
        R_RETURN(m_base_file->Flush());
    }

    Result FileInterfaceAdapter::SetSize(s64 size) {
        R_UNLESS(size >= 0, fs::ResultInvalidSize());
        R_RETURN(m_base_file->SetSize(size));
    }

    Result FileInterfaceAdapter::GetSize(ams::sf::Out<s64> out) {
        /* Get the size, retrying on corruption. */
        R_RETURN(RetryFinitelyForDataCorrupted([&] () ALWAYS_INLINE_LAMBDA {
            R_RETURN(m_base_file->GetSize(out.GetPointer()));
        }));
    }

    Result FileInterfaceAdapter::OperateRange(ams::sf::Out<fs::FileQueryRangeInfo> out, s32 op_id, s64 offset, s64 size) {
        /* N includes this redundant check, so we will too. */
        R_UNLESS(out.GetPointer() != nullptr, fs::ResultNullptrArgument());

        /* Clear the range info. */
        out->Clear();

        if (op_id == static_cast<s32>(fs::OperationId::QueryRange)) {
            fs::FileQueryRangeInfo info;
            R_TRY(m_base_file->OperateRange(std::addressof(info), sizeof(info), fs::OperationId::QueryRange, offset, size, nullptr, 0));
            out->Merge(info);
        } else if (op_id == static_cast<s32>(fs::OperationId::Invalidate)) {
            R_TRY(m_base_file->OperateRange(nullptr, 0, fs::OperationId::Invalidate, offset, size, nullptr, 0));
        }

        R_SUCCEED();
    }

    Result FileInterfaceAdapter::OperateRangeWithBuffer(const ams::sf::OutNonSecureBuffer &out_buf, const ams::sf::InNonSecureBuffer &in_buf, s32 op_id, s64 offset, s64 size) {
        /* Check that we have permission to perform the operation. */
        switch (static_cast<fs::OperationId>(op_id)) {
            using enum fs::OperationId;
            case QueryUnpreparedRange:
            case QueryLazyLoadCompletionRate:
            case SetLazyLoadPriority:
                /* Lazy load/unprepared operations are always allowed to be performed with buffer. */
                break;
            default:
                R_UNLESS(m_allow_all_operations, fs::ResultPermissionDenied());
        }

        /* Perform the operation. */
        R_RETURN(m_base_file->OperateRange(out_buf.GetPointer(), out_buf.GetSize(), static_cast<fs::OperationId>(op_id), offset, size, in_buf.GetPointer(), in_buf.GetSize()));
    }

    DirectoryInterfaceAdapter::DirectoryInterfaceAdapter(std::unique_ptr<fs::fsa::IDirectory> &&dir, FileSystemInterfaceAdapter *parent, bool allow_all)
        : m_parent_filesystem(parent, true), m_base_dir(std::move(dir)), m_allow_all_operations(allow_all)
    {
        /* ... */
    }

    Result DirectoryInterfaceAdapter::Read(ams::sf::Out<s64> out, const ams::sf::OutBuffer &out_entries) {
        /* Get the maximum number of entries we can read into the buffer. */
        const s64 max_num_entries = out_entries.GetSize() / sizeof(fs::DirectoryEntry);
        R_UNLESS(max_num_entries >= 0, fs::ResultInvalidSize());

        /* Get the size, retrying on corruption. */
        s64 num_read = 0;
        R_TRY(RetryFinitelyForDataCorrupted([&] () ALWAYS_INLINE_LAMBDA {
            R_RETURN(m_base_dir->Read(std::addressof(num_read), reinterpret_cast<fs::DirectoryEntry *>(out_entries.GetPointer()), max_num_entries));
        }));

        /* Set the output. */
        *out = num_read;
        R_SUCCEED();

    }

    Result DirectoryInterfaceAdapter::GetEntryCount(ams::sf::Out<s64> out) {
        R_RETURN(m_base_dir->GetEntryCount(out.GetPointer()));
    }

    Result FileSystemInterfaceAdapter::SetUpPath(fs::Path *out, const fssrv::sf::Path &sf_path) {
        /* Initialize the fs path. */
        if (m_path_flags.IsWindowsPathAllowed()) {
            R_TRY(out->InitializeWithReplaceUnc(sf_path.str));
        } else {
            R_TRY(out->Initialize(sf_path.str));
        }

        /* Ensure the path is normalized. */
        R_RETURN(out->Normalize(m_path_flags));
    }

    Result FileSystemInterfaceAdapter::CreateFile(const fssrv::sf::Path &path, s64 size, s32 option) {
        /* Check pre-conditions. */
        R_UNLESS(size >= 0, fs::ResultInvalidSize());

        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        R_RETURN(m_base_fs->CreateFile(fs_path, size, option));
    }

    Result FileSystemInterfaceAdapter::DeleteFile(const fssrv::sf::Path &path) {
        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        R_RETURN(m_base_fs->DeleteFile(fs_path));
    }

    Result FileSystemInterfaceAdapter::CreateDirectory(const fssrv::sf::Path &path) {
        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        /* Sanity check that the directory isn't the root. */
        R_UNLESS(fs_path != RootDirectory, fs::ResultPathAlreadyExists());

        R_RETURN(m_base_fs->CreateDirectory(fs_path));
    }

    Result FileSystemInterfaceAdapter::DeleteDirectory(const fssrv::sf::Path &path) {
        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        /* Sanity check that the directory isn't the root. */
        R_UNLESS(fs_path != RootDirectory, fs::ResultDirectoryNotDeletable());

        R_RETURN(m_base_fs->DeleteDirectory(fs_path));
    }

    Result FileSystemInterfaceAdapter::DeleteDirectoryRecursively(const fssrv::sf::Path &path) {
        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        /* Sanity check that the directory isn't the root. */
        R_UNLESS(fs_path != RootDirectory, fs::ResultDirectoryNotDeletable());

        R_RETURN(m_base_fs->DeleteDirectoryRecursively(fs_path));
    }

    Result FileSystemInterfaceAdapter::RenameFile(const fssrv::sf::Path &old_path, const fssrv::sf::Path &new_path) {
        /* Normalize the input paths. */
        fs::Path fs_old_path;
        fs::Path fs_new_path;
        R_TRY(this->SetUpPath(std::addressof(fs_old_path), old_path));
        R_TRY(this->SetUpPath(std::addressof(fs_new_path), new_path));

        R_RETURN(m_base_fs->RenameFile(fs_old_path, fs_new_path));
    }

    Result FileSystemInterfaceAdapter::RenameDirectory(const fssrv::sf::Path &old_path, const fssrv::sf::Path &new_path) {
        /* Normalize the input paths. */
        fs::Path fs_old_path;
        fs::Path fs_new_path;
        R_TRY(this->SetUpPath(std::addressof(fs_old_path), old_path));
        R_TRY(this->SetUpPath(std::addressof(fs_new_path), new_path));

        R_UNLESS(!fs::IsSubPath(fs_old_path.GetString(), fs_new_path.GetString()), fs::ResultDirectoryNotRenamable());

        R_RETURN(m_base_fs->RenameDirectory(fs_old_path, fs_new_path));
    }

    Result FileSystemInterfaceAdapter::GetEntryType(ams::sf::Out<u32> out, const fssrv::sf::Path &path) {
        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        static_assert(sizeof(*out.GetPointer()) == sizeof(fs::DirectoryEntryType));
        R_RETURN(m_base_fs->GetEntryType(reinterpret_cast<fs::DirectoryEntryType *>(out.GetPointer()), fs_path));
    }

    Result FileSystemInterfaceAdapter::OpenFile(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFile>> out, const fssrv::sf::Path &path, u32 mode) {
        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        /* Open the file, retrying on corruption. */
        std::unique_ptr<fs::fsa::IFile> file;
        R_TRY(RetryFinitelyForDataCorrupted([&] () ALWAYS_INLINE_LAMBDA {
            R_RETURN(m_base_fs->OpenFile(std::addressof(file), fs_path, static_cast<fs::OpenMode>(mode)));
        }));

        /* If we're a mitm interface, we should preserve the resulting target object id. */
        if (m_is_mitm_interface) {
            /* TODO: This is a hack to get the mitm API to work. Better solution? */
            const auto target_object_id = file->GetDomainObjectId();

            ams::sf::SharedPointer<fssrv::sf::IFile> file_intf = FileSystemObjectFactory::CreateSharedEmplaced<fssrv::sf::IFile, FileInterfaceAdapter>(std::move(file), this, m_allow_all_operations);
            R_UNLESS(file_intf != nullptr, fs::ResultAllocationMemoryFailedInFileSystemInterfaceAdapterA());

            out.SetValue(std::move(file_intf), target_object_id);
        } else {
            ams::sf::SharedPointer<fssrv::sf::IFile> file_intf = FileSystemObjectFactory::CreateSharedEmplaced<fssrv::sf::IFile, FileInterfaceAdapter>(std::move(file), this, m_allow_all_operations);
            R_UNLESS(file_intf != nullptr, fs::ResultAllocationMemoryFailedInFileSystemInterfaceAdapterA());

            out.SetValue(std::move(file_intf));
        }

        R_SUCCEED();
    }

    Result FileSystemInterfaceAdapter::OpenDirectory(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IDirectory>> out, const fssrv::sf::Path &path, u32 mode) {
        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        /* Open the directory, retrying on corruption. */
        std::unique_ptr<fs::fsa::IDirectory> dir;
        R_TRY(RetryFinitelyForDataCorrupted([&] () ALWAYS_INLINE_LAMBDA {
            R_RETURN(m_base_fs->OpenDirectory(std::addressof(dir), fs_path, static_cast<fs::OpenDirectoryMode>(mode)));
        }));

        /* If we're a mitm interface, we should preserve the resulting target object id. */
        if (m_is_mitm_interface) {
            /* TODO: This is a hack to get the mitm API to work. Better solution? */
            const auto target_object_id = dir->GetDomainObjectId();

            ams::sf::SharedPointer<fssrv::sf::IDirectory> dir_intf = FileSystemObjectFactory::CreateSharedEmplaced<fssrv::sf::IDirectory, DirectoryInterfaceAdapter>(std::move(dir), this, m_allow_all_operations);
            R_UNLESS(dir_intf != nullptr, fs::ResultAllocationMemoryFailedInFileSystemInterfaceAdapterA());

            out.SetValue(std::move(dir_intf), target_object_id);
        } else {
            ams::sf::SharedPointer<fssrv::sf::IDirectory> dir_intf = FileSystemObjectFactory::CreateSharedEmplaced<fssrv::sf::IDirectory, DirectoryInterfaceAdapter>(std::move(dir), this, m_allow_all_operations);
            R_UNLESS(dir_intf != nullptr, fs::ResultAllocationMemoryFailedInFileSystemInterfaceAdapterA());

            out.SetValue(std::move(dir_intf));
        }

        R_SUCCEED();
    }

    Result FileSystemInterfaceAdapter::Commit() {
        R_RETURN(m_base_fs->Commit());
    }

    Result FileSystemInterfaceAdapter::GetFreeSpaceSize(ams::sf::Out<s64> out, const fssrv::sf::Path &path) {
        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        R_RETURN(m_base_fs->GetFreeSpaceSize(out.GetPointer(), fs_path));
    }

    Result FileSystemInterfaceAdapter::GetTotalSpaceSize(ams::sf::Out<s64> out, const fssrv::sf::Path &path) {
        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        R_RETURN(m_base_fs->GetTotalSpaceSize(out.GetPointer(), fs_path));
    }

    Result FileSystemInterfaceAdapter::CleanDirectoryRecursively(const fssrv::sf::Path &path) {
        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        R_RETURN(m_base_fs->CleanDirectoryRecursively(fs_path));
    }

    Result FileSystemInterfaceAdapter::GetFileTimeStampRaw(ams::sf::Out<fs::FileTimeStampRaw> out, const fssrv::sf::Path &path) {
        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        R_RETURN(m_base_fs->GetFileTimeStampRaw(out.GetPointer(), fs_path));
    }

    Result FileSystemInterfaceAdapter::QueryEntry(const ams::sf::OutNonSecureBuffer &out_buf, const ams::sf::InNonSecureBuffer &in_buf, s32 query_id, const fssrv::sf::Path &path) {
        /* Check that we have permission to perform the operation. */
        switch (static_cast<fs::fsa::QueryId>(query_id)) {
            using enum fs::fsa::QueryId;
            case SetConcatenationFileAttribute:
            case IsSignedSystemPartitionOnSdCardValid:
            case QueryUnpreparedFileInformation:
                /* Only certain operations are unconditionally allowable. */
                break;
            default:
                R_UNLESS(m_allow_all_operations, fs::ResultPermissionDenied());
        }

        /* Normalize the input path. */
        fs::Path fs_path;
        R_TRY(this->SetUpPath(std::addressof(fs_path), path));

        char *dst       = reinterpret_cast<char *>(out_buf.GetPointer());
        const char *src = reinterpret_cast<const char *>(in_buf.GetPointer());
        R_RETURN(m_base_fs->QueryEntry(dst, out_buf.GetSize(), src, in_buf.GetSize(), static_cast<fs::fsa::QueryId>(query_id), fs_path));
    }

    #if defined(ATMOSPHERE_OS_HORIZON)
    Result RemoteFileSystem::OpenFile(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFile>> out, const fssrv::sf::Path &path, u32 mode) {
        FsFile f;
        R_TRY(fsFsOpenFile(std::addressof(m_base_fs), path.str, mode, std::addressof(f)));

        auto intf = FileSystemObjectFactory::CreateSharedEmplaced<fssrv::sf::IFile, RemoteFile>(f);
        R_UNLESS(intf != nullptr, fs::ResultAllocationMemoryFailedInFileSystemInterfaceAdapterA());

        out.SetValue(std::move(intf));
        R_SUCCEED();
    }

    Result RemoteFileSystem::OpenDirectory(ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IDirectory>> out, const fssrv::sf::Path &path, u32 mode) {
        FsDir d;
        R_TRY(fsFsOpenDirectory(std::addressof(m_base_fs), path.str, mode, std::addressof(d)));

        auto intf = FileSystemObjectFactory::CreateSharedEmplaced<fssrv::sf::IDirectory, RemoteDirectory>(d);
        R_UNLESS(intf != nullptr, fs::ResultAllocationMemoryFailedInFileSystemInterfaceAdapterA());

        out.SetValue(std::move(intf));
        R_SUCCEED();
    }
    #endif

}
