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
#pragma once
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fs/fsa/fs_ifile.hpp>
#include <stratosphere/fs/fsa/fs_idirectory.hpp>
#include <stratosphere/fs/fsa/fs_ifilesystem.hpp>

namespace ams::fs {

    namespace {

        class ReadOnlyFile : public fsa::IFile, public impl::Newable {
            NON_COPYABLE(ReadOnlyFile);
            NON_MOVEABLE(ReadOnlyFile);
            private:
                std::unique_ptr<fsa::IFile> base_file;
            public:
                explicit ReadOnlyFile(std::unique_ptr<fsa::IFile> &&f) : base_file(std::move(f)) { /* ... */ }
                virtual ~ReadOnlyFile() { /* ... */ }
            private:
                virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override final {
                    return this->base_file->Read(out, offset, buffer, size, option);
                }

                virtual Result DoGetSize(s64 *out) override final {
                    return this->base_file->GetSize(out);
                }

                virtual Result DoFlush() override final {
                    return ResultSuccess();
                }

                virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override final {
                    return fs::ResultUnsupportedOperationInReadOnlyFileA();
                }

                virtual Result DoSetSize(s64 size) override final {
                    return fs::ResultUnsupportedOperationInReadOnlyFileA();
                }

                virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
                    switch (op_id) {
                        case OperationId::InvalidateCache:
                        case OperationId::QueryRange:
                            return this->base_file->OperateRange(dst, dst_size, op_id, offset, size, src, src_size);
                        default:
                            return fs::ResultUnsupportedOperationInReadOnlyFileB();
                    }
                }
            public:
                virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                    return this->base_file->GetDomainObjectId();
                }
        };

    }

    template<typename T>
    class ReadOnlyFileSystemTemplate : public fsa::IFileSystem, public impl::Newable {
        NON_COPYABLE(ReadOnlyFileSystemTemplate);
        NON_MOVEABLE(ReadOnlyFileSystemTemplate);
        private:
            T base_fs;
        public:
            explicit ReadOnlyFileSystemTemplate(T &&fs) : base_fs(std::move(fs)) { /* ... */ }
            virtual ~ReadOnlyFileSystemTemplate() { /* ... */ }
        private:
            virtual Result DoOpenFile(std::unique_ptr<fsa::IFile> *out_file, const char *path, OpenMode mode) override final {
                /* Only allow opening files with mode = read. */
                R_UNLESS((mode & fs::OpenMode_All) == fs::OpenMode_Read, fs::ResultInvalidOpenMode());

                std::unique_ptr<fsa::IFile> base_file;
                R_TRY(this->base_fs->OpenFile(std::addressof(base_file), path, mode));

                auto read_only_file = std::make_unique<ReadOnlyFile>(std::move(base_file));
                R_UNLESS(read_only_file != nullptr, fs::ResultAllocationFailureInReadOnlyFileSystemA());

                *out_file = std::move(read_only_file);
                return ResultSuccess();
            }

            virtual Result DoOpenDirectory(std::unique_ptr<fsa::IDirectory> *out_dir, const char *path, OpenDirectoryMode mode) override final {
                return this->base_fs->OpenDirectory(out_dir, path, mode);
            }

            virtual Result DoGetEntryType(DirectoryEntryType *out, const char *path) override final {
                return this->base_fs->GetEntryType(out, path);
            }

            virtual Result DoCommit() override final {
                return ResultSuccess();
            }

            virtual Result DoCreateFile(const char *path, s64 size, int flags) override final {
                return fs::ResultUnsupportedOperationInReadOnlyFileSystemTemplateA();
            }

            virtual Result DoDeleteFile(const char *path) override final {
                return fs::ResultUnsupportedOperationInReadOnlyFileSystemTemplateA();
            }

            virtual Result DoCreateDirectory(const char *path) override final {
                return fs::ResultUnsupportedOperationInReadOnlyFileSystemTemplateA();
            }

            virtual Result DoDeleteDirectory(const char *path) override final {
                return fs::ResultUnsupportedOperationInReadOnlyFileSystemTemplateA();
            }

            virtual Result DoDeleteDirectoryRecursively(const char *path) override final {
                return fs::ResultUnsupportedOperationInReadOnlyFileSystemTemplateA();
            }

            virtual Result DoRenameFile(const char *old_path, const char *new_path) override final {
                return fs::ResultUnsupportedOperationInReadOnlyFileSystemTemplateA();
            }

            virtual Result DoRenameDirectory(const char *old_path, const char *new_path) override final {
                return fs::ResultUnsupportedOperationInReadOnlyFileSystemTemplateA();
            }

            virtual Result DoCleanDirectoryRecursively(const char *path) override final {
                return fs::ResultUnsupportedOperationInReadOnlyFileSystemTemplateA();
            }

            virtual Result DoGetFreeSpaceSize(s64 *out, const char *path) override final {
                return fs::ResultUnsupportedOperationInReadOnlyFileSystemTemplateB();
            }

            virtual Result DoGetTotalSpaceSize(s64 *out, const char *path) override final {
                return fs::ResultUnsupportedOperationInReadOnlyFileSystemTemplateB();
            }

            virtual Result DoCommitProvisionally(s64 counter) override final {
                return fs::ResultUnsupportedOperationInReadOnlyFileSystemTemplateC();
            }
    };

    using ReadOnlyFileSystem       = ReadOnlyFileSystemTemplate<std::unique_ptr<::ams::fs::fsa::IFileSystem>>;
    using ReadOnlyFileSystemShared = ReadOnlyFileSystemTemplate<std::shared_ptr<::ams::fs::fsa::IFileSystem>>;

}
