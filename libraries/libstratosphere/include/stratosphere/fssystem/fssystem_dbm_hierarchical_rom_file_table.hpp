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
#include <vapours.hpp>
#include <stratosphere/fssystem/fssystem_dbm_rom_types.hpp>
#include <stratosphere/fssystem/fssystem_dbm_rom_path_tool.hpp>
#include <stratosphere/fssystem/fssystem_dbm_rom_key_value_storage.hpp>

namespace ams::fssystem {

    template<typename DBS, typename DES, typename FBS, typename FES>
    class HierarchicalRomFileTable {
        private:
            using DirectoryBucketStorage = DBS;
            using DirectoryEntryStorage  = DES;
            using FileBucketStorage      = FBS;
            using FileEntryStorage       = FES;
        public:
            using Position = u32;

            struct FindPosition {
                Position next_dir;
                Position next_file;
            };
            static_assert(std::is_pod<FindPosition>::value);

            using DirectoryInfo = RomDirectoryInfo;
            using FileInfo      = RomFileInfo;

            static constexpr RomFileId ConvertToFileId(Position pos) {
                return static_cast<RomFileId>(pos);
            }
        private:
            static constexpr inline Position InvalidPosition = ~Position();
            static constexpr inline Position RootPosition = 0;
            static constexpr inline size_t ReservedDirectoryCount = 1;

            static constexpr RomDirectoryId ConvertToDirectoryId(Position pos) {
                return static_cast<RomDirectoryId>(pos);
            }

            static constexpr Position ConvertToPosition(RomDirectoryId id) {
                return static_cast<Position>(id);
            }

            static_assert(std::is_same<RomDirectoryId, RomFileId>::value);

            struct RomDirectoryEntry {
                Position next;
                Position dir;
                Position file;
            };
            static_assert(std::is_pod<RomDirectoryEntry>::value);

            struct RomFileEntry {
                Position next;
                FileInfo info;
            };
            static_assert(std::is_pod<RomFileEntry>::value);

            static constexpr inline u32 MaxKeyLength = RomPathTool::MaxPathLength;

            template<typename BucketStorageType, typename EntryStorageType, typename ImplKeyType, typename ClientKeyType, typename ValueType>
            class EntryMapTable : public RomKeyValueStorage<BucketStorageType, EntryStorageType, ImplKeyType, ValueType, MaxKeyLength> {
                private:
                    using BucketStorage = BucketStorageType;
                    using EntryStorage  = EntryStorageType;
                public:
                    using ImplKey   = ImplKeyType;
                    using ClientKey = ClientKeyType;
                    using Value     = ValueType;
                    using Position  = HierarchicalRomFileTable::Position;
                    using Base      = RomKeyValueStorage<BucketStorageType, EntryStorageType, ImplKeyType, ValueType, MaxKeyLength>;
                public:
                    Result Add(Position *out, const ClientKeyType &key, const Value &value) {
                        return Base::AddImpl(out, key.key, key.Hash(), key.name.path, key.name.length * sizeof(RomPathChar), value);
                    }

                    Result Get(Position *out_pos, Value *out_val, const ClientKeyType &key) const {
                        return Base::GetImpl(out_pos, out_val, key.key, key.Hash(), key.name.path, key.name.length * sizeof(RomPathChar));
                    }

                    Result GetByPosition(ImplKey *out_key, Value *out_val, void *out_aux, size_t *out_aux_size, Position pos) const {
                        return Base::GetByPosition(out_key, out_val, out_aux, out_aux_size, pos);
                    }

                    Result SetByPosition(Position pos, const Value &value, fs::WriteOption option) const {
                        return Base::SetByPosition(pos, value, option);
                    }
            };

            struct RomEntryKey {
                Position parent;

                bool IsEqual(const RomEntryKey &rhs, const void *aux_lhs, size_t aux_lhs_size, const void *aux_rhs, size_t aux_rhs_size) const {
                    if (this->parent != rhs.parent) {
                        return false;
                    }
                    if (aux_lhs_size != aux_rhs_size) {
                        return false;
                    }
                    return RomPathTool::IsEqualPath(reinterpret_cast<const RomPathChar *>(aux_lhs), reinterpret_cast<const RomPathChar *>(aux_rhs), aux_lhs_size / sizeof(RomPathChar));
                }
            };
            static_assert(std::is_pod<RomEntryKey>::value);

            struct EntryKey {
                RomEntryKey key;
                RomPathTool::RomEntryName name;

                constexpr u32 Hash() const {
                    u32 hash = this->key.parent ^ 123456789;
                    const RomPathChar *name = this->name.path;
                    const RomPathChar *end  = name + this->name.length;
                    while (name < end) {
                        const u32 cur = static_cast<u32>(static_cast<std::make_unsigned<RomPathChar>::type>(*(name++)));
                        hash = ((hash >> 5) | (hash << 27)) ^ cur;
                    }
                    return hash;
                }
            };
            static_assert(std::is_pod<EntryKey>::value);

            using DirectoryEntryMapTable = EntryMapTable<DirectoryBucketStorage, DirectoryEntryStorage, RomEntryKey, EntryKey, RomDirectoryEntry>;
            using FileEntryMapTable      = EntryMapTable<FileBucketStorage, FileEntryStorage, RomEntryKey, EntryKey, RomFileEntry>;
        private:
            DirectoryEntryMapTable dir_table;
            FileEntryMapTable file_table;
        public:
            static u32 QueryDirectoryEntrySize(u32 name_len) {
                AMS_ABORT_UNLESS(name_len <= RomPathTool::MaxPathLength);
                return DirectoryEntryMapTable::QueryEntrySize(name_len * sizeof(RomPathChar));
            }

            static u32 QueryFileEntrySize(u32 name_len) {
                AMS_ABORT_UNLESS(name_len <= RomPathTool::MaxPathLength);
                return FileEntryMapTable::QueryEntrySize(name_len * sizeof(RomPathChar));
            }

            static u32 QueryDirectoryEntryBucketStorageSize(u32 count) { return DirectoryEntryMapTable::QueryBucketStorageSize(count); }
            static u32 QueryFileEntryBucketStorageSize(u32 count) { return FileEntryMapTable::QueryBucketStorageSize(count); }

            static Result Format(DirectoryBucketStorage *dir_bucket, s64 dir_bucket_ofs, u32 dir_bucket_size, DirectoryEntryStorage *dir_entry, s64 dir_entry_ofs, u32 dir_entry_size, FileBucketStorage *file_bucket, s64 file_bucket_ofs, u32 file_bucket_size, FileEntryStorage *file_entry, s64 file_entry_ofs, u32 file_entry_size) {
                R_TRY(DirectoryEntryMapTable::Format(dir_bucket, dir_bucket_ofs, DirectoryEntryMapTable::QueryBucketCount(dir_bucket_size), dir_entry, dir_entry_ofs, dir_entry_size));
                R_TRY(FileEntryMapTable::Format(file_bucket, file_bucket_ofs, FileEntryMapTable::QueryBucketCount(file_bucket_size), file_entry, file_entry_ofs, file_entry_size));
                return ResultSuccess();
            }
        public:
            HierarchicalRomFileTable() { /* ... */ }

            constexpr u32 GetDirectoryEntryCount() const {
                return this->dir_table.GetEntryCount();
            }

            constexpr u32 GetFileEntryCount() const {
                return this->file_table.GetEntryCount();
            }

            Result Initialize(DirectoryBucketStorage *dir_bucket, s64 dir_bucket_ofs, u32 dir_bucket_size, DirectoryEntryStorage *dir_entry, s64 dir_entry_ofs, u32 dir_entry_size, FileBucketStorage *file_bucket, s64 file_bucket_ofs, u32 file_bucket_size, FileEntryStorage *file_entry, s64 file_entry_ofs, u32 file_entry_size) {
                AMS_ASSERT(dir_bucket  != nullptr);
                AMS_ASSERT(dir_entry   != nullptr);
                AMS_ASSERT(file_bucket != nullptr);
                AMS_ASSERT(file_entry  != nullptr);

                R_TRY(this->dir_table.Initialize(dir_bucket, dir_bucket_ofs, DirectoryEntryMapTable::QueryBucketCount(dir_bucket_size), dir_entry, dir_entry_ofs, dir_entry_size));
                R_TRY(this->file_table.Initialize(file_bucket, file_bucket_ofs, FileEntryMapTable::QueryBucketCount(file_bucket_size), file_entry, file_entry_ofs, file_entry_size));

                return ResultSuccess();
            }

            void Finalize() {
                this->dir_table.Finalize();
                this->file_table.Finalize();
            }

            Result CreateRootDirectory() {
                Position root_pos = RootPosition;
                EntryKey root_key = {};
                root_key.key.parent = root_pos;
                RomPathTool::InitializeRomEntryName(std::addressof(root_key.name));
                RomDirectoryEntry root_entry = {
                    .next = InvalidPosition,
                    .dir  = InvalidPosition,
                    .file = InvalidPosition,
                };
                return this->dir_table.Add(std::addressof(root_pos), root_key, root_entry);
            }

            Result CreateDirectory(RomDirectoryId *out, const RomPathChar *path, const DirectoryInfo &info) {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(path != nullptr);

                EntryKey parent_key = {};
                RomDirectoryEntry parent_entry = {};
                EntryKey new_key = {};
                R_TRY(this->FindDirectoryRecursive(std::addressof(parent_key), std::addressof(parent_entry), std::addressof(new_key), path));

                R_TRY(this->CheckSameEntryExists(new_key, fs::ResultDbmAlreadyExists()));

                RomDirectoryEntry new_entry = {
                    .next = InvalidPosition,
                    .dir  = InvalidPosition,
                    .file = InvalidPosition,
                };

                Position new_pos = 0;
                R_TRY_CATCH(this->dir_table.Add(std::addressof(new_pos), new_key, new_entry)) {
                    R_CONVERT(fs::ResultDbmKeyFull, fs::ResultDbmDirectoryEntryFull())
                } R_END_TRY_CATCH;

                *out = ConvertToDirectoryId(new_pos);

                if (parent_entry.dir == InvalidPosition) {
                    parent_entry.dir = new_pos;

                    R_TRY(this->dir_table.SetByPosition(new_key.key.parent, parent_entry, fs::WriteOption::None));
                } else {
                    Position cur_pos = parent_entry.dir;
                    while (true) {
                        RomEntryKey cur_key = {};
                        RomDirectoryEntry cur_entry = {};
                        R_TRY(this->dir_table.GetByPosition(std::addressof(cur_key), std::addressof(cur_entry), nullptr, nullptr, cur_pos));

                        if (cur_entry.next == InvalidPosition) {
                            cur_entry.next = new_pos;

                            R_TRY(this->dir_table.SetByPosition(cur_pos, cur_entry, fs::WriteOption::None));
                            break;
                        }

                        cur_pos = cur_entry.next;
                    }
                }

                return ResultSuccess();
            }

            Result CreateFile(RomFileId *out, const RomPathChar *path, const FileInfo &info) {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(path != nullptr);

                EntryKey parent_key = {};
                RomDirectoryEntry parent_entry = {};
                EntryKey new_key = {};
                R_TRY(this->FindFileRecursive(std::addressof(parent_key), std::addressof(parent_entry), std::addressof(new_key), path));

                R_TRY(this->CheckSameEntryExists(new_key, fs::ResultDbmAlreadyExists()));

                RomFileEntry new_entry = {
                    .next = InvalidPosition,
                    .info = info,
                };

                Position new_pos = 0;
                R_TRY_CATCH(this->file_table.Add(std::addressof(new_pos), new_key, new_entry)) {
                    R_CONVERT(fs::ResultDbmKeyFull, fs::ResultDbmFileEntryFull())
                } R_END_TRY_CATCH;

                *out = ConvertToFileId(new_pos);

                if (parent_entry.file == InvalidPosition) {
                    parent_entry.file = new_pos;

                    R_TRY(this->dir_table.SetByPosition(new_key.key.parent, parent_entry, fs::WriteOption::None));
                } else {
                    Position cur_pos = parent_entry.file;
                    while (true) {
                        RomEntryKey cur_key = {};
                        RomFileEntry cur_entry = {};
                        R_TRY(this->file_table.GetByPosition(std::addressof(cur_key), std::addressof(cur_entry), nullptr, nullptr, cur_pos));

                        if (cur_entry.next == InvalidPosition) {
                            cur_entry.next = new_pos;

                            R_TRY(this->file_table.SetByPosition(cur_pos, cur_entry, fs::WriteOption::None));
                            break;
                        }

                        cur_pos = cur_entry.next;
                    }
                }

                return ResultSuccess();
            }

            Result ConvertPathToDirectoryId(RomDirectoryId *out, const RomPathChar *path) const {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(path != nullptr);

                EntryKey parent_key = {};
                RomDirectoryEntry parent_entry = {};
                EntryKey key = {};
                R_TRY(this->FindDirectoryRecursive(std::addressof(parent_key), std::addressof(parent_entry), std::addressof(key), path));

                Position pos = 0;
                RomDirectoryEntry entry = {};
                R_TRY(this->GetDirectoryEntry(std::addressof(pos), std::addressof(entry), key));

                *out = ConvertToDirectoryId(pos);
                return ResultSuccess();
            }

            Result ConvertPathToFileId(RomFileId *out, const RomPathChar *path) const {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(path != nullptr);

                EntryKey parent_key = {};
                RomDirectoryEntry parent_entry = {};
                EntryKey key = {};
                R_TRY(this->FindDirectoryRecursive(std::addressof(parent_key), std::addressof(parent_entry), std::addressof(key), path));

                Position pos = 0;
                RomFileEntry entry = {};
                R_TRY(this->GetFileEntry(std::addressof(pos), std::addressof(entry), key));

                *out = ConvertToFileId(pos);
                return ResultSuccess();
            }

            Result GetDirectoryInformation(DirectoryInfo *out, const RomPathChar *path) const {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(path != nullptr);

                EntryKey parent_key = {};
                RomDirectoryEntry parent_entry = {};
                EntryKey key = {};
                R_TRY(this->FindDirectoryRecursive(std::addressof(parent_key), std::addressof(parent_entry), std::addressof(key), path));

                return this->GetDirectoryInformation(out, key);
            }

            Result GetDirectoryInformation(DirectoryInfo *out, RomDirectoryId id) const {
                AMS_ASSERT(out != nullptr);

                RomDirectoryEntry entry = {};
                R_TRY(this->GetDirectoryEntry(std::addressof(entry), id));

                return ResultSuccess();
            }

            Result OpenFile(FileInfo *out, const RomPathChar *path) const {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(path != nullptr);

                EntryKey parent_key = {};
                RomDirectoryEntry parent_entry = {};
                EntryKey key = {};
                R_TRY(this->FindFileRecursive(std::addressof(parent_key), std::addressof(parent_entry), std::addressof(key), path));

                return this->OpenFile(out, key);
            }

            Result OpenFile(FileInfo *out, RomFileId id) const {
                AMS_ASSERT(out != nullptr);

                RomFileEntry entry = {};
                R_TRY(this->GetFileEntry(std::addressof(entry), id));

                *out = entry.info;
                return ResultSuccess();
            }

            Result FindOpen(FindPosition *out, const RomPathChar *path) const {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(path != nullptr);

                EntryKey parent_key = {};
                RomDirectoryEntry parent_entry = {};
                EntryKey key = {};
                R_TRY(this->FindDirectoryRecursive(std::addressof(parent_key), std::addressof(parent_entry), std::addressof(key), path));

                return this->FindOpen(out, key);
            }

            Result FindOpen(FindPosition *out, RomDirectoryId id) const {
                AMS_ASSERT(out != nullptr);

                out->next_dir  = InvalidPosition;
                out->next_file = InvalidPosition;

                RomDirectoryEntry entry = {};
                R_TRY(this->GetDirectoryEntry(std::addressof(entry), id));

                out->next_dir  = entry.dir;
                out->next_file = entry.file;

                return ResultSuccess();
            }

            Result FindNextDirectory(RomPathChar *out, FindPosition *find) const {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(find != nullptr);

                R_UNLESS(find->next_dir != InvalidPosition, fs::ResultDbmFindFinished());

                RomEntryKey key = {};
                RomDirectoryEntry entry = {};
                size_t aux_size = 0;
                R_TRY(this->dir_table.GetByPosition(std::addressof(key), std::addressof(entry), out, std::addressof(aux_size), find->next_dir));

                out[aux_size / sizeof(RomPathChar)] = RomStringTraits::NullTerminator;

                find->next_dir = entry.next;
                return ResultSuccess();
            }

            Result FindNextFile(RomPathChar *out, FindPosition *find) const {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(find != nullptr);

                R_UNLESS(find->next_file != InvalidPosition, fs::ResultDbmFindFinished());

                RomEntryKey key = {};
                RomFileEntry entry = {};
                size_t aux_size = 0;
                R_TRY(this->file_table.GetByPosition(std::addressof(key), std::addressof(entry), out, std::addressof(aux_size), find->next_file));

                out[aux_size / sizeof(RomPathChar)] = RomStringTraits::NullTerminator;

                find->next_file = entry.next;
                return ResultSuccess();
            }

            Result QueryRomFileSystemSize(u32 *out_dir_entry_size, u32 *out_file_entry_size) {
                AMS_ASSERT(out_dir_entry_size != nullptr);
                AMS_ASSERT(out_file_entry_size != nullptr);

                *out_dir_entry_size = this->dir_table.GetTotalEntrySize();
                *out_file_entry_size = this->file_table.GetTotalEntrySize();
                return ResultSuccess();
            }
        private:
            Result GetGrandParent(Position *out_pos, EntryKey *out_dir_key, RomDirectoryEntry *out_dir_entry, Position pos, RomPathTool::RomEntryName name, const RomPathChar *path) const {
                AMS_ASSERT(out_pos != nullptr);
                AMS_ASSERT(out_dir_key != nullptr);
                AMS_ASSERT(out_dir_entry != nullptr);

                RomEntryKey gp_key = {};
                RomDirectoryEntry gp_entry = {};
                R_TRY(this->dir_table.GetByPosition(std::addressof(gp_key), std::addressof(gp_entry), nullptr, nullptr, pos));
                out_dir_key->key.parent = gp_key.parent;

                R_TRY(RomPathTool::GetParentDirectoryName(std::addressof(out_dir_key->name), name, path));

                R_TRY(this->GetDirectoryEntry(out_pos, out_dir_entry, *out_dir_key));

                return ResultSuccess();
            }

            Result FindParentDirectoryRecursive(Position *out_pos, EntryKey *out_dir_key, RomDirectoryEntry *out_dir_entry, RomPathTool::PathParser &parser, const RomPathChar *path) const {
                AMS_ASSERT(out_pos != nullptr);
                AMS_ASSERT(out_dir_key != nullptr);
                AMS_ASSERT(out_dir_entry != nullptr);

                Position dir_pos = RootPosition;
                EntryKey dir_key = {};
                RomDirectoryEntry dir_entry = {};
                dir_key.key.parent = RootPosition;

                R_TRY(parser.GetNextDirectoryName(std::addressof(dir_key.name)));
                R_TRY(this->GetDirectoryEntry(std::addressof(dir_pos), std::addressof(dir_entry), dir_key));

                Position parent_pos = dir_pos;
                while (!parser.IsFinished()) {
                    EntryKey old_key = dir_key;

                    R_TRY(parser.GetNextDirectoryName(std::addressof(dir_key.name)));

                    if (RomPathTool::IsCurrentDirectory(dir_key.name)) {
                        dir_key = old_key;
                        continue;
                    } else if (RomPathTool::IsParentDirectory(dir_key.name)) {
                        R_UNLESS(parent_pos != RootPosition, fs::ResultDbmInvalidOperation());

                        R_TRY(this->GetGrandParent(std::addressof(parent_pos), std::addressof(dir_key), std::addressof(dir_entry), dir_key.key.parent, dir_key.name, path));
                    } else {
                        dir_key.key.parent = parent_pos;
                        R_TRY(this->GetDirectoryEntry(std::addressof(dir_pos), std::addressof(dir_entry), dir_key));

                        parent_pos = dir_pos;
                    }
                }

                *out_pos = parent_pos;
                *out_dir_key = dir_key;
                *out_dir_entry = dir_entry;
                return ResultSuccess();
            }

            Result FindPathRecursive(EntryKey *out_parent_key, RomDirectoryEntry *out_parent_dir_entry, EntryKey *out_key, bool is_dir, const RomPathChar *path) const {
                AMS_ASSERT(out_parent_key != nullptr);
                AMS_ASSERT(out_parent_dir_entry != nullptr);
                AMS_ASSERT(out_key != nullptr);
                AMS_ASSERT(path != nullptr);

                RomPathTool::PathParser parser;
                R_TRY(parser.Initialize(path));

                Position parent_pos = 0;
                R_TRY(this->FindParentDirectoryRecursive(std::addressof(parent_pos), out_parent_key, out_parent_dir_entry, parser, path));

                if (is_dir) {
                    RomPathTool::RomEntryName name = {};
                    R_TRY(parser.GetAsDirectoryName(std::addressof(name)));

                    if (RomPathTool::IsCurrentDirectory(name)) {
                        *out_key = *out_parent_key;
                        if (out_key->key.parent != RootPosition) {
                            Position pos = 0;
                            R_TRY(this->GetGrandParent(std::addressof(pos), out_parent_key, out_parent_dir_entry, out_key->key.parent, out_key->name, path));
                        }
                    } else if (RomPathTool::IsParentDirectory(name)) {
                        R_UNLESS(parent_pos != RootPosition, fs::ResultDbmInvalidOperation());

                        Position pos = 0;
                        RomDirectoryEntry cur_entry = {};
                        R_TRY(this->GetGrandParent(std::addressof(pos), out_key, std::addressof(cur_entry), out_parent_key->key.parent, out_parent_key->name, path));

                        if (out_key->key.parent != RootPosition) {
                            R_TRY(this->GetGrandParent(std::addressof(pos), out_parent_key, out_parent_dir_entry, out_key->key.parent, out_key->name, path));
                        }
                    } else {
                        out_key->name = name;
                        out_key->key.parent = (out_key->name.length > 0) ? parent_pos : RootPosition;
                    }
                } else {
                    R_UNLESS(!parser.IsDirectoryPath(), fs::ResultDbmInvalidOperation());

                    out_key->key.parent = parent_pos;
                    R_TRY(parser.GetAsFileName(std::addressof(out_key->name)));
                }

                return ResultSuccess();
            }

            Result FindDirectoryRecursive(EntryKey *out_parent_key, RomDirectoryEntry *out_parent_dir_entry, EntryKey *out_key, const RomPathChar *path) const {
                return this->FindPathRecursive(out_parent_key, out_parent_dir_entry, out_key, true, path);
            }

            Result FindFileRecursive(EntryKey *out_parent_key, RomDirectoryEntry *out_parent_dir_entry, EntryKey *out_key, const RomPathChar *path) const {
                return this->FindPathRecursive(out_parent_key, out_parent_dir_entry, out_key, false, path);
            }

            Result CheckSameEntryExists(const EntryKey &key, Result if_exists) const {
                /* Check dir */
                {
                    Position pos = InvalidPosition;
                    RomDirectoryEntry entry = {};
                    const Result get_res = this->dir_table.Get(std::addressof(pos), std::addressof(entry), key);
                    if (!fs::ResultDbmKeyNotFound::Includes(get_res)) {
                        R_TRY(get_res);
                        return if_exists;
                    }
                }

                /* Check file */
                {
                    Position pos = InvalidPosition;
                    RomFileEntry entry = {};
                    const Result get_res = this->file_table.Get(std::addressof(pos), std::addressof(entry), key);
                    if (!fs::ResultDbmKeyNotFound::Includes(get_res)) {
                        R_TRY(get_res);
                        return if_exists;
                    }
                }
                return ResultSuccess();
            }

            Result GetDirectoryEntry(Position *out_pos, RomDirectoryEntry *out_entry, const EntryKey &key) const {
                AMS_ASSERT(out_pos != nullptr);
                AMS_ASSERT(out_entry != nullptr);

                const Result dir_res = this->dir_table.Get(out_pos, out_entry, key);
                R_UNLESS(R_FAILED(dir_res),                            dir_res);
                R_UNLESS(!fs::ResultDbmKeyNotFound::Includes(dir_res), dir_res);

                Position pos = 0;
                RomFileEntry entry = {};
                const Result file_res = this->file_table.Get(std::addressof(pos), std::addressof(entry), key);
                R_UNLESS(R_FAILED(file_res),                            fs::ResultDbmInvalidOperation());
                R_UNLESS(!fs::ResultDbmKeyNotFound::Includes(file_res), fs::ResultDbmDirectoryNotFound());
                return file_res;
            }

            Result GetDirectoryEntry(RomDirectoryEntry *out_entry, RomDirectoryId id) const {
                AMS_ASSERT(out_entry != nullptr);
                Position pos = ConvertToPosition(id);

                RomEntryKey key = {};
                const Result dir_res = this->dir_table.GetByPosition(std::addressof(key), out_entry, nullptr, nullptr, pos);
                R_UNLESS(R_FAILED(dir_res),                            dir_res);
                R_UNLESS(!fs::ResultDbmKeyNotFound::Includes(dir_res), dir_res);

                RomFileEntry entry = {};
                const Result file_res = this->file_table.GetByPosition(std::addressof(key), std::addressof(entry), nullptr, nullptr, pos);
                R_UNLESS(R_FAILED(file_res),                            fs::ResultDbmInvalidOperation());
                R_UNLESS(!fs::ResultDbmKeyNotFound::Includes(file_res), fs::ResultDbmDirectoryNotFound());
                return file_res;
            }

            Result GetFileEntry(Position *out_pos, RomFileEntry *out_entry, const EntryKey &key) const {
                AMS_ASSERT(out_pos != nullptr);
                AMS_ASSERT(out_entry != nullptr);

                const Result file_res = this->file_table.Get(out_pos, out_entry, key);
                R_UNLESS(R_FAILED(file_res),                            file_res);
                R_UNLESS(!fs::ResultDbmKeyNotFound::Includes(file_res), file_res);

                Position pos = 0;
                RomDirectoryEntry entry = {};
                const Result dir_res = this->dir_table.Get(std::addressof(pos), std::addressof(entry), key);
                R_UNLESS(R_FAILED(dir_res),                            fs::ResultDbmInvalidOperation());
                R_UNLESS(!fs::ResultDbmKeyNotFound::Includes(dir_res), fs::ResultDbmFileNotFound());
                return dir_res;
            }

            Result GetFileEntry(RomFileEntry *out_entry, RomFileId id) const {
                AMS_ASSERT(out_entry != nullptr);
                Position pos = ConvertToPosition(id);

                RomEntryKey key = {};
                const Result file_res = this->file_table.GetByPosition(std::addressof(key), out_entry, nullptr, nullptr, pos);
                R_UNLESS(R_FAILED(file_res),                            file_res);
                R_UNLESS(!fs::ResultDbmKeyNotFound::Includes(file_res), file_res);

                RomDirectoryEntry entry = {};
                const Result dir_res = this->dir_table.GetByPosition(std::addressof(key), std::addressof(entry), nullptr, nullptr, pos);
                R_UNLESS(R_FAILED(dir_res),                            fs::ResultDbmInvalidOperation());
                R_UNLESS(!fs::ResultDbmKeyNotFound::Includes(dir_res), fs::ResultDbmFileNotFound());
                return dir_res;
            }

            Result GetDirectoryInformation(DirectoryInfo *out, const EntryKey &key) const {
                AMS_ASSERT(out != nullptr);

                Position pos = 0;
                RomDirectoryEntry entry = {};
                R_TRY(this->GetDirectoryEntry(std::addressof(pos), std::addressof(entry), key));

                return ResultSuccess();
            }

            Result OpenFile(FileInfo *out, const EntryKey &key) const {
                AMS_ASSERT(out != nullptr);

                Position pos = 0;
                RomFileEntry entry = {};
                R_TRY(this->GetFileEntry(std::addressof(pos), std::addressof(entry), key));

                *out = entry.info;
                return ResultSuccess();
            }

            Result FindOpen(FindPosition *out, const EntryKey &key) const {
                AMS_ASSERT(out != nullptr);

                out->next_dir  = InvalidPosition;
                out->next_file = InvalidPosition;

                Position pos = 0;
                RomDirectoryEntry entry = {};
                R_TRY(this->GetDirectoryEntry(std::addressof(pos), std::addressof(entry), key));

                out->next_dir  = entry.dir;
                out->next_file = entry.file;

                return ResultSuccess();
            }
    };

}
