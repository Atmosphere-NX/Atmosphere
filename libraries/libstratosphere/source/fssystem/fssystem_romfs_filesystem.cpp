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

        constexpr size_t CalculateRequiredWorkingMemorySize(const fs::RomFileSystemInformation &header) {
            return header.directory_bucket_size + header.directory_entry_size + header.file_bucket_size + header.file_entry_size;
        }

        class RomFsFile : public ams::fs::fsa::IFile, public ams::fs::impl::Newable {
            private:
                RomFsFileSystem *m_parent;
                s64 m_start;
                s64 m_end;
            private:
                s64 GetSize() const {
                    return m_end - m_start;
                }
            public:
                RomFsFile(RomFsFileSystem *p, s64 s, s64 e) : m_parent(p), m_start(s), m_end(e) { /* ... */ }
                virtual ~RomFsFile() { /* ... */ }
            public:
                virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override {
                    R_TRY(buffers::DoContinuouslyUntilBufferIsAllocated([&]() -> Result {
                        size_t read_size = 0;
                        R_TRY(this->DryRead(std::addressof(read_size), offset, size, option, fs::OpenMode_Read));

                        R_TRY(m_parent->GetBaseStorage()->Read(offset + m_start, buffer, read_size));
                        *out = read_size;

                        R_SUCCEED();
                    }, AMS_CURRENT_FUNCTION_NAME));

                    R_SUCCEED();
                }

                virtual Result DoGetSize(s64 *out) override {
                    *out = this->GetSize();
                    R_SUCCEED();
                }

                virtual Result DoFlush() override {
                    R_SUCCEED();
                }

                virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override {
                    AMS_UNUSED(buffer);

                    bool needs_append;
                    R_TRY(this->DryWrite(std::addressof(needs_append), offset, size, option, fs::OpenMode_Read));
                    AMS_ASSERT(needs_append == false);

                    R_THROW(fs::ResultUnsupportedWriteForRomFsFile());
                }

                virtual Result DoSetSize(s64 size) override {
                    R_TRY(this->DrySetSize(size, fs::OpenMode_Read));
                    R_THROW(fs::ResultUnsupportedWriteForRomFsFile());
                }

                virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                    switch (op_id) {
                        case fs::OperationId::Invalidate:
                            {
                                R_RETURN(buffers::DoContinuouslyUntilBufferIsAllocated([&]() -> Result {
                                    R_RETURN(m_parent->GetBaseStorage()->OperateRange(fs::OperationId::Invalidate, 0, std::numeric_limits<s64>::max()));
                                }, AMS_CURRENT_FUNCTION_NAME));
                            }
                        case fs::OperationId::QueryRange:
                            {
                                R_UNLESS(offset >= 0,               fs::ResultInvalidOffset());
                                R_UNLESS(this->GetSize() >= offset, fs::ResultOutOfRange());

                                auto operate_size = size;
                                if (offset + operate_size > this->GetSize() || offset + operate_size < offset) {
                                    operate_size = this->GetSize() - offset;
                                }

                                R_RETURN(buffers::DoContinuouslyUntilBufferIsAllocated([&]() -> Result {
                                    R_RETURN(m_parent->GetBaseStorage()->OperateRange(dst, dst_size, op_id, m_start + offset, operate_size, src, src_size));
                                }, AMS_CURRENT_FUNCTION_NAME));
                            }
                        default:
                            R_THROW(fs::ResultUnsupportedOperateRangeForRomFsFile());
                    }
                }
            public:
                virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                    AMS_ABORT();
                }
        };

        class RomFsDirectory : public ams::fs::fsa::IDirectory, public ams::fs::impl::Newable {
            private:
                using FindPosition = RomFsFileSystem::RomFileTable::FindPosition;
            private:
                RomFsFileSystem *m_parent;
                FindPosition m_current_find;
                FindPosition m_first_find;
                fs::OpenDirectoryMode m_mode;
            public:
                RomFsDirectory(RomFsFileSystem *p, const FindPosition &f, fs::OpenDirectoryMode m) : m_parent(p), m_current_find(f), m_first_find(f), m_mode(m) { /* ... */ }
                virtual ~RomFsDirectory() override { /* ... */ }
            public:
                virtual Result DoRead(s64 *out_count, fs::DirectoryEntry *out_entries, s64 max_entries) override {
                    R_TRY(buffers::DoContinuouslyUntilBufferIsAllocated([&]() -> Result {
                        R_RETURN(this->ReadInternal(out_count, std::addressof(m_current_find), out_entries, max_entries));
                    }, AMS_CURRENT_FUNCTION_NAME));
                    R_SUCCEED();
                }

                virtual Result DoGetEntryCount(s64 *out) override {
                    FindPosition find = m_first_find;

                    R_TRY(buffers::DoContinuouslyUntilBufferIsAllocated([&]() -> Result {
                        R_TRY(this->ReadInternal(out, std::addressof(find), nullptr, 0));
                        R_SUCCEED();
                    }, AMS_CURRENT_FUNCTION_NAME));
                    R_SUCCEED();
                }
            private:
                Result ReadInternal(s64 *out_count, FindPosition *find, fs::DirectoryEntry *out_entries, s64 max_entries) {
                    constexpr size_t NameBufferSize = fs::EntryNameLengthMax + 1;
                    fs::RomPathChar name[NameBufferSize];
                    s32 i = 0;

                    if (m_mode & fs::OpenDirectoryMode_Directory) {
                        while (i < max_entries || out_entries == nullptr) {
                            R_TRY_CATCH(m_parent->GetRomFileTable()->FindNextDirectory(name, find, NameBufferSize)) {
                                R_CATCH(fs::ResultDbmFindFinished) { break; }
                            } R_END_TRY_CATCH;

                            if (out_entries) {
                                R_UNLESS(strnlen(name, NameBufferSize) < NameBufferSize, fs::ResultTooLongPath());
                                strncpy(out_entries[i].name, name, fs::EntryNameLengthMax);
                                out_entries[i].name[fs::EntryNameLengthMax] = '\x00';
                                out_entries[i].type = fs::DirectoryEntryType_Directory;
                                out_entries[i].file_size = 0;
                            }

                            i++;
                        }
                    }

                    if (m_mode & fs::OpenDirectoryMode_File) {
                        while (i < max_entries || out_entries == nullptr) {
                            auto file_pos = find->next_file;

                            R_TRY_CATCH(m_parent->GetRomFileTable()->FindNextFile(name, find, NameBufferSize)) {
                                R_CATCH(fs::ResultDbmFindFinished) { break; }
                            } R_END_TRY_CATCH;

                            if (out_entries) {
                                R_UNLESS(strnlen(name, NameBufferSize) < NameBufferSize, fs::ResultTooLongPath());
                                strncpy(out_entries[i].name, name, fs::EntryNameLengthMax);
                                out_entries[i].name[fs::EntryNameLengthMax] = '\x00';
                                out_entries[i].type = fs::DirectoryEntryType_File;

                                RomFsFileSystem::RomFileTable::FileInfo file_info;
                                R_TRY(m_parent->GetRomFileTable()->OpenFile(std::addressof(file_info), m_parent->GetRomFileTable()->PositionToFileId(file_pos)));
                                out_entries[i].file_size = file_info.size.Get();
                            }

                            i++;
                        }
                    }

                    *out_count = i;
                    R_SUCCEED();
                }
            public:
                virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                    AMS_ABORT();
                }
        };

    }


    RomFsFileSystem::RomFsFileSystem() : m_base_storage() {
        /* ... */
    }

    RomFsFileSystem::~RomFsFileSystem() {
        /* ... */
    }

    fs::IStorage *RomFsFileSystem::GetBaseStorage() {
        return m_base_storage;
    }

    RomFsFileSystem::RomFileTable *RomFsFileSystem::GetRomFileTable() {
        return std::addressof(m_rom_file_table);
    }

    Result RomFsFileSystem::GetRequiredWorkingMemorySize(size_t *out, fs::IStorage *storage) {
        fs::RomFileSystemInformation header;

        R_TRY(buffers::DoContinuouslyUntilBufferIsAllocated([&]() -> Result {
            R_TRY(storage->Read(0, std::addressof(header), sizeof(header)));
            R_SUCCEED();
        }, AMS_CURRENT_FUNCTION_NAME));

        *out = CalculateRequiredWorkingMemorySize(header);
        R_SUCCEED();
    }

    Result RomFsFileSystem::Initialize(fs::IStorage *base, void *work, size_t work_size, bool use_cache) {
        AMS_ABORT_UNLESS(!use_cache || work != nullptr);
        AMS_ABORT_UNLESS(base != nullptr);

        /* Register blocking context for the scope. */
        buffers::ScopedBufferManagerContextRegistration _sr;
        buffers::EnableBlockingBufferManagerAllocation();

        /* Read the header. */
        fs::RomFileSystemInformation header;
        R_TRY(base->Read(0, std::addressof(header), sizeof(header)));

        /* Set up our storages. */
        if (use_cache) {
            const size_t needed_size = CalculateRequiredWorkingMemorySize(header);
            R_UNLESS(work_size >= needed_size, fs::ResultAllocationMemoryFailedInRomFsFileSystemA());

            u8 *buf = static_cast<u8 *>(work);
            auto dir_bucket_buf  = buf; buf += header.directory_bucket_size;
            auto dir_entry_buf   = buf; buf += header.directory_entry_size;
            auto file_bucket_buf = buf; buf += header.file_bucket_size;
            auto file_entry_buf  = buf; buf += header.file_entry_size;

            R_TRY(base->Read(header.directory_bucket_offset, dir_bucket_buf,  static_cast<size_t>(header.directory_bucket_size)));
            R_TRY(base->Read(header.directory_entry_offset,  dir_entry_buf,   static_cast<size_t>(header.directory_entry_size)));
            R_TRY(base->Read(header.file_bucket_offset,      file_bucket_buf, static_cast<size_t>(header.file_bucket_size)));
            R_TRY(base->Read(header.file_entry_offset,       file_entry_buf,  static_cast<size_t>(header.file_entry_size)));

            m_dir_bucket_storage.reset(new fs::MemoryStorage(dir_bucket_buf, header.directory_bucket_size));
            m_dir_entry_storage.reset(new fs::MemoryStorage(dir_entry_buf, header.directory_entry_size));
            m_file_bucket_storage.reset(new fs::MemoryStorage(file_bucket_buf, header.file_bucket_size));
            m_file_entry_storage.reset(new fs::MemoryStorage(file_entry_buf, header.file_entry_size));
        } else {
            m_dir_bucket_storage.reset(new fs::SubStorage(base, header.directory_bucket_offset, header.directory_bucket_size));
            m_dir_entry_storage.reset(new fs::SubStorage(base, header.directory_entry_offset, header.directory_entry_size));
            m_file_bucket_storage.reset(new fs::SubStorage(base, header.file_bucket_offset, header.file_bucket_size));
            m_file_entry_storage.reset(new fs::SubStorage(base, header.file_entry_offset, header.file_entry_size));
        }

        /* Ensure we allocated storages successfully. */
        R_UNLESS(m_dir_bucket_storage  != nullptr, fs::ResultAllocationMemoryFailedInRomFsFileSystemB());
        R_UNLESS(m_dir_entry_storage   != nullptr, fs::ResultAllocationMemoryFailedInRomFsFileSystemB());
        R_UNLESS(m_file_bucket_storage != nullptr, fs::ResultAllocationMemoryFailedInRomFsFileSystemB());
        R_UNLESS(m_file_entry_storage  != nullptr, fs::ResultAllocationMemoryFailedInRomFsFileSystemB());

        /* Initialize the rom table. */
        R_TRY(m_rom_file_table.Initialize(fs::SubStorage(m_dir_bucket_storage.get(),  0, static_cast<u32>(header.directory_bucket_size)),
                                          fs::SubStorage(m_dir_entry_storage.get(),   0, static_cast<u32>(header.directory_entry_size)),
                                          fs::SubStorage(m_file_bucket_storage.get(), 0, static_cast<u32>(header.file_bucket_size)),
                                          fs::SubStorage(m_file_entry_storage.get(),  0, static_cast<u32>(header.file_entry_size))));

        /* Set members. */
        m_entry_size = header.body_offset;
        m_base_storage = base;
        R_SUCCEED();
    }

    Result RomFsFileSystem::Initialize(std::shared_ptr<fs::IStorage> base, void *work, size_t work_size, bool use_cache) {
        m_shared_storage = std::move(base);
        R_RETURN(this->Initialize(m_shared_storage.get(), work, work_size, use_cache));
    }

    Result RomFsFileSystem::GetFileInfo(RomFileTable::FileInfo *out, const char *path) {
        R_TRY(buffers::DoContinuouslyUntilBufferIsAllocated([&]() -> Result {
            R_TRY_CATCH(m_rom_file_table.OpenFile(out, path)) {
                R_CONVERT(fs::ResultDbmNotFound, fs::ResultPathNotFound());
            } R_END_TRY_CATCH;

            R_SUCCEED();
        }, AMS_CURRENT_FUNCTION_NAME));

        R_SUCCEED();
    }

    Result RomFsFileSystem::GetFileBaseOffset(s64 *out, const fs::Path &path) {
        R_TRY(this->CheckPathFormat(path));

        RomFileTable::FileInfo info;
        R_TRY(this->GetFileInfo(std::addressof(info), path));
        *out = m_entry_size + info.offset.Get();
        R_SUCCEED();
    }

    Result RomFsFileSystem::DoCreateFile(const fs::Path &path, s64 size, int flags) {
        AMS_UNUSED(path, size, flags);
        R_THROW(fs::ResultUnsupportedWriteForRomFsFileSystem());
    }

    Result RomFsFileSystem::DoDeleteFile(const fs::Path &path) {
        AMS_UNUSED(path);
        R_THROW(fs::ResultUnsupportedWriteForRomFsFileSystem());
    }

    Result RomFsFileSystem::DoCreateDirectory(const fs::Path &path) {
        AMS_UNUSED(path);
        R_THROW(fs::ResultUnsupportedWriteForRomFsFileSystem());
    }

    Result RomFsFileSystem::DoDeleteDirectory(const fs::Path &path) {
        AMS_UNUSED(path);
        R_THROW(fs::ResultUnsupportedWriteForRomFsFileSystem());
    }

    Result RomFsFileSystem::DoDeleteDirectoryRecursively(const fs::Path &path) {
        AMS_UNUSED(path);
        R_THROW(fs::ResultUnsupportedWriteForRomFsFileSystem());
    }

    Result RomFsFileSystem::DoRenameFile(const fs::Path &old_path, const fs::Path &new_path) {
        AMS_UNUSED(old_path, new_path);
        R_THROW(fs::ResultUnsupportedWriteForRomFsFileSystem());
    }

    Result RomFsFileSystem::DoRenameDirectory(const fs::Path &old_path, const fs::Path &new_path) {
        AMS_UNUSED(old_path, new_path);
        R_THROW(fs::ResultUnsupportedWriteForRomFsFileSystem());
    }

    Result RomFsFileSystem::DoGetEntryType(fs::DirectoryEntryType *out, const fs::Path &path) {
        R_TRY(this->CheckPathFormat(path));

        R_TRY(buffers::DoContinuouslyUntilBufferIsAllocated([&]() -> Result {
            fs::HierarchicalRomFileTable::FindPosition find_pos;

            R_TRY_CATCH(m_rom_file_table.FindOpen(std::addressof(find_pos), path.GetString())) {
                R_CONVERT(fs::ResultDbmNotFound, fs::ResultPathNotFound())
                R_CATCH(fs::ResultDbmInvalidOperation) {
                    *out = fs::DirectoryEntryType_File;
                    R_SUCCEED();
                }
            } R_END_TRY_CATCH;

            *out = fs::DirectoryEntryType_Directory;
            R_SUCCEED();
        }, AMS_CURRENT_FUNCTION_NAME));

        R_SUCCEED();
    }

    Result RomFsFileSystem::DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const fs::Path &path, fs::OpenMode mode) {
        R_UNLESS(mode == fs::OpenMode_Read, fs::ResultInvalidOpenMode());

        R_TRY(this->CheckPathFormat(path));

        RomFileTable::FileInfo file_info;
        R_TRY(this->GetFileInfo(std::addressof(file_info), path));

        auto file = std::make_unique<RomFsFile>(this, m_entry_size + file_info.offset.Get(), m_entry_size + file_info.offset.Get() + file_info.size.Get());
        R_UNLESS(file != nullptr, fs::ResultAllocationMemoryFailedInRomFsFileSystemC());

        *out_file = std::move(file);
        R_SUCCEED();
    }

    Result RomFsFileSystem::DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const fs::Path &path, fs::OpenDirectoryMode mode) {
        R_TRY(this->CheckPathFormat(path));

        RomFileTable::FindPosition find;
        R_TRY(buffers::DoContinuouslyUntilBufferIsAllocated([&]() -> Result {
            R_TRY_CATCH(m_rom_file_table.FindOpen(std::addressof(find), path.GetString())) {
                R_CONVERT(fs::ResultDbmNotFound, fs::ResultPathNotFound())
            } R_END_TRY_CATCH;

            R_SUCCEED();
        }, AMS_CURRENT_FUNCTION_NAME));

        auto dir = std::make_unique<RomFsDirectory>(this, find, mode);
        R_UNLESS(dir != nullptr, fs::ResultAllocationMemoryFailedInRomFsFileSystemD());

        *out_dir = std::move(dir);
        R_SUCCEED();
    }

    Result RomFsFileSystem::DoCommit() {
        R_SUCCEED();
    }

    Result RomFsFileSystem::DoGetFreeSpaceSize(s64 *out, const fs::Path &path) {
        AMS_UNUSED(path);

        *out = 0;
        R_SUCCEED();
    }

    Result RomFsFileSystem::DoCleanDirectoryRecursively(const fs::Path &path) {
        AMS_UNUSED(path);
        R_THROW(fs::ResultUnsupportedWriteForRomFsFileSystem());
    }

    Result RomFsFileSystem::DoCommitProvisionally(s64 counter) {
        AMS_UNUSED(counter);
        R_THROW(fs::ResultUnsupportedCommitProvisionallyForRomFsFileSystem());
    }

}
