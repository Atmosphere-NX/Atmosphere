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
#include <stratosphere/fs/fs_file.hpp>
#include <stratosphere/fs/fs_directory.hpp>
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/kvdb/kvdb_auto_buffer.hpp>
#include <stratosphere/kvdb/kvdb_archive.hpp>
#include <stratosphere/kvdb/kvdb_bounded_string.hpp>

namespace ams::kvdb {

    template<class Key>
    class MemoryKeyValueStore {
        static_assert(util::is_pod<Key>::value, "KeyValueStore Keys must be pod!");
        NON_COPYABLE(MemoryKeyValueStore);
        NON_MOVEABLE(MemoryKeyValueStore);
        public:
            /* Subtypes. */
            class Entry {
                private:
                    Key key;
                    void *value;
                    size_t value_size;
                public:
                    constexpr Entry(const Key &k, void *v, size_t s) : key(k), value(v), value_size(s) { /* ... */ }

                    const Key &GetKey() const {
                        return this->key;
                    }

                    template<typename Value = void>
                    Value *GetValuePointer() {
                        /* Size check. Note: Nintendo does not size check. */
                        if constexpr (!std::is_same<Value, void>::value) {
                            AMS_ABORT_UNLESS(sizeof(Value) <= this->value_size);
                            /* Ensure we only get pod. */
                            static_assert(util::is_pod<Value>::value, "KeyValueStore Values must be pod");
                        }
                        return reinterpret_cast<Value *>(this->value);
                    }

                    template<typename Value = void>
                    const Value *GetValuePointer() const {
                        /* Size check. Note: Nintendo does not size check. */
                        if constexpr (!std::is_same<Value, void>::value) {
                            AMS_ABORT_UNLESS(sizeof(Value) <= this->value_size);
                            /* Ensure we only get pod. */
                            static_assert(util::is_pod<Value>::value, "KeyValueStore Values must be pod");
                        }
                        return reinterpret_cast<Value *>(this->value);
                    }

                    template<typename Value>
                    Value &GetValue() {
                        return *(this->GetValuePointer<Value>());
                    }

                    template<typename Value>
                    const Value &GetValue() const {
                        return *(this->GetValuePointer<Value>());
                    }

                    size_t GetValueSize() const {
                        return this->value_size;
                    }

                    constexpr inline bool operator<(const Key &rhs) const {
                        return key < rhs;
                    }

                    constexpr inline bool operator==(const Key &rhs) const {
                        return key == rhs;
                    }
            };

            class Index {
                private:
                    size_t count;
                    size_t capacity;
                    Entry *entries;
                    MemoryResource *memory_resource;
                public:
                    Index() : count(0), capacity(0), entries(nullptr), memory_resource(nullptr) { /* ... */ }

                    ~Index() {
                        if (this->entries != nullptr) {
                            this->ResetEntries();
                            this->memory_resource->Deallocate(this->entries, sizeof(Entry) * this->capacity);
                            this->entries = nullptr;
                        }
                    }

                    size_t GetCount() const {
                        return this->count;
                    }

                    size_t GetCapacity() const {
                        return this->capacity;
                    }

                    void ResetEntries() {
                        for (size_t i = 0; i < this->count; i++) {
                            this->memory_resource->Deallocate(this->entries[i].GetValuePointer(), this->entries[i].GetValueSize());
                        }
                        this->count = 0;
                    }

                    Result Initialize(size_t capacity, MemoryResource *mr) {
                        this->entries = reinterpret_cast<Entry *>(mr->Allocate(sizeof(Entry) * capacity));
                        R_UNLESS(this->entries != nullptr, ResultAllocationFailed());
                        this->capacity = capacity;
                        this->memory_resource = mr;
                        return ResultSuccess();
                    }

                    Result Set(const Key &key, const void *value, size_t value_size) {
                        /* Find entry for key. */
                        Entry *it = this->lower_bound(key);
                        if (it != this->end() && it->GetKey() == key) {
                            /* Entry already exists. Free old value. */
                            this->memory_resource->Deallocate(it->GetValuePointer(), it->GetValueSize());
                        } else {
                            /* We need to add a new entry. Check we have room, move future keys forward. */
                            R_UNLESS(this->count < this->capacity, ResultOutOfKeyResource());
                            std::memmove(it + 1, it, sizeof(*it) * (this->end() - it));
                            this->count++;
                        }

                        /* Allocate new value. */
                        void *new_value = this->memory_resource->Allocate(value_size);
                        R_UNLESS(new_value != nullptr, ResultAllocationFailed());
                        std::memcpy(new_value, value, value_size);

                        /* Save the new Entry in the map. */
                        *it = Entry(key, new_value, value_size);
                        return ResultSuccess();
                    }

                    Result AddUnsafe(const Key &key, void *value, size_t value_size) {
                        R_UNLESS(this->count < this->capacity, ResultOutOfKeyResource());

                        this->entries[this->count++] = Entry(key, value, value_size);
                        return ResultSuccess();
                    }

                    Result Remove(const Key &key) {
                        /* Find entry for key. */
                        Entry *it = this->find(key);
                        R_UNLESS(it != this->end(), ResultKeyNotFound());

                        /* Free the value, move entries back. */
                        this->memory_resource->Deallocate(it->GetValuePointer(), it->GetValueSize());
                        std::memmove(it, it + 1, sizeof(*it) * (this->end() - (it + 1)));
                        this->count--;
                        return ResultSuccess();
                    }

                    Entry *begin() {
                        return this->GetBegin();
                    }

                    const Entry *begin() const {
                        return this->GetBegin();
                    }

                    Entry *end() {
                        return this->GetEnd();
                    }

                    const Entry *end() const {
                        return this->GetEnd();
                    }

                    const Entry *cbegin() const {
                        return this->begin();
                    }

                    const Entry *cend() const {
                        return this->end();
                    }

                    Entry *lower_bound(const Key &key) {
                        return this->GetLowerBound(key);
                    }

                    const Entry *lower_bound(const Key &key) const {
                        return this->GetLowerBound(key);
                    }

                    Entry *find(const Key &key) {
                        return this->Find(key);
                    }

                    const Entry *find(const Key &key) const {
                        return this->Find(key);
                    }
                private:
                    Entry *GetBegin() {
                        return this->entries;
                    }

                    const Entry *GetBegin() const {
                        return this->entries;
                    }

                    Entry *GetEnd() {
                        return this->GetBegin() + this->count;
                    }

                    const Entry *GetEnd() const {
                        return this->GetBegin() + this->count;
                    }

                    Entry *GetLowerBound(const Key &key) {
                        return std::lower_bound(this->GetBegin(), this->GetEnd(), key);
                    }

                    const Entry *GetLowerBound(const Key &key) const {
                        return std::lower_bound(this->GetBegin(), this->GetEnd(), key);
                    }

                    Entry *Find(const Key &key) {
                        auto it = this->GetLowerBound(key);
                        auto end = this->GetEnd();
                        if (it != end && it->GetKey() == key) {
                            return it;
                        }
                        return end;
                    }

                    const Entry *Find(const Key &key) const {
                        auto it = this->GetLowerBound(key);
                        auto end = this->GetEnd();
                        if (it != end && it->GetKey() == key) {
                            return it;
                        }
                        return end;
                    }
            };
        private:
            using Path = kvdb::BoundedString<fs::EntryNameLengthMax>;
        private:
            Index index;
            Path path;
            Path temp_path;
            MemoryResource *memory_resource;
        public:
            MemoryKeyValueStore() { /* ... */ }

            Result Initialize(const char *dir, size_t capacity, MemoryResource *mr) {
                /* Ensure that the passed path is a directory. */
                fs::DirectoryEntryType entry_type;
                R_TRY(fs::GetEntryType(std::addressof(entry_type), dir));
                R_UNLESS(entry_type == fs::DirectoryEntryType_Directory, fs::ResultPathNotFound());

                /* Set paths. */
                this->path.SetFormat("%s%s", dir, "/imkvdb.arc");
                this->temp_path.SetFormat("%s%s", dir, "/imkvdb.tmp");

                /* Initialize our index. */
                R_TRY(this->index.Initialize(capacity, mr));
                this->memory_resource = mr;

                return ResultSuccess();
            }

            Result Initialize(size_t capacity, MemoryResource *mr) {
                /* This initializes without an archive file. */
                /* A store initialized this way cannot have its contents loaded from or flushed to disk. */
                this->path.Set("");
                this->temp_path.Set("");

                /* Initialize our index. */
                R_TRY(this->index.Initialize(capacity, mr));
                this->memory_resource = mr;
                return ResultSuccess();
            }

            size_t GetCount() const {
                return this->index.GetCount();
            }

            size_t GetCapacity() const {
                return this->index.GetCapacity();
            }

            Result Load() {
                /* Reset any existing entries. */
                this->index.ResetEntries();

                /* Try to read the archive -- note, path not found is a success condition. */
                /* This is because no archive file = no entries, so we're in the right state. */
                AutoBuffer buffer;
                R_TRY_CATCH(this->ReadArchiveFile(&buffer)) {
                    R_CONVERT(fs::ResultPathNotFound, ResultSuccess());
                } R_END_TRY_CATCH;

                /* Parse entries from the buffer. */
                {
                    ArchiveReader reader(buffer);

                    size_t entry_count = 0;
                    R_TRY(reader.ReadEntryCount(&entry_count));

                    for (size_t i = 0; i < entry_count; i++) {
                        /* Get size of key/value. */
                        size_t key_size = 0, value_size = 0;
                        R_TRY(reader.GetEntrySize(&key_size, &value_size));

                        /* Allocate memory for value. */
                        void *new_value = this->memory_resource->Allocate(value_size);
                        R_UNLESS(new_value != nullptr, ResultAllocationFailed());
                        auto value_guard = SCOPE_GUARD { this->memory_resource->Deallocate(new_value, value_size); };

                        /* Read key and value. */
                        Key key;
                        R_TRY(reader.ReadEntry(&key, sizeof(key), new_value, value_size));
                        R_TRY(this->index.AddUnsafe(key, new_value, value_size));

                        /* We succeeded, so cancel the value guard to prevent deallocation. */
                        value_guard.Cancel();
                    }
                }

                return ResultSuccess();
            }

            Result Save(bool destructive = false) {
                /* Create a buffer to hold the archive. */
                AutoBuffer buffer;
                R_TRY(buffer.Initialize(this->GetArchiveSize()));

                /* Write the archive to the buffer. */
                {
                    ArchiveWriter writer(buffer);
                    writer.WriteHeader(this->GetCount());
                    for (const auto &it : this->index) {
                        const auto &key = it.GetKey();
                        writer.WriteEntry(&key, sizeof(Key), it.GetValuePointer(), it.GetValueSize());
                    }
                }

                /* Save the buffer to disk. */
                return this->Commit(buffer, destructive);
            }

            Result Set(const Key &key, const void *value, size_t value_size) {
                return this->index.Set(key, value, value_size);
            }

            template<typename Value>
            Result Set(const Key &key, const Value &value) {
                /* Only allow setting pod. */
                static_assert(util::is_pod<Value>::value, "KeyValueStore Values must be pod");
                return this->Set(key, &value, sizeof(Value));
            }

            template<typename Value>
            Result Set(const Key &key, const Value *value) {
                /* Only allow setting pod. */
                static_assert(util::is_pod<Value>::value, "KeyValueStore Values must be pod");
                return this->Set(key, value, sizeof(Value));
            }

            Result Get(size_t *out_size, void *out_value, size_t max_out_size, const Key &key) {
                /* Find entry. */
                auto it = this->find(key);
                R_UNLESS(it != this->end(), ResultKeyNotFound());

                size_t size = std::min(max_out_size, it->GetValueSize());
                std::memcpy(out_value, it->GetValuePointer(), size);
                *out_size = size;
                return ResultSuccess();
            }

            template<typename Value = void>
            Result GetValuePointer(Value **out_value, const Key &key) {
                /* Find entry. */
                auto it = this->find(key);
                R_UNLESS(it != this->end(), ResultKeyNotFound());

                *out_value = it->template GetValuePointer<Value>();
                return ResultSuccess();
            }

            template<typename Value = void>
            Result GetValuePointer(const Value **out_value, const Key &key) const {
                /* Find entry. */
                auto it = this->find(key);
                R_UNLESS(it != this->end(), ResultKeyNotFound());

                *out_value = it->template GetValuePointer<Value>();
                return ResultSuccess();
            }

            template<typename Value>
            Result GetValue(Value *out_value, const Key &key) const {
                /* Find entry. */
                auto it = this->find(key);
                R_UNLESS(it != this->end(), ResultKeyNotFound());

                *out_value = it->template GetValue<Value>();
                return ResultSuccess();
            }

            Result GetValueSize(size_t *out_size, const Key &key) const {
                /* Find entry. */
                auto it = this->find(key);
                R_UNLESS(it != this->end(), ResultKeyNotFound());

                *out_size = it->GetValueSize();
                return ResultSuccess();
            }

            Result Remove(const Key &key) {
                return this->index.Remove(key);
            }

            Entry *begin() {
                return this->index.begin();
            }

            const Entry *begin() const {
                return this->index.begin();
            }

            Entry *end() {
                return this->index.end();
            }

            const Entry *end() const {
                return this->index.end();
            }

            const Entry *cbegin() const {
                return this->index.cbegin();
            }

            const Entry *cend() const {
                return this->index.cend();
            }

            Entry *lower_bound(const Key &key) {
                return this->index.lower_bound(key);
            }

            const Entry *lower_bound(const Key &key) const {
                return this->index.lower_bound(key);
            }

            Entry *find(const Key &key) {
                return this->index.find(key);
            }

            const Entry *find(const Key &key) const {
                return this->index.find(key);
            }
        private:
            Result SaveArchiveToFile(const char *path, const void *buf, size_t size) {
                /* Try to delete the archive, but allow deletion failure. */
                fs::DeleteFile(path);

                /* Create new archive. */
                R_TRY(fs::CreateFile(path, size));

                /* Write data to the archive. */
                {
                    fs::FileHandle file;
                    R_TRY(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Write));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };
                    R_TRY(fs::WriteFile(file, 0, buf, size, fs::WriteOption::Flush));
                }

                return ResultSuccess();
            }

            Result Commit(const AutoBuffer &buffer, bool destructive) {
                if (destructive) {
                    /* Delete and save to the real archive. */
                    R_TRY(SaveArchiveToFile(this->path.Get(), buffer.Get(), buffer.GetSize()));
                } else {
                    /* Delete and save to a temporary archive. */
                    R_TRY(SaveArchiveToFile(this->temp_path.Get(), buffer.Get(), buffer.GetSize()));

                    /* Try to delete the saved archive, but allow deletion failure. */
                    fs::DeleteFile(this->path.Get());

                    /* Rename the path. */
                    R_TRY(fs::RenameFile(this->temp_path.Get(), this->path.Get()));
                }

                return ResultSuccess();
            }

            size_t GetArchiveSize() const {
                ArchiveSizeHelper size_helper;

                for (const auto &it : this->index) {
                    size_helper.AddEntry(sizeof(Key), it.GetValueSize());
                }

                return size_helper.GetSize();
            }

            Result ReadArchiveFile(AutoBuffer *dst) const {
                /* Open the file. */
                fs::FileHandle file;
                R_TRY(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Read));
                ON_SCOPE_EXIT { fs::CloseFile(file); };

                /* Get the archive file size. */
                s64 archive_size;
                R_TRY(fs::GetFileSize(std::addressof(archive_size), file));

                /* Make a new buffer, read the file. */
                R_TRY(dst->Initialize(static_cast<size_t>(archive_size)));
                R_TRY(fs::ReadFile(file, 0, dst->Get(), dst->GetSize()));

                return ResultSuccess();
            }
    };

}