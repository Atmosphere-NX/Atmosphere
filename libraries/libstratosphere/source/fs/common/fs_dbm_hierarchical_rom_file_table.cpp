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

    s64 HierarchicalRomFileTable::QueryDirectoryEntryBucketStorageSize(s64 count) {
        return DirectoryEntryMapTable::QueryBucketStorageSize(count);
    }

    size_t HierarchicalRomFileTable::QueryDirectoryEntrySize(size_t aux_size) {
        return DirectoryEntryMapTable::QueryEntrySize(aux_size);
    }

    s64 HierarchicalRomFileTable::QueryFileEntryBucketStorageSize(s64 count) {
        return FileEntryMapTable::QueryBucketStorageSize(count);
    }

    size_t HierarchicalRomFileTable::QueryFileEntrySize(size_t aux_size) {
        return FileEntryMapTable::QueryEntrySize(aux_size);
    }

    Result HierarchicalRomFileTable::Format(SubStorage dir_bucket, SubStorage file_bucket) {
        s64 dir_bucket_size;
        R_TRY(dir_bucket.GetSize(std::addressof(dir_bucket_size)));
        R_TRY(DirectoryEntryMapTable::Format(dir_bucket, DirectoryEntryMapTable::QueryBucketCount(dir_bucket_size)));

        s64 file_bucket_size;
        R_TRY(file_bucket.GetSize(std::addressof(file_bucket_size)));
        R_TRY(FileEntryMapTable::Format(file_bucket, FileEntryMapTable::QueryBucketCount(file_bucket_size)));

        return ResultSuccess();
    }

    HierarchicalRomFileTable::HierarchicalRomFileTable() { /* ... */ }

    Result HierarchicalRomFileTable::Initialize(SubStorage dir_bucket, SubStorage dir_entry, SubStorage file_bucket, SubStorage file_entry) {
        s64 dir_bucket_size;
        R_TRY(dir_bucket.GetSize(std::addressof(dir_bucket_size)));
        R_TRY(this->dir_table.Initialize(dir_bucket, DirectoryEntryMapTable::QueryBucketCount(dir_bucket_size), dir_entry));

        s64 file_bucket_size;
        R_TRY(file_bucket.GetSize(std::addressof(file_bucket_size)));
        R_TRY(this->file_table.Initialize(file_bucket, FileEntryMapTable::QueryBucketCount(file_bucket_size), file_entry));

        return ResultSuccess();
    }

    void HierarchicalRomFileTable::Finalize() {
        this->dir_table.Finalize();
        this->file_table.Finalize();
    }

    Result HierarchicalRomFileTable::CreateRootDirectory() {
        Position root_pos = RootPosition;
        EntryKey root_key = {};
        root_key.key.parent = root_pos;
        RomPathTool::InitEntryName(std::addressof(root_key.name));
        RomDirectoryEntry root_entry = {
            .next = InvalidPosition,
            .dir  = InvalidPosition,
            .file = InvalidPosition,
        };
        return this->dir_table.Add(std::addressof(root_pos), root_key, root_entry);
    }

    Result HierarchicalRomFileTable::CreateDirectory(RomDirectoryId *out, const RomPathChar *path, const DirectoryInfo &info) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(path != nullptr);

        RomDirectoryEntry parent_entry = {};
        EntryKey new_key = {};
        R_TRY(this->FindDirectoryRecursive(std::addressof(new_key), std::addressof(parent_entry), path));

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

        *out = PositionToDirectoryId(new_pos);

        if (parent_entry.dir == InvalidPosition) {
            parent_entry.dir = new_pos;

            R_TRY(this->dir_table.SetByPosition(new_key.key.parent, parent_entry));
        } else {
            Position cur_pos = parent_entry.dir;
            while (true) {
                RomEntryKey cur_key = {};
                RomDirectoryEntry cur_entry = {};
                R_TRY(this->dir_table.GetByPosition(std::addressof(cur_key), std::addressof(cur_entry), cur_pos));

                if (cur_entry.next == InvalidPosition) {
                    cur_entry.next = new_pos;

                    R_TRY(this->dir_table.SetByPosition(cur_pos, cur_entry));
                    break;
                }

                cur_pos = cur_entry.next;
            }
        }

        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::CreateFile(RomFileId *out, const RomPathChar *path, const FileInfo &info) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(path != nullptr);

        RomDirectoryEntry parent_entry = {};
        EntryKey new_key = {};
        R_TRY(this->FindFileRecursive(std::addressof(new_key), std::addressof(parent_entry), path));

        R_TRY(this->CheckSameEntryExists(new_key, fs::ResultDbmAlreadyExists()));

        RomFileEntry new_entry = {
            .next = InvalidPosition,
            .info = info,
        };

        Position new_pos = 0;
        R_TRY_CATCH(this->file_table.Add(std::addressof(new_pos), new_key, new_entry)) {
            R_CONVERT(fs::ResultDbmKeyFull, fs::ResultDbmFileEntryFull())
        } R_END_TRY_CATCH;

        *out = PositionToFileId(new_pos);

        if (parent_entry.file == InvalidPosition) {
            parent_entry.file = new_pos;

            R_TRY(this->dir_table.SetByPosition(new_key.key.parent, parent_entry));
        } else {
            Position cur_pos = parent_entry.file;
            while (true) {
                RomEntryKey cur_key = {};
                RomFileEntry cur_entry = {};
                R_TRY(this->file_table.GetByPosition(std::addressof(cur_key), std::addressof(cur_entry), cur_pos));

                if (cur_entry.next == InvalidPosition) {
                    cur_entry.next = new_pos;

                    R_TRY(this->file_table.SetByPosition(cur_pos, cur_entry));
                    break;
                }

                cur_pos = cur_entry.next;
            }
        }

        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::ConvertPathToDirectoryId(RomDirectoryId *out, const RomPathChar *path) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(path != nullptr);

        RomDirectoryEntry parent_entry = {};
        EntryKey key = {};
        R_TRY(this->FindDirectoryRecursive(std::addressof(key), std::addressof(parent_entry), path));

        Position pos = 0;
        RomDirectoryEntry entry = {};
        R_TRY(this->GetDirectoryEntry(std::addressof(pos), std::addressof(entry), key));

        *out = PositionToDirectoryId(pos);
        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::ConvertPathToFileId(RomFileId *out, const RomPathChar *path) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(path != nullptr);

        RomDirectoryEntry parent_entry = {};
        EntryKey key = {};
        R_TRY(this->FindDirectoryRecursive(std::addressof(key), std::addressof(parent_entry), path));

        Position pos = 0;
        RomFileEntry entry = {};
        R_TRY(this->GetFileEntry(std::addressof(pos), std::addressof(entry), key));

        *out = PositionToFileId(pos);
        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::GetDirectoryInformation(DirectoryInfo *out, const RomPathChar *path) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(path != nullptr);

        RomDirectoryEntry parent_entry = {};
        EntryKey key = {};
        R_TRY(this->FindDirectoryRecursive(std::addressof(key), std::addressof(parent_entry), path));

        return this->GetDirectoryInformation(out, key);
    }

    Result HierarchicalRomFileTable::GetDirectoryInformation(DirectoryInfo *out, RomDirectoryId id) {
        AMS_ASSERT(out != nullptr);

        RomDirectoryEntry entry = {};
        R_TRY(this->GetDirectoryEntry(std::addressof(entry), id));

        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::OpenFile(FileInfo *out, const RomPathChar *path) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(path != nullptr);

        RomDirectoryEntry parent_entry = {};
        EntryKey key = {};
        R_TRY(this->FindFileRecursive(std::addressof(key), std::addressof(parent_entry), path));

        return this->OpenFile(out, key);
    }

    Result HierarchicalRomFileTable::OpenFile(FileInfo *out, RomFileId id) {
        AMS_ASSERT(out != nullptr);

        RomFileEntry entry = {};
        R_TRY(this->GetFileEntry(std::addressof(entry), id));

        *out = entry.info;
        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::FindOpen(FindPosition *out, const RomPathChar *path) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(path != nullptr);

        RomDirectoryEntry parent_entry = {};
        EntryKey key = {};
        R_TRY(this->FindDirectoryRecursive(std::addressof(key), std::addressof(parent_entry), path));

        return this->FindOpen(out, key);
    }

    Result HierarchicalRomFileTable::FindOpen(FindPosition *out, RomDirectoryId id) {
        AMS_ASSERT(out != nullptr);

        out->next_dir  = InvalidPosition;
        out->next_file = InvalidPosition;

        RomDirectoryEntry entry = {};
        R_TRY(this->GetDirectoryEntry(std::addressof(entry), id));

        out->next_dir  = entry.dir;
        out->next_file = entry.file;

        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::FindNextDirectory(RomPathChar *out, FindPosition *find, size_t length) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(find != nullptr);
        AMS_ASSERT(length > RomPathTool::MaxPathLength);

        R_UNLESS(find->next_dir != InvalidPosition, fs::ResultDbmFindFinished());

        RomEntryKey key = {};
        RomDirectoryEntry entry = {};
        size_t aux_size = 0;
        R_TRY(this->dir_table.GetByPosition(std::addressof(key), std::addressof(entry), out, std::addressof(aux_size), find->next_dir));
        AMS_ASSERT(aux_size / sizeof(RomPathChar) <= RomPathTool::MaxPathLength);

        out[aux_size / sizeof(RomPathChar)] = RomStringTraits::NullTerminator;

        find->next_dir = entry.next;
        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::FindNextFile(RomPathChar *out, FindPosition *find, size_t length) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(find != nullptr);
        AMS_ASSERT(length > RomPathTool::MaxPathLength);

        R_UNLESS(find->next_file != InvalidPosition, fs::ResultDbmFindFinished());

        RomEntryKey key = {};
        RomFileEntry entry = {};
        size_t aux_size = 0;
        R_TRY(this->file_table.GetByPosition(std::addressof(key), std::addressof(entry), out, std::addressof(aux_size), find->next_file));
        AMS_ASSERT(aux_size / sizeof(RomPathChar) <= RomPathTool::MaxPathLength);

        out[aux_size / sizeof(RomPathChar)] = RomStringTraits::NullTerminator;

        find->next_file = entry.next;
        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::QueryRomFileSystemSize(s64 *out_dir_entry_size, s64 *out_file_entry_size) {
        AMS_ASSERT(out_dir_entry_size != nullptr);
        AMS_ASSERT(out_file_entry_size != nullptr);

        *out_dir_entry_size = this->dir_table.GetTotalEntrySize();
        *out_file_entry_size = this->file_table.GetTotalEntrySize();
        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::GetGrandParent(Position *out_pos, EntryKey *out_dir_key, RomDirectoryEntry *out_dir_entry, Position pos, RomPathTool::RomEntryName name, const RomPathChar *path) {
        AMS_ASSERT(out_pos != nullptr);
        AMS_ASSERT(out_dir_key != nullptr);
        AMS_ASSERT(out_dir_entry != nullptr);
        AMS_ASSERT(path != nullptr);

        RomEntryKey gp_key = {};
        RomDirectoryEntry gp_entry = {};
        R_TRY(this->dir_table.GetByPosition(std::addressof(gp_key), std::addressof(gp_entry), pos));
        out_dir_key->key.parent = gp_key.parent;

        R_TRY(RomPathTool::GetParentDirectoryName(std::addressof(out_dir_key->name), name, path));

        R_TRY(this->GetDirectoryEntry(out_pos, out_dir_entry, *out_dir_key));

        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::FindParentDirectoryRecursive(Position *out_pos, EntryKey *out_dir_key, RomDirectoryEntry *out_dir_entry, RomPathTool::PathParser *parser, const RomPathChar *path) {
        AMS_ASSERT(out_pos != nullptr);
        AMS_ASSERT(out_dir_key != nullptr);
        AMS_ASSERT(out_dir_entry != nullptr);
        AMS_ASSERT(parser != nullptr);
        AMS_ASSERT(path != nullptr);

        Position dir_pos = RootPosition;
        EntryKey dir_key = {};
        RomDirectoryEntry dir_entry = {};
        dir_key.key.parent = RootPosition;

        R_TRY(parser->GetNextDirectoryName(std::addressof(dir_key.name)));
        R_TRY(this->GetDirectoryEntry(std::addressof(dir_pos), std::addressof(dir_entry), dir_key));

        Position parent_pos = dir_pos;
        while (!parser->IsParseFinished()) {
            EntryKey old_key = dir_key;

            R_TRY(parser->GetNextDirectoryName(std::addressof(dir_key.name)));

            if (RomPathTool::IsCurrentDirectory(dir_key.name)) {
                dir_key = old_key;
                continue;
            } else if (RomPathTool::IsParentDirectory(dir_key.name)) {
                R_UNLESS(parent_pos != RootPosition, fs::ResultDirectoryUnobtainable());

                R_TRY(this->GetGrandParent(std::addressof(parent_pos), std::addressof(dir_key), std::addressof(dir_entry), dir_key.key.parent, dir_key.name, path));
            } else {
                dir_key.key.parent = parent_pos;
                R_TRY_CATCH(this->GetDirectoryEntry(std::addressof(dir_pos), std::addressof(dir_entry), dir_key)) {
                    R_CONVERT(fs::ResultDbmInvalidOperation, fs::ResultDbmNotFound())
                } R_END_TRY_CATCH;

                parent_pos = dir_pos;
            }
        }

        *out_pos = parent_pos;
        *out_dir_key = dir_key;
        *out_dir_entry = dir_entry;
        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::FindPathRecursive(EntryKey *out_key, RomDirectoryEntry *out_dir_entry, bool is_dir, const RomPathChar *path) {
        AMS_ASSERT(out_key != nullptr);
        AMS_ASSERT(out_dir_entry != nullptr);
        AMS_ASSERT(path != nullptr);

        RomPathTool::PathParser parser;
        R_TRY(parser.Initialize(path));

        EntryKey parent_key = {};
        Position parent_pos = 0;
        R_TRY(this->FindParentDirectoryRecursive(std::addressof(parent_pos), std::addressof(parent_key), out_dir_entry, std::addressof(parser), path));

        if (is_dir) {
            RomPathTool::RomEntryName name = {};
            R_TRY(parser.GetAsDirectoryName(std::addressof(name)));

            if (RomPathTool::IsCurrentDirectory(name)) {
                *out_key = parent_key;
                if (out_key->key.parent != RootPosition) {
                    Position pos = 0;
                    R_TRY(this->GetGrandParent(std::addressof(pos), std::addressof(parent_key), out_dir_entry, out_key->key.parent, out_key->name, path));
                }
            } else if (RomPathTool::IsParentDirectory(name)) {
                R_UNLESS(parent_pos != RootPosition, fs::ResultDirectoryUnobtainable());

                Position pos = 0;
                RomDirectoryEntry cur_entry = {};
                R_TRY(this->GetGrandParent(std::addressof(pos), out_key, std::addressof(cur_entry), parent_key.key.parent, parent_key.name, path));

                if (out_key->key.parent != RootPosition) {
                    R_TRY(this->GetGrandParent(std::addressof(pos), std::addressof(parent_key), out_dir_entry, out_key->key.parent, out_key->name, path));
                }
            } else {
                out_key->name = name;
                out_key->key.parent = (out_key->name.length > 0) ? parent_pos : RootPosition;
            }
        } else {
            {
                RomPathTool::RomEntryName name = {};
                R_TRY(parser.GetAsDirectoryName(std::addressof(name)));
                R_UNLESS(!RomPathTool::IsParentDirectory(name) || parent_pos != RootPosition, fs::ResultDirectoryUnobtainable());
            }

            R_UNLESS(!parser.IsDirectoryPath(), fs::ResultDbmInvalidOperation());

            out_key->key.parent = parent_pos;
            R_TRY(parser.GetAsFileName(std::addressof(out_key->name)));
        }

        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::FindDirectoryRecursive(EntryKey *out_key, RomDirectoryEntry *out_dir_entry, const RomPathChar *path) {
        AMS_ASSERT(out_key != nullptr);
        AMS_ASSERT(out_dir_entry != nullptr);
        AMS_ASSERT(path != nullptr);

        return this->FindPathRecursive(out_key, out_dir_entry, true, path);
    }

    Result HierarchicalRomFileTable::FindFileRecursive(EntryKey *out_key, RomDirectoryEntry *out_dir_entry, const RomPathChar *path) {
        AMS_ASSERT(out_key != nullptr);
        AMS_ASSERT(out_dir_entry != nullptr);
        AMS_ASSERT(path != nullptr);

        return this->FindPathRecursive(out_key, out_dir_entry, false, path);
    }

    Result HierarchicalRomFileTable::CheckSameEntryExists(const EntryKey &key, Result if_exists) {
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

    Result HierarchicalRomFileTable::GetDirectoryEntry(Position *out_pos, RomDirectoryEntry *out_entry, const EntryKey &key) {
        AMS_ASSERT(out_pos != nullptr);
        AMS_ASSERT(out_entry != nullptr);

        const Result dir_res = this->dir_table.Get(out_pos, out_entry, key);
        R_UNLESS(R_FAILED(dir_res),                           dir_res);
        R_UNLESS(fs::ResultDbmKeyNotFound::Includes(dir_res), dir_res);

        Position pos = 0;
        RomFileEntry entry = {};
        const Result file_res = this->file_table.Get(std::addressof(pos), std::addressof(entry), key);
        R_UNLESS(R_FAILED(file_res),                            fs::ResultDbmInvalidOperation());
        R_UNLESS(!fs::ResultDbmKeyNotFound::Includes(file_res), fs::ResultDbmDirectoryNotFound());
        return file_res;
    }

    Result HierarchicalRomFileTable::GetDirectoryEntry(RomDirectoryEntry *out_entry, RomDirectoryId id) {
        AMS_ASSERT(out_entry != nullptr);
        Position pos = DirectoryIdToPosition(id);

        RomEntryKey key = {};
        const Result dir_res = this->dir_table.GetByPosition(std::addressof(key), out_entry, pos);
        R_UNLESS(R_FAILED(dir_res),                           dir_res);
        R_UNLESS(fs::ResultDbmKeyNotFound::Includes(dir_res), dir_res);

        RomFileEntry entry = {};
        const Result file_res = this->file_table.GetByPosition(std::addressof(key), std::addressof(entry), pos);
        R_UNLESS(R_FAILED(file_res),                            fs::ResultDbmInvalidOperation());
        R_UNLESS(!fs::ResultDbmKeyNotFound::Includes(file_res), fs::ResultDbmDirectoryNotFound());
        return file_res;
    }

    Result HierarchicalRomFileTable::GetFileEntry(Position *out_pos, RomFileEntry *out_entry, const EntryKey &key) {
        AMS_ASSERT(out_pos != nullptr);
        AMS_ASSERT(out_entry != nullptr);

        const Result file_res = this->file_table.Get(out_pos, out_entry, key);
        R_UNLESS(R_FAILED(file_res),                           file_res);
        R_UNLESS(fs::ResultDbmKeyNotFound::Includes(file_res), file_res);

        Position pos = 0;
        RomDirectoryEntry entry = {};
        const Result dir_res = this->dir_table.Get(std::addressof(pos), std::addressof(entry), key);
        R_UNLESS(R_FAILED(dir_res),                            fs::ResultDbmInvalidOperation());
        R_UNLESS(!fs::ResultDbmKeyNotFound::Includes(dir_res), fs::ResultDbmFileNotFound());
        return dir_res;
    }

    Result HierarchicalRomFileTable::GetFileEntry(RomFileEntry *out_entry, RomFileId id) {
        AMS_ASSERT(out_entry != nullptr);
        Position pos = FileIdToPosition(id);

        RomEntryKey key = {};
        const Result file_res = this->file_table.GetByPosition(std::addressof(key), out_entry, pos);
        R_UNLESS(R_FAILED(file_res),                           file_res);
        R_UNLESS(fs::ResultDbmKeyNotFound::Includes(file_res), file_res);

        RomDirectoryEntry entry = {};
        const Result dir_res = this->dir_table.GetByPosition(std::addressof(key), std::addressof(entry), pos);
        R_UNLESS(R_FAILED(dir_res),                            fs::ResultDbmInvalidOperation());
        R_UNLESS(!fs::ResultDbmKeyNotFound::Includes(dir_res), fs::ResultDbmFileNotFound());
        return dir_res;
    }

    Result HierarchicalRomFileTable::GetDirectoryInformation(DirectoryInfo *out, const EntryKey &key) {
        AMS_ASSERT(out != nullptr);

        Position pos = 0;
        RomDirectoryEntry entry = {};
        R_TRY(this->GetDirectoryEntry(std::addressof(pos), std::addressof(entry), key));

        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::OpenFile(FileInfo *out, const EntryKey &key) {
        AMS_ASSERT(out != nullptr);

        Position pos = 0;
        RomFileEntry entry = {};
        R_TRY(this->GetFileEntry(std::addressof(pos), std::addressof(entry), key));

        *out = entry.info;
        return ResultSuccess();
    }

    Result HierarchicalRomFileTable::FindOpen(FindPosition *out, const EntryKey &key) {
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

}