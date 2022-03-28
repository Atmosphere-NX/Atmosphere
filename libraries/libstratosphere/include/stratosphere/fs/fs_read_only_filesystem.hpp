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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fs/fsa/fs_ifile.hpp>
#include <stratosphere/fs/fsa/fs_idirectory.hpp>
#include <stratosphere/fs/fsa/fs_ifilesystem.hpp>

namespace ams::fs {

    /* ACCURATE_TO_VERSION: Unknown */

    namespace {

        class ReadOnlyFile : public fsa::IFile, public impl::Newable {
            NON_COPYABLE(ReadOnlyFile);
            NON_MOVEABLE(ReadOnlyFile);
            private:
                std::unique_ptr<fsa::IFile> m_base_file;
            public:
                explicit ReadOnlyFile(std::unique_ptr<fsa::IFile> &&f) : m_base_file(std::move(f)) { AMS_ASSERT(m_base_file != nullptr); }
                virtual ~ReadOnlyFile() { /* ... */ }
            private:
                virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override final {
                    R_RETURN(m_base_file->Read(out, offset, buffer, size, option));
                }

                virtual Result DoGetSize(s64 *out) override final {
                    R_RETURN(m_base_file->GetSize(out));
                }

                virtual Result DoFlush() override final {
                    R_SUCCEED();
                }

                virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override final {
                    bool need_append;
                    R_TRY(this->DryWrite(std::addressof(need_append), offset, size, option, fs::OpenMode_Read));

                    AMS_ASSERT(!need_append);

                    AMS_UNUSED(buffer);
                    R_THROW(fs::ResultUnsupportedWriteForReadOnlyFile());
                }

                virtual Result DoSetSize(s64 size) override final {
                    R_TRY(this->DrySetSize(size, fs::OpenMode_Read));
                    R_THROW(fs::ResultUnsupportedWriteForReadOnlyFile());
                }

                virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
                    switch (op_id) {
                        case OperationId::Invalidate:
                        case OperationId::QueryRange:
                            R_RETURN(m_base_file->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                        default:
                            R_THROW(fs::ResultUnsupportedOperateRangeForReadOnlyFile());
                    }
                }
            public:
                virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                    return m_base_file->GetDomainObjectId();
                }
        };

    }

    template<typename T>
    class ReadOnlyFileSystemTemplate : public fsa::IFileSystem, public impl::Newable {
        NON_COPYABLE(ReadOnlyFileSystemTemplate);
        NON_MOVEABLE(ReadOnlyFileSystemTemplate);
        private:
            T m_base_fs;
        public:
            explicit ReadOnlyFileSystemTemplate(T &&fs) : m_base_fs(std::move(fs)) { /* ... */ }
            virtual ~ReadOnlyFileSystemTemplate() { /* ... */ }
        private:
            virtual Result DoOpenFile(std::unique_ptr<fsa::IFile> *out_file, const fs::Path &path, OpenMode mode) override final {
                /* Only allow opening files with mode = read. */
                R_UNLESS((mode & fs::OpenMode_All) == fs::OpenMode_Read, fs::ResultInvalidOpenMode());

                std::unique_ptr<fsa::IFile> base_file;
                R_TRY(m_base_fs->OpenFile(std::addressof(base_file), path, mode));

                auto read_only_file = std::make_unique<ReadOnlyFile>(std::move(base_file));
                R_UNLESS(read_only_file != nullptr, fs::ResultAllocationMemoryFailedInReadOnlyFileSystemA());

                *out_file = std::move(read_only_file);
                R_SUCCEED();
            }

            virtual Result DoOpenDirectory(std::unique_ptr<fsa::IDirectory> *out_dir, const fs::Path &path, OpenDirectoryMode mode) override final {
                R_RETURN(m_base_fs->OpenDirectory(out_dir, path, mode));
            }

            virtual Result DoGetEntryType(DirectoryEntryType *out, const fs::Path &path) override final {
                R_RETURN(m_base_fs->GetEntryType(out, path));
            }

            virtual Result DoCommit() override final {
                R_SUCCEED();
            }

            virtual Result DoCreateFile(const fs::Path &path, s64 size, int flags) override final {
                AMS_UNUSED(path, size, flags);
                R_THROW(fs::ResultUnsupportedWriteForReadOnlyFileSystem());
            }

            virtual Result DoDeleteFile(const fs::Path &path) override final {
                AMS_UNUSED(path);
                R_THROW(fs::ResultUnsupportedWriteForReadOnlyFileSystem());
            }

            virtual Result DoCreateDirectory(const fs::Path &path) override final {
                AMS_UNUSED(path);
                R_THROW(fs::ResultUnsupportedWriteForReadOnlyFileSystem());
            }

            virtual Result DoDeleteDirectory(const fs::Path &path) override final {
                AMS_UNUSED(path);
                R_THROW(fs::ResultUnsupportedWriteForReadOnlyFileSystem());
            }

            virtual Result DoDeleteDirectoryRecursively(const fs::Path &path) override final {
                AMS_UNUSED(path);
                R_THROW(fs::ResultUnsupportedWriteForReadOnlyFileSystem());
            }

            virtual Result DoRenameFile(const fs::Path &old_path, const fs::Path &new_path) override final {
                AMS_UNUSED(old_path, new_path);
                R_THROW(fs::ResultUnsupportedWriteForReadOnlyFileSystem());
            }

            virtual Result DoRenameDirectory(const fs::Path &old_path, const fs::Path &new_path) override final {
                AMS_UNUSED(old_path, new_path);
                R_THROW(fs::ResultUnsupportedWriteForReadOnlyFileSystem());
            }

            virtual Result DoCleanDirectoryRecursively(const fs::Path &path) override final {
                AMS_UNUSED(path);
                R_THROW(fs::ResultUnsupportedWriteForReadOnlyFileSystem());
            }

            virtual Result DoGetFreeSpaceSize(s64 *out, const fs::Path &path) override final {
                R_RETURN(m_base_fs->GetFreeSpaceSize(out, path));
            }

            virtual Result DoGetTotalSpaceSize(s64 *out, const fs::Path &path) override final {
                AMS_UNUSED(out, path);
                R_THROW(fs::ResultUnsupportedGetTotalSpaceSizeForReadOnlyFileSystem());
            }

            virtual Result DoCommitProvisionally(s64 counter) override final {
                AMS_UNUSED(counter);
                R_THROW(fs::ResultUnsupportedCommitProvisionallyForReadOnlyFileSystem());
            }
    };

    using ReadOnlyFileSystem       = ReadOnlyFileSystemTemplate<std::unique_ptr<::ams::fs::fsa::IFileSystem>>;
    using ReadOnlyFileSystemShared = ReadOnlyFileSystemTemplate<std::shared_ptr<::ams::fs::fsa::IFileSystem>>;

}
