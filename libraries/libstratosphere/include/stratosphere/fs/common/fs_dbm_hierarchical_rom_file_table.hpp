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
#include <stratosphere/fs/common/fs_dbm_rom_types.hpp>
#include <stratosphere/fs/common/fs_dbm_rom_path_tool.hpp>
#include <stratosphere/fs/common/fs_dbm_rom_key_value_storage.hpp>

namespace ams::fs {

    class HierarchicalRomFileTable {
        public:
            using Position = u32;

            struct FindPosition {
                Position next_dir;
                Position next_file;
            };
            static_assert(util::is_pod<FindPosition>::value);

            using DirectoryInfo = RomDirectoryInfo;
            using FileInfo      = RomFileInfo;

            static constexpr RomFileId PositionToFileId(Position pos) {
                return static_cast<RomFileId>(pos);
            }

            static constexpr Position FileIdToPosition(RomFileId id) {
                return static_cast<Position>(id);
            }
        private:
            static constexpr inline Position InvalidPosition = ~Position();
            static constexpr inline Position RootPosition = 0;
            static constexpr inline size_t ReservedDirectoryCount = 1;

            static constexpr RomDirectoryId PositionToDirectoryId(Position pos) {
                return static_cast<RomDirectoryId>(pos);
            }

            static constexpr Position DirectoryIdToPosition(RomDirectoryId id) {
                return static_cast<Position>(id);
            }

            static_assert(std::is_same<RomDirectoryId, RomFileId>::value);

            struct RomDirectoryEntry {
                Position next;
                Position dir;
                Position file;
            };
            static_assert(util::is_pod<RomDirectoryEntry>::value);

            struct RomFileEntry {
                Position next;
                FileInfo info;
            };
            static_assert(util::is_pod<RomFileEntry>::value);

            static constexpr inline u32 MaxKeyLength = RomPathTool::MaxPathLength;

            template<typename ImplKeyType, typename ClientKeyType, typename ValueType>
            class EntryMapTable : public KeyValueRomStorageTemplate<ImplKeyType, ValueType, MaxKeyLength> {
                public:
                    using ImplKey   = ImplKeyType;
                    using ClientKey = ClientKeyType;
                    using Value     = ValueType;
                    using Position  = HierarchicalRomFileTable::Position;
                    using Base      = KeyValueRomStorageTemplate<ImplKeyType, ValueType, MaxKeyLength>;
                public:
                    Result Add(Position *out, const ClientKeyType &key, const Value &value) {
                        return Base::AddInternal(out, key.key, key.Hash(), key.name.path, key.name.length * sizeof(RomPathChar), value);
                    }

                    Result Get(Position *out_pos, Value *out_val, const ClientKeyType &key) {
                        return Base::GetInternal(out_pos, out_val, key.key, key.Hash(), key.name.path, key.name.length * sizeof(RomPathChar));
                    }

                    Result GetByPosition(ImplKey *out_key, Value *out_val, Position pos) {
                        return Base::GetByPosition(out_key, out_val, pos);
                    }

                    Result GetByPosition(ImplKey *out_key, Value *out_val, void *out_aux, size_t *out_aux_size, Position pos) {
                        return Base::GetByPosition(out_key, out_val, out_aux, out_aux_size, pos);
                    }

                    Result SetByPosition(Position pos, const Value &value) {
                        return Base::SetByPosition(pos, value);
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
            static_assert(util::is_pod<RomEntryKey>::value);

            struct EntryKey {
                RomEntryKey key;
                RomPathTool::RomEntryName name;

                constexpr u32 Hash() const {
                    u32 hash = this->key.parent ^ 123456789;
                    const RomPathChar *       name = this->name.path;
                    const RomPathChar * const end  = name + this->name.length;
                    while (name < end) {
                        const u32 cur = static_cast<u32>(static_cast<std::make_unsigned<RomPathChar>::type>(*(name++)));
                        hash = ((hash >> 5) | (hash << 27)) ^ cur;
                    }
                    return hash;
                }
            };
            static_assert(util::is_pod<EntryKey>::value);

            using DirectoryEntryMapTable = EntryMapTable<RomEntryKey, EntryKey, RomDirectoryEntry>;
            using FileEntryMapTable      = EntryMapTable<RomEntryKey, EntryKey, RomFileEntry>;
        private:
            DirectoryEntryMapTable dir_table;
            FileEntryMapTable file_table;
        public:
            static s64 QueryDirectoryEntryBucketStorageSize(s64 count);
            static size_t QueryDirectoryEntrySize(size_t aux_size);
            static s64 QueryFileEntryBucketStorageSize(s64 count);
            static size_t QueryFileEntrySize(size_t aux_size);

            static Result Format(SubStorage dir_bucket, SubStorage file_bucket);
        public:
            HierarchicalRomFileTable();

            constexpr u32 GetDirectoryEntryCount() const {
                return this->dir_table.GetEntryCount();
            }

            constexpr u32 GetFileEntryCount() const {
                return this->file_table.GetEntryCount();
            }

            Result Initialize(SubStorage dir_bucket, SubStorage dir_entry, SubStorage file_bucket, SubStorage file_entry);
            void Finalize();

            Result CreateRootDirectory();
            Result CreateDirectory(RomDirectoryId *out, const RomPathChar *path, const DirectoryInfo &info);
            Result CreateFile(RomFileId *out, const RomPathChar *path, const FileInfo &info);
            Result ConvertPathToDirectoryId(RomDirectoryId *out, const RomPathChar *path);
            Result ConvertPathToFileId(RomFileId *out, const RomPathChar *path);

            Result GetDirectoryInformation(DirectoryInfo *out, const RomPathChar *path);
            Result GetDirectoryInformation(DirectoryInfo *out, RomDirectoryId id);

            Result OpenFile(FileInfo *out, const RomPathChar *path);
            Result OpenFile(FileInfo *out, RomFileId id);

            Result FindOpen(FindPosition *out, const RomPathChar *path);
            Result FindOpen(FindPosition *out, RomDirectoryId id);

            Result FindNextDirectory(RomPathChar *out, FindPosition *find, size_t length);
            Result FindNextFile(RomPathChar *out, FindPosition *find, size_t length);

            Result QueryRomFileSystemSize(s64 *out_dir_entry_size, s64 *out_file_entry_size);
        private:
            Result GetGrandParent(Position *out_pos, EntryKey *out_dir_key, RomDirectoryEntry *out_dir_entry, Position pos, RomPathTool::RomEntryName name, const RomPathChar *path);

            Result FindParentDirectoryRecursive(Position *out_pos, EntryKey *out_dir_key, RomDirectoryEntry *out_dir_entry, RomPathTool::PathParser *parser, const RomPathChar *path);

            Result FindPathRecursive(EntryKey *out_key, RomDirectoryEntry *out_dir_entry, bool is_dir, const RomPathChar *path);
            Result FindDirectoryRecursive(EntryKey *out_key, RomDirectoryEntry *out_dir_entry, const RomPathChar *path);
            Result FindFileRecursive(EntryKey *out_key, RomDirectoryEntry *out_dir_entry, const RomPathChar *path);

            Result CheckSameEntryExists(const EntryKey &key, Result if_exists);

            Result GetDirectoryEntry(Position *out_pos, RomDirectoryEntry *out_entry, const EntryKey &key);
            Result GetDirectoryEntry(RomDirectoryEntry *out_entry, RomDirectoryId id);

            Result GetFileEntry(Position *out_pos, RomFileEntry *out_entry, const EntryKey &key);
            Result GetFileEntry(RomFileEntry *out_entry, RomFileId id);

            Result GetDirectoryInformation(DirectoryInfo *out, const EntryKey &key);

            Result OpenFile(FileInfo *out, const EntryKey &key);

            Result FindOpen(FindPosition *out, const EntryKey &key);
    };

}
