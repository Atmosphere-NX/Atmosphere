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

namespace ams::fs::impl {

    class FileServiceObjectAdapter : public ::ams::fs::impl::Newable, public ::ams::fs::fsa::IFile {
        NON_COPYABLE(FileServiceObjectAdapter);
        NON_MOVEABLE(FileServiceObjectAdapter);
        private:
            sf::SharedPointer<fssrv::sf::IFile> m_x;
        public:
            explicit FileServiceObjectAdapter(sf::SharedPointer<fssrv::sf::IFile> &&o) : m_x(o) { /* ... */}
            virtual ~FileServiceObjectAdapter() { /* ... */ }
        public:
            virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override final {
                s64 read_size = 0;
                R_TRY(m_x->Read(std::addressof(read_size), offset, sf::OutNonSecureBuffer(buffer, size), static_cast<s64>(size), option));

                *out = static_cast<size_t>(read_size);
                R_SUCCEED();
            }

            virtual Result DoGetSize(s64 *out) override final {
                R_RETURN(m_x->GetSize(out));
            }

            virtual Result DoFlush() override final {
                R_RETURN(m_x->Flush());
            }

            virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override final {
                R_RETURN(m_x->Write(offset, sf::InNonSecureBuffer(buffer, size), static_cast<s64>(size), option));
            }

            virtual Result DoSetSize(s64 size) override final {
                R_RETURN(m_x->SetSize(size));
            }

            virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
                switch (op_id) {
                    case OperationId::Invalidate:
                        {
                            fs::QueryRangeInfo dummy_range_info;
                            R_RETURN(m_x->OperateRange(std::addressof(dummy_range_info), static_cast<s32>(op_id), offset, size));
                        }
                    case OperationId::QueryRange:
                        {
                            R_UNLESS(dst != nullptr,                         fs::ResultNullptrArgument());
                            R_UNLESS(dst_size == sizeof(fs::QueryRangeInfo), fs::ResultInvalidSize());

                            R_RETURN(m_x->OperateRange(reinterpret_cast<fs::QueryRangeInfo *>(dst), static_cast<s32>(op_id), offset, size));
                        }
                    default:
                        {
                            R_RETURN(m_x->OperateRangeWithBuffer(sf::OutNonSecureBuffer(dst, dst_size), sf::InNonSecureBuffer(src, src_size), static_cast<s32>(op_id), offset, size));
                        }
                }
            }
        public:
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override final {
                AMS_ABORT("Invalid GetDomainObjectId call");
            }
    };

    class DirectoryServiceObjectAdapter : public ::ams::fs::impl::Newable, public ::ams::fs::fsa::IDirectory {
        NON_COPYABLE(DirectoryServiceObjectAdapter);
        NON_MOVEABLE(DirectoryServiceObjectAdapter);
        private:
            sf::SharedPointer<fssrv::sf::IDirectory> m_x;
        public:
            explicit DirectoryServiceObjectAdapter(sf::SharedPointer<fssrv::sf::IDirectory> &&o) : m_x(o) { /* ... */}
            virtual ~DirectoryServiceObjectAdapter() { /* ... */ }
        public:
            virtual Result DoRead(s64 *out_count, DirectoryEntry *out_entries, s64 max_entries) override final {
                R_RETURN(m_x->Read(out_count, sf::OutBuffer(out_entries, max_entries * sizeof(*out_entries))));
            }

            virtual Result DoGetEntryCount(s64 *out) override final {
                R_RETURN(m_x->GetEntryCount(out));
            }
        public:
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override final {
                AMS_ABORT("Invalid GetDomainObjectId call");
            }
    };

    class FileSystemServiceObjectAdapter : public ::ams::fs::impl::Newable, public ::ams::fs::fsa::IFileSystem {
        NON_COPYABLE(FileSystemServiceObjectAdapter);
        NON_MOVEABLE(FileSystemServiceObjectAdapter);
        private:
            sf::SharedPointer<fssrv::sf::IFileSystem> m_x;
        public:
            explicit FileSystemServiceObjectAdapter(sf::SharedPointer<fssrv::sf::IFileSystem> &&o) : m_x(o) { /* ... */}
            virtual ~FileSystemServiceObjectAdapter() { /* ... */ }

            sf::SharedPointer<fssrv::sf::IFileSystem> GetFileSystem() const { return m_x; }
        private:
            static Result GetPathForServiceObject(fssrv::sf::Path *out, const fs::Path &path) {
                const size_t len = util::Strlcpy<char>(out->str, path.GetString(), sizeof(out->str));
                R_UNLESS(len < sizeof(out->str), fs::ResultTooLongPath());
                R_SUCCEED();
            }
        private:
            virtual Result DoOpenFile(std::unique_ptr<fsa::IFile> *out_file, const fs::Path &path, OpenMode mode) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                /* Open the file. */
                sf::SharedPointer<fssrv::sf::IFile> file;
                R_TRY(m_x->OpenFile(std::addressof(file), fsp_path, mode));

                /* Create the output fsa file. */
                out_file->reset(new FileServiceObjectAdapter(std::move(file)));
                R_UNLESS(out_file != nullptr, fs::ResultAllocationMemoryFailedNew());

                R_SUCCEED();
            }

            virtual Result DoOpenDirectory(std::unique_ptr<fsa::IDirectory> *out_dir, const fs::Path &path, OpenDirectoryMode mode) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                /* Open the directory. */
                sf::SharedPointer<fssrv::sf::IDirectory> dir;
                R_TRY(m_x->OpenDirectory(std::addressof(dir), fsp_path, mode));

                /* Create the output fsa directory. */
                out_dir->reset(new DirectoryServiceObjectAdapter(std::move(dir)));
                R_UNLESS(out_dir != nullptr, fs::ResultAllocationMemoryFailedNew());

                R_SUCCEED();
            }

            virtual Result DoGetEntryType(DirectoryEntryType *out, const fs::Path &path) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                R_RETURN(m_x->GetEntryType(out, fsp_path));
            }

            virtual Result DoCommit() override final {
                R_RETURN(m_x->Commit());
            }

            virtual Result DoCreateFile(const fs::Path &path, s64 size, int flags) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                R_RETURN(m_x->CreateFile(fsp_path, size, flags));
            }

            virtual Result DoDeleteFile(const fs::Path &path) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                R_RETURN(m_x->DeleteFile(fsp_path));
            }

            virtual Result DoCreateDirectory(const fs::Path &path) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                R_RETURN(m_x->CreateDirectory(fsp_path));
            }

            virtual Result DoDeleteDirectory(const fs::Path &path) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                R_RETURN(m_x->DeleteDirectory(fsp_path));
            }

            virtual Result DoDeleteDirectoryRecursively(const fs::Path &path) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                R_RETURN(m_x->DeleteDirectoryRecursively(fsp_path));
            }

            virtual Result DoRenameFile(const fs::Path &old_path, const fs::Path &new_path) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_old_path;
                fssrv::sf::Path fsp_new_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_old_path), old_path));
                R_TRY(GetPathForServiceObject(std::addressof(fsp_new_path), new_path));

                R_RETURN(m_x->RenameFile(fsp_old_path, fsp_new_path));
            }

            virtual Result DoRenameDirectory(const fs::Path &old_path, const fs::Path &new_path) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_old_path;
                fssrv::sf::Path fsp_new_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_old_path), old_path));
                R_TRY(GetPathForServiceObject(std::addressof(fsp_new_path), new_path));

                R_RETURN(m_x->RenameDirectory(fsp_old_path, fsp_new_path));
            }

            virtual Result DoCleanDirectoryRecursively(const fs::Path &path) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                R_RETURN(m_x->CleanDirectoryRecursively(fsp_path));
            }

            virtual Result DoGetFreeSpaceSize(s64 *out, const fs::Path &path) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                R_RETURN(m_x->GetFreeSpaceSize(out, fsp_path));
            }

            virtual Result DoGetTotalSpaceSize(s64 *out, const fs::Path &path) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                R_RETURN(m_x->GetTotalSpaceSize(out, fsp_path));
            }

            virtual Result DoGetFileTimeStampRaw(fs::FileTimeStampRaw *out, const fs::Path &path) override final {
                /* Convert the path. */
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                R_RETURN(m_x->GetFileTimeStampRaw(out, fsp_path));
            }

            virtual Result DoQueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fs::fsa::QueryId query, const fs::Path &path) override {
                fssrv::sf::Path fsp_path;
                R_TRY(GetPathForServiceObject(std::addressof(fsp_path), path));

                R_RETURN(m_x->QueryEntry(sf::OutNonSecureBuffer(dst, dst_size), sf::InNonSecureBuffer(src, src_size), static_cast<s32>(query), fsp_path));
            }
    };

}
