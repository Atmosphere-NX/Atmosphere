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

namespace ams::fs {

    namespace {

        Result ConvertNcaCorruptedResult(Result res) {
            AMS_ASSERT(fs::ResultNcaCorrupted::Includes(res));

            R_TRY_CATCH(res) {
                R_CONVERT(fs::ResultInvalidNcaFileSystemType,                   fs::ResultInvalidRomNcaFileSystemType())
                R_CONVERT(fs::ResultInvalidAcidFileSize,                        fs::ResultInvalidRomAcidFileSize())
                R_CONVERT(fs::ResultInvalidAcidSize,                            fs::ResultInvalidRomAcidSize())
                R_CONVERT(fs::ResultInvalidAcid,                                fs::ResultInvalidRomAcid())
                R_CONVERT(fs::ResultAcidVerificationFailed,                     fs::ResultRomAcidVerificationFailed())
                R_CONVERT(fs::ResultInvalidNcaSignature,                        fs::ResultInvalidRomNcaSignature())
                R_CONVERT(fs::ResultNcaHeaderSignature1VerificationFailed,      fs::ResultRomNcaHeaderSignature1VerificationFailed())
                R_CONVERT(fs::ResultNcaHeaderSignature2VerificationFailed,      fs::ResultRomNcaHeaderSignature2VerificationFailed())
                R_CONVERT(fs::ResultNcaFsHeaderHashVerificationFailed,          fs::ResultRomNcaFsHeaderHashVerificationFailed())
                R_CONVERT(fs::ResultInvalidNcaKeyIndex,                         fs::ResultInvalidRomNcaKeyIndex())
                R_CONVERT(fs::ResultInvalidNcaFsHeaderHashType,                 fs::ResultInvalidRomNcaFsHeaderHashType())
                R_CONVERT(fs::ResultInvalidNcaFsHeaderEncryptionType,           fs::ResultInvalidRomNcaFsHeaderEncryptionType())
                R_CONVERT(fs::ResultInvalidHierarchicalSha256BlockSize,         fs::ResultInvalidRomHierarchicalSha256BlockSize())
                R_CONVERT(fs::ResultInvalidHierarchicalSha256LayerCount,        fs::ResultInvalidRomHierarchicalSha256LayerCount())
                R_CONVERT(fs::ResultHierarchicalSha256BaseStorageTooLarge,      fs::ResultRomHierarchicalSha256BaseStorageTooLarge())
                R_CONVERT(fs::ResultHierarchicalSha256HashVerificationFailed,   fs::ResultRomHierarchicalSha256HashVerificationFailed())
                R_CATCH_ALL() { /* ... */ }
            } R_END_TRY_CATCH;

            AMS_ASSERT(false);
            return fs::ResultNcaCorrupted();
        }

        Result ConvertIntegrityVerificationStorageCorruptedResult(Result res) {
            AMS_ASSERT(fs::ResultIntegrityVerificationStorageCorrupted::Includes(res));

            R_TRY_CATCH(res) {
                R_CONVERT(fs::ResultIncorrectIntegrityVerificationMagic,                fs::ResultIncorrectRomIntegrityVerificationMagic())
                R_CONVERT(fs::ResultInvalidZeroHash,                                    fs::ResultInvalidRomZeroHash())
                R_CONVERT(fs::ResultNonRealDataVerificationFailed,                      fs::ResultRomNonRealDataVerificationFailed())
                R_CONVERT(fs::ResultInvalidHierarchicalIntegrityVerificationLayerCount, fs::ResultInvalidRomHierarchicalIntegrityVerificationLayerCount())
                R_CONVERT(fs::ResultClearedRealDataVerificationFailed,                  fs::ResultClearedRomRealDataVerificationFailed())
                R_CONVERT(fs::ResultUnclearedRealDataVerificationFailed,                fs::ResultUnclearedRomRealDataVerificationFailed())
                R_CATCH_ALL() { /* ... */ }
            } R_END_TRY_CATCH;

            AMS_ASSERT(false);
            return fs::ResultIntegrityVerificationStorageCorrupted();
        }

        Result ConvertBuiltInStorageCorruptedResult(Result res) {
            AMS_ASSERT(fs::ResultBuiltInStorageCorrupted::Includes(res));

            R_TRY_CATCH(res) {
                R_CONVERT(fs::ResultGptHeaderVerificationFailed, fs::ResultRomGptHeaderVerificationFailed())
                R_CATCH_ALL() { /* ... */ }
            } R_END_TRY_CATCH;

            AMS_ASSERT(false);
            return fs::ResultBuiltInStorageCorrupted();
        }

        Result ConvertPartitionFileSystemCorruptedResult(Result res) {
            AMS_ASSERT(fs::ResultPartitionFileSystemCorrupted::Includes(res));

            R_TRY_CATCH(res) {
                R_CONVERT(fs::ResultInvalidSha256PartitionHashTarget,           fs::ResultInvalidRomSha256PartitionHashTarget())
                R_CONVERT(fs::ResultSha256PartitionHashVerificationFailed,      fs::ResultRomSha256PartitionHashVerificationFailed())
                R_CONVERT(fs::ResultPartitionSignatureVerificationFailed,       fs::ResultRomPartitionSignatureVerificationFailed())
                R_CONVERT(fs::ResultSha256PartitionSignatureVerificationFailed, fs::ResultRomSha256PartitionSignatureVerificationFailed())
                R_CONVERT(fs::ResultInvalidPartitionEntryOffset,                fs::ResultInvalidRomPartitionEntryOffset())
                R_CONVERT(fs::ResultInvalidSha256PartitionMetaDataSize,         fs::ResultInvalidRomSha256PartitionMetaDataSize())
                R_CATCH_ALL() { /* ... */ }
            } R_END_TRY_CATCH;

            AMS_ASSERT(false);
            return fs::ResultPartitionFileSystemCorrupted();
        }

        Result ConvertFatFileSystemCorruptedResult(Result res) {
            AMS_ASSERT(fs::ResultFatFileSystemCorrupted::Includes(res));

            return res;
        }

        Result ConvertHostFileSystemCorruptedResult(Result res) {
            AMS_ASSERT(fs::ResultHostFileSystemCorrupted::Includes(res));

            R_TRY_CATCH(res) {
                R_CONVERT(fs::ResultHostEntryCorrupted,    fs::ResultRomHostEntryCorrupted())
                R_CONVERT(fs::ResultHostFileDataCorrupted, fs::ResultRomHostFileDataCorrupted())
                R_CONVERT(fs::ResultHostFileCorrupted,     fs::ResultRomHostFileCorrupted())
                R_CONVERT(fs::ResultInvalidHostHandle,     fs::ResultInvalidRomHostHandle())
                R_CATCH_ALL() { /* ... */ }
            } R_END_TRY_CATCH;

            AMS_ASSERT(false);
            return fs::ResultHostFileSystemCorrupted();
        }

        Result ConvertDatabaseCorruptedResult(Result res) {
            AMS_ASSERT(fs::ResultDatabaseCorrupted::Includes(res));

            R_TRY_CATCH(res) {
                R_CONVERT(fs::ResultInvalidAllocationTableBlock,     fs::ResultInvalidRomAllocationTableBlock())
                R_CONVERT(fs::ResultInvalidKeyValueListElementIndex, fs::ResultInvalidRomKeyValueListElementIndex())
                R_CATCH_ALL() { /* ... */ }
            } R_END_TRY_CATCH;

            AMS_ASSERT(false);
            return fs::ResultDatabaseCorrupted();
        }

        Result ConvertRomFsResult(Result res) {
            R_TRY_CATCH(res) {
                R_CONVERT(fs::ResultUnsupportedVersion,                    fs::ResultUnsupportedRomVersion())
                R_CONVERT(fs::ResultNcaCorrupted,                          ConvertNcaCorruptedResult(res))
                R_CONVERT(fs::ResultIntegrityVerificationStorageCorrupted, ConvertIntegrityVerificationStorageCorruptedResult(res))
                R_CONVERT(fs::ResultBuiltInStorageCorrupted,               ConvertBuiltInStorageCorruptedResult(res))
                R_CONVERT(fs::ResultPartitionFileSystemCorrupted,          ConvertPartitionFileSystemCorruptedResult(res))
                R_CONVERT(fs::ResultFatFileSystemCorrupted,                ConvertFatFileSystemCorruptedResult(res))
                R_CONVERT(fs::ResultHostFileSystemCorrupted,               ConvertHostFileSystemCorruptedResult(res))
                R_CONVERT(fs::ResultDatabaseCorrupted,                     ConvertDatabaseCorruptedResult(res))
                R_CONVERT(fs::ResultNotFound,                              fs::ResultPathNotFound())
                R_CONVERT(fs::ResultPermissionDenied,                      fs::ResultTargetLocked())
                R_CONVERT(fs::ResultIncompatiblePath,                      fs::ResultPathNotFound())
            } R_END_TRY_CATCH;

            return ResultSuccess();
        }

        Result ReadFile(IStorage *storage, s64 offset, void *buffer, size_t size) {
            AMS_ASSERT(storage != nullptr);
            AMS_ASSERT(offset >= 0);
            AMS_ASSERT(buffer != nullptr || size == 0);

            return ConvertRomFsResult(storage->Read(offset, buffer, size));
        }

        Result ReadFileHeader(IStorage *storage, RomFileSystemInformation *out) {
            AMS_ASSERT(storage != nullptr);
            AMS_ASSERT(out != nullptr);

            return ReadFile(storage, 0, out, sizeof(*out));
        }

        constexpr size_t CalculateRequiredWorkingMemorySize(const RomFileSystemInformation &header) {
            const size_t needed_size = header.directory_bucket_size + header.directory_entry_size + header.file_bucket_size + header.file_entry_size;
            return util::AlignUp(needed_size, 8);
        }

        class RomFsFile : public fsa::IFile, public impl::Newable {
            private:
                RomFsFileSystem *parent;
                s64 start;
                s64 end;
            public:
                RomFsFile(RomFsFileSystem *p, s64 s, s64 e) : parent(p), start(s), end(e) { /* ... */ }
                virtual ~RomFsFile() { /* ... */ }

                Result VerifyArguments(size_t *out, s64 offset, void *buf, size_t size, const fs::ReadOption &option) {
                    R_TRY(DryRead(out, offset, size, option, fs::OpenMode_Read));

                    AMS_ASSERT(this->GetStorage() != nullptr);
                    AMS_ASSERT(offset >= 0);
                    AMS_ASSERT(buf != nullptr || size == 0);

                    return ResultSuccess();
                }

                Result ConvertResult(Result res) const {
                    return ConvertRomFsResult(res);
                }

                s64 GetOffset() const {
                    return this->start;
                }

                s64 GetSize() const {
                    return this->end - this->start;
                }

                IStorage *GetStorage() {
                    return this->parent->GetBaseStorage();
                }
            public:
                virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override {
                    size_t read_size = 0;
                    R_TRY(this->VerifyArguments(std::addressof(read_size), offset, buffer, size, option));

                    R_TRY(this->ConvertResult(this->GetStorage()->Read(offset + this->start, buffer, size)));
                    *out = read_size;

                    return ResultSuccess();
                }

                virtual Result DoGetSize(s64 *out) override {
                    *out = this->GetSize();
                    return ResultSuccess();
                }

                virtual Result DoFlush() override {
                    return ResultSuccess();
                }

                virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override {
                    return fs::ResultUnsupportedOperationInRomFsFileA();
                }

                virtual Result DoSetSize(s64 size) override {
                    return fs::ResultUnsupportedOperationInRomFsFileA();
                }

                virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                    switch (op_id) {
                        case OperationId::InvalidateCache:
                        case OperationId::QueryRange:
                            {
                                R_UNLESS(offset >= 0,          fs::ResultOutOfRange());
                                R_UNLESS(this->GetSize() >= 0, fs::ResultOutOfRange());

                                auto operate_size = size;
                                if (offset + operate_size > this->GetSize() || offset + operate_size < offset) {
                                    operate_size = this->GetSize() - offset;
                                }

                                return this->GetStorage()->OperateRange(dst, dst_size, op_id, this->start + offset, operate_size, src, src_size);
                            }
                        default:
                            return fs::ResultUnsupportedOperationInRomFsFileB();
                    }
                }
            public:
                virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                    AMS_ABORT();
                }
        };

        class RomFsDirectory : public fsa::IDirectory, public impl::Newable {
            private:
                using FindPosition = RomFsFileSystem::RomFileTable::FindPosition;
            private:
                RomFsFileSystem *parent;
                FindPosition current_find;
                FindPosition first_find;
                fs::OpenDirectoryMode mode;
            public:
                RomFsDirectory(RomFsFileSystem *p, const FindPosition &f, fs::OpenDirectoryMode m) : parent(p), current_find(f), first_find(f), mode(m) { /* ... */ }
                virtual ~RomFsDirectory() override { /* ... */ }
            public:
                virtual Result DoRead(s64 *out_count, DirectoryEntry *out_entries, s64 max_entries) {
                    return this->ReadInternal(out_count, std::addressof(this->current_find), out_entries, max_entries);
                }

                virtual Result DoGetEntryCount(s64 *out) {
                    FindPosition find = this->first_find;
                    return this->ReadInternal(out, std::addressof(find), nullptr, 0);
                }
            private:
                Result ReadInternal(s64 *out_count, FindPosition *find, DirectoryEntry *out_entries, s64 max_entries) {
                    AMS_ASSERT(out_count != nullptr);
                    AMS_ASSERT(find != nullptr);

                    constexpr size_t NameBufferSize = fs::EntryNameLengthMax + 1;
                    char *name_buf = static_cast<char *>(::ams::fs::impl::Allocate(NameBufferSize));
                    R_UNLESS(name_buf != nullptr, fs::ResultAllocationFailureInRomFsFileSystemE());
                    ON_SCOPE_EXIT { ::ams::fs::impl::Deallocate(name_buf, NameBufferSize); };

                    s32 i = 0;

                    if (this->mode & fs::OpenDirectoryMode_Directory) {
                        while (i < max_entries || out_entries == nullptr) {
                            R_TRY_CATCH(this->parent->GetRomFileTable()->FindNextDirectory(name_buf, find, NameBufferSize)) {
                                R_CATCH(fs::ResultDbmFindFinished) { break; }
                            } R_END_TRY_CATCH;

                            if (out_entries) {
                                R_UNLESS(strnlen(name_buf, NameBufferSize) < NameBufferSize, fs::ResultTooLongPath());
                                strncpy(out_entries[i].name, name_buf, fs::EntryNameLengthMax);
                                out_entries[i].name[fs::EntryNameLengthMax] = '\x00';
                                out_entries[i].type = fs::DirectoryEntryType_Directory;
                                out_entries[i].file_size = 0;
                            }

                            i++;
                        }
                    }

                    if (this->mode & fs::OpenDirectoryMode_File) {
                        while (i < max_entries || out_entries == nullptr) {
                            auto file_pos = find->next_file;

                            R_TRY_CATCH(this->parent->GetRomFileTable()->FindNextFile(name_buf, find, NameBufferSize)) {
                                R_CATCH(fs::ResultDbmFindFinished) { break; }
                            } R_END_TRY_CATCH;

                            if (out_entries) {
                                R_UNLESS(strnlen(name_buf, NameBufferSize) < NameBufferSize, fs::ResultTooLongPath());
                                strncpy(out_entries[i].name, name_buf, fs::EntryNameLengthMax);
                                out_entries[i].name[fs::EntryNameLengthMax] = '\x00';
                                out_entries[i].type = fs::DirectoryEntryType_File;

                                RomFsFileSystem::RomFileTable::FileInfo file_info;
                                R_TRY(this->parent->GetRomFileTable()->OpenFile(std::addressof(file_info), this->parent->GetRomFileTable()->PositionToFileId(file_pos)));
                                out_entries[i].file_size = file_info.size.Get();
                            }

                            i++;
                        }
                    }

                    *out_count = i;
                    return ResultSuccess();
                }
            public:
                virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                    AMS_ABORT();
                }
        };

    }


    RomFsFileSystem::RomFsFileSystem() : base_storage() {
        /* ... */
    }

    RomFsFileSystem::~RomFsFileSystem() {
        /* ... */
    }

    Result RomFsFileSystem::GetRequiredWorkingMemorySize(size_t *out, IStorage *storage) {
        RomFileSystemInformation header;
        R_TRY(ReadFileHeader(storage, std::addressof(header)));

        *out = CalculateRequiredWorkingMemorySize(header);
        return ResultSuccess();
    }

    Result RomFsFileSystem::Initialize(IStorage *base, void *work, size_t work_size, bool use_cache) {
        AMS_ABORT_UNLESS(!use_cache || work != nullptr);
        AMS_ABORT_UNLESS(base != nullptr);

        /* Read the header. */
        RomFileSystemInformation header;
        R_TRY(ReadFileHeader(base, std::addressof(header)));

        /* Set up our storages. */
        if (use_cache) {
            const size_t needed_size = CalculateRequiredWorkingMemorySize(header);
            R_UNLESS(work_size >= needed_size, fs::ResultPreconditionViolation());

            u8 *buf = static_cast<u8 *>(work);
            auto dir_bucket_buf  = buf; buf += header.directory_bucket_size;
            auto dir_entry_buf   = buf; buf += header.directory_entry_size;
            auto file_bucket_buf = buf; buf += header.file_bucket_size;
            auto file_entry_buf  = buf; buf += header.file_entry_size;

            R_TRY(ReadFile(base, header.directory_bucket_offset, dir_bucket_buf,  header.directory_bucket_size));
            R_TRY(ReadFile(base, header.directory_entry_offset,  dir_entry_buf,   header.directory_entry_size));
            R_TRY(ReadFile(base, header.file_bucket_offset,      file_bucket_buf, header.file_bucket_size));
            R_TRY(ReadFile(base, header.file_entry_offset,       file_entry_buf,  header.file_entry_size));

            this->dir_bucket_storage.reset(new MemoryStorage(dir_bucket_buf, header.directory_bucket_size));
            this->dir_entry_storage.reset(new MemoryStorage(dir_entry_buf, header.directory_entry_size));
            this->file_bucket_storage.reset(new MemoryStorage(file_bucket_buf, header.file_bucket_size));
            this->file_entry_storage.reset(new MemoryStorage(file_entry_buf, header.file_entry_size));
        } else {
            this->dir_bucket_storage.reset(new SubStorage(base, header.directory_bucket_offset, header.directory_bucket_size));
            this->dir_entry_storage.reset(new SubStorage(base, header.directory_entry_offset, header.directory_entry_size));
            this->file_bucket_storage.reset(new SubStorage(base, header.file_bucket_offset, header.file_bucket_size));
            this->file_entry_storage.reset(new SubStorage(base, header.file_entry_offset, header.file_entry_size));
        }

        /* Ensure we allocated storages successfully. */
        R_UNLESS(this->dir_bucket_storage  != nullptr, fs::ResultAllocationFailureInRomFsFileSystemA());
        R_UNLESS(this->dir_entry_storage   != nullptr, fs::ResultAllocationFailureInRomFsFileSystemA());
        R_UNLESS(this->file_bucket_storage != nullptr, fs::ResultAllocationFailureInRomFsFileSystemA());
        R_UNLESS(this->file_entry_storage  != nullptr, fs::ResultAllocationFailureInRomFsFileSystemA());

        /* Initialize the rom table. */
        {

            SubStorage db(this->dir_bucket_storage.get(),  0, header.directory_bucket_size);
            SubStorage de(this->dir_entry_storage.get(),   0, header.directory_entry_size);
            SubStorage fb(this->file_bucket_storage.get(), 0, header.file_bucket_size);
            SubStorage fe(this->file_entry_storage.get(),  0, header.file_entry_size);
            R_TRY(this->rom_file_table.Initialize(db, de, fb, fe));
        }

        /* Set members. */
        this->entry_size = header.body_offset;
        this->base_storage = base;
        return ResultSuccess();
    }

    Result RomFsFileSystem::Initialize(std::unique_ptr<IStorage>&& base, void *work, size_t work_size, bool use_cache) {
        this->unique_storage = std::move(base);
        return this->Initialize(this->unique_storage.get(), work, work_size, use_cache);
    }

    Result RomFsFileSystem::GetFileInfo(RomFileTable::FileInfo *out, const char *path) {
        R_TRY_CATCH(this->rom_file_table.OpenFile(out, path)) {
            R_CONVERT(fs::ResultDbmNotFound,         fs::ResultPathNotFound());
            R_CONVERT(fs::ResultDbmInvalidOperation, fs::ResultPathNotFound());
        } R_END_TRY_CATCH;
        return ResultSuccess();
    }

    IStorage *RomFsFileSystem::GetBaseStorage() {
        return this->base_storage;
    }

    RomFsFileSystem::RomFileTable *RomFsFileSystem::GetRomFileTable() {
        return std::addressof(this->rom_file_table);
    }

    Result RomFsFileSystem::GetFileBaseOffset(s64 *out, const char *path) {
        AMS_ABORT_UNLESS(out != nullptr);
        AMS_ABORT_UNLESS(path != nullptr);

        RomFileTable::FileInfo info;
        R_TRY(this->GetFileInfo(std::addressof(info), path));
        *out = this->entry_size + info.offset.Get();
        return ResultSuccess();
    }

    Result RomFsFileSystem::DoCreateFile(const char *path, s64 size, int flags) {
        return fs::ResultUnsupportedOperationInRomFsFileSystemA();
    }

    Result RomFsFileSystem::DoDeleteFile(const char *path) {
        return fs::ResultUnsupportedOperationInRomFsFileSystemA();
    }

    Result RomFsFileSystem::DoCreateDirectory(const char *path) {
        return fs::ResultUnsupportedOperationInRomFsFileSystemA();
    }

    Result RomFsFileSystem::DoDeleteDirectory(const char *path) {
        return fs::ResultUnsupportedOperationInRomFsFileSystemA();
    }

    Result RomFsFileSystem::DoDeleteDirectoryRecursively(const char *path) {
        return fs::ResultUnsupportedOperationInRomFsFileSystemA();
    }

    Result RomFsFileSystem::DoRenameFile(const char *old_path, const char *new_path) {
        return fs::ResultUnsupportedOperationInRomFsFileSystemA();
    }

    Result RomFsFileSystem::DoRenameDirectory(const char *old_path, const char *new_path) {
        return fs::ResultUnsupportedOperationInRomFsFileSystemA();
    }

    Result RomFsFileSystem::DoGetEntryType(fs::DirectoryEntryType *out, const char *path) {
        RomDirectoryInfo dir_info;
        R_TRY_CATCH(this->rom_file_table.GetDirectoryInformation(std::addressof(dir_info), path)) {
            R_CONVERT(fs::ResultDbmNotFound, fs::ResultPathNotFound())
            R_CATCH(fs::ResultDbmInvalidOperation) {
                RomFileTable::FileInfo file_info;
                R_TRY(this->GetFileInfo(std::addressof(file_info), path));
                *out = fs::DirectoryEntryType_File;
                return ResultSuccess();
            }
        } R_END_TRY_CATCH;

        *out = fs::DirectoryEntryType_Directory;
        return ResultSuccess();
    }

    Result RomFsFileSystem::DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const char *path, fs::OpenMode mode) {
        AMS_ASSERT(out_file != nullptr);
        AMS_ASSERT(path != nullptr);

        R_UNLESS((mode & fs::OpenMode_All) == fs::OpenMode_Read, fs::ResultInvalidOpenMode());

        RomFileTable::FileInfo file_info;
        R_TRY(this->GetFileInfo(std::addressof(file_info), path));

        auto file = std::make_unique<RomFsFile>(this, this->entry_size + file_info.offset.Get(), this->entry_size + file_info.offset.Get() + file_info.size.Get());
        R_UNLESS(file != nullptr, fs::ResultAllocationFailureInRomFsFileSystemB());

        *out_file = std::move(file);
        return ResultSuccess();
    }

    Result RomFsFileSystem::DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const char *path, fs::OpenDirectoryMode mode) {
        AMS_ASSERT(out_dir != nullptr);
        AMS_ASSERT(path != nullptr);

        RomFileTable::FindPosition find;
        R_TRY_CATCH(this->rom_file_table.FindOpen(std::addressof(find), path)) {
            R_CONVERT(fs::ResultDbmNotFound,         fs::ResultPathNotFound())
            R_CONVERT(fs::ResultDbmInvalidOperation, fs::ResultPathNotFound())
        } R_END_TRY_CATCH;

        auto dir = std::make_unique<RomFsDirectory>(this, find, mode);
        R_UNLESS(dir != nullptr, fs::ResultAllocationFailureInRomFsFileSystemC());

        *out_dir = std::move(dir);
        return ResultSuccess();
    }

    Result RomFsFileSystem::DoCommit() {
        return ResultSuccess();
    }

    Result RomFsFileSystem::DoGetFreeSpaceSize(s64 *out, const char *path) {
        *out = 0;
        return ResultSuccess();
    }

    Result RomFsFileSystem::DoGetTotalSpaceSize(s64 *out, const char *path) {
        return fs::ResultUnsupportedOperationInRomFsFileSystemC();
    }

    Result RomFsFileSystem::DoCleanDirectoryRecursively(const char *path) {
        return fs::ResultUnsupportedOperationInRomFsFileSystemA();
    }

    Result RomFsFileSystem::DoCommitProvisionally(s64 counter) {
        return fs::ResultUnsupportedOperationInRomFsFileSystemB();
    }

    Result RomFsFileSystem::DoRollback() {
        return ResultSuccess();
    }
}
