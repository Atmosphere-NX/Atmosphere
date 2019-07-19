/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <algorithm>
#include <switch.h>
#include <sys/stat.h>
#include "kvdb_auto_buffer.hpp"
#include "kvdb_archive.hpp"
#include "kvdb_bounded_string.hpp"

namespace sts::kvdb {

    template<class Key>
    class MemoryKeyValueStore {
        static_assert(std::is_pod<Key>::value, "KeyValueStore Keys must be pod!");
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
                            if (sizeof(Value) > this->value_size) {
                                std::abort();
                            }
                            /* Ensure we only get pod. */
                            static_assert(std::is_pod<Value>::value, "KeyValueStore Values must be pod");
                        }
                        return reinterpret_cast<Value *>(this->value);
                    }

                    template<typename Value = void>
                    const Value *GetValuePointer() const {
                        /* Size check. Note: Nintendo does not size check. */
                        if constexpr (!std::is_same<Value, void>::value) {
                            if (sizeof(Value) > this->value_size) {
                                std::abort();
                            }
                            /* Ensure we only get pod. */
                            static_assert(std::is_pod<Value>::value, "KeyValueStore Values must be pod");
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
                public:
                    Index() : count(0), capacity(0), entries(nullptr) { /* ... */ }

                    ~Index() {
                        if (this->entries != nullptr) {
                            this->ResetEntries();
                            std::free(this->entries);
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
                            std::free(this->entries[i].GetValuePointer());
                        }
                        this->count = 0;
                    }

                    Result Initialize(size_t capacity) {
                        this->entries = reinterpret_cast<Entry *>(std::malloc(sizeof(Entry) * capacity));
                        if (this->entries == nullptr) {
                            return ResultKvdbAllocationFailed;
                        }
                        this->capacity = capacity;
                        return ResultSuccess;
                    }

                    Result Set(const Key &key, const void *value, size_t value_size) {
                        /* Allocate new value. */
                        void *new_value = std::malloc(value_size);
                        if (new_value == nullptr) {
                            return ResultKvdbAllocationFailed;
                        }
                        std::memcpy(new_value, value, value_size);

                        /* Find entry for key. */
                        Entry *it = this->lower_bound(key);
                        if (it != this->end() && it->GetKey() == key) {
                            /* Entry already exists. Free old value. */
                            std::free(it->GetValuePointer());
                        } else {
                            /* We need to add a new entry. Check we have room, move future keys forward. */
                            if (this->count >= this->capacity) {
                                std::free(new_value);
                                return ResultKvdbKeyCapacityInsufficient;
                            }
                            std::memmove(it + 1, it, sizeof(*it) * (this->end() - it));
                            this->count++;
                        }

                        /* Save the new Entry in the map. */
                        *it = Entry(key, new_value, value_size);
                        return ResultSuccess;
                    }

                    Result AddUnsafe(const Key &key, void *value, size_t value_size) {
                        if (this->count >= this->capacity) {
                            return ResultKvdbKeyCapacityInsufficient;
                        }

                        this->entries[this->count++] = Entry(key, value, value_size);
                        return ResultSuccess;
                    }

                    Result Remove(const Key &key) {
                        /* Find entry for key. */
                        Entry *it = this->find(key);

                        /* Check if the entry is valid. */
                        if (it != this->end()) {
                            /* Free the value, move entries back. */
                            std::free(it->GetValuePointer());
                            std::memmove(it, it + 1, sizeof(*it) * (this->end() - (it + 1)));
                            this->count--;
                            return ResultSuccess;
                        }

                        /* If it's not, we didn't remove it. */
                        return ResultKvdbKeyNotFound;
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
            static constexpr size_t MaxPathLen = 0x300; /* TODO: FS_MAX_PATH - 1? */
            using Path = kvdb::BoundedString<MaxPathLen>;
        private:
            Index index;
            Path path;
            Path temp_path;
        public:
            MemoryKeyValueStore() { /* ... */ }

            Result Initialize(const char *dir, size_t capacity) {
                /* Ensure that the passed path is a directory. */
                {
                    struct stat st;
                    if (stat(dir, &st) != 0 || !(S_ISDIR(st.st_mode))) {
                        return ResultFsPathNotFound;
                    }
                }

                /* Set paths. */
                this->path.SetFormat("%s%s", dir, "/imkvdb.arc");
                this->temp_path.SetFormat("%s%s", dir, "/imkvdb.tmp");

                /* Initialize our index. */
                R_TRY(this->index.Initialize(capacity));
                return ResultSuccess;
            }

            Result Initialize(size_t capacity) {
                /* This initializes without an archive file. */
                /* A store initialized this way cannot have its contents loaded from or flushed to disk. */
                this->path.Set("");
                this->temp_path.Set("");

                /* Initialize our index. */
                R_TRY(this->index.Initialize(capacity));
                return ResultSuccess;
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
                    R_CATCH(ResultFsPathNotFound) {
                        return ResultSuccess;
                    }
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
                        void *new_value = std::malloc(value_size);
                        if (new_value == nullptr) {
                            return ResultKvdbAllocationFailed;
                        }
                        auto value_guard = SCOPE_GUARD { std::free(new_value); };

                        /* Read key and value. */
                        Key key;
                        R_TRY(reader.ReadEntry(&key, sizeof(key), new_value, value_size));
                        R_TRY(this->index.AddUnsafe(key, new_value, value_size));

                        /* We succeeded, so cancel the value guard to prevent deallocation. */
                        value_guard.Cancel();
                    }
                }

                return ResultSuccess;
            }

            Result Save() {
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
                return this->Commit(buffer);
            }

            Result Set(const Key &key, const void *value, size_t value_size) {
                return this->index.Set(key, value, value_size);
            }

            template<typename Value>
            Result Set(const Key &key, const Value &value) {
                /* Only allow setting pod. */
                static_assert(std::is_pod<Value>::value, "KeyValueStore Values must be pod");
                return this->Set(key, &value, sizeof(Value));
            }

            template<typename Value>
            Result Set(const Key &key, const Value *value) {
                /* Only allow setting pod. */
                static_assert(std::is_pod<Value>::value, "KeyValueStore Values must be pod");
                return this->Set(key, value, sizeof(Value));
            }

            Result Get(size_t *out_size, void *out_value, size_t max_out_size, const Key &key) {
                /* Find entry. */
                auto it = this->find(key);
                if (it == this->end()) {
                    return ResultKvdbKeyNotFound;
                }

                size_t size = std::min(max_out_size, it->GetValueSize());
                std::memcpy(out_value, it->GetValuePointer(), size);
                *out_size = size;
                return ResultSuccess;
            }

            template<typename Value = void>
            Result GetValuePointer(Value **out_value, const Key &key) {
                /* Find entry. */
                auto it = this->find(key);
                if (it == this->end()) {
                    return ResultKvdbKeyNotFound;
                }

                *out_value = it->template GetValuePointer<Value>();
                return ResultSuccess;
            }

            template<typename Value = void>
            Result GetValuePointer(const Value **out_value, const Key &key) const {
                /* Find entry. */
                auto it = this->find(key);
                if (it == this->end()) {
                    return ResultKvdbKeyNotFound;
                }

                *out_value = it->template GetValuePointer<Value>();
                return ResultSuccess;
            }

            template<typename Value>
            Result GetValue(Value *out_value, const Key &key) const {
                /* Find entry. */
                auto it = this->find(key);
                if (it == this->end()) {
                    return ResultKvdbKeyNotFound;
                }

                *out_value = it->template GetValue<Value>();
                return ResultSuccess;
            }

            Result GetValueSize(size_t *out_size, const Key &key) const {
                /* Find entry. */
                auto it = this->find(key);
                if (it == this->end()) {
                    return ResultKvdbKeyNotFound;
                }

                *out_size = it->GetValueSize();
                return ResultSuccess;
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
            Result Commit(const AutoBuffer &buffer) {
                /* Try to delete temporary archive, but allow deletion failure (it may not exist). */
                std::remove(this->temp_path.Get());

                /* Create new temporary archive. */
                R_TRY(fsdevCreateFile(this->temp_path.Get(), buffer.GetSize(), 0));

                /* Write data to the temporary archive. */
                {
                    FILE *f = fopen(this->temp_path, "r+b");
                    if (f == nullptr) {
                        return fsdevGetLastResult();
                    }
                    ON_SCOPE_EXIT { fclose(f); };

                    if (fwrite(buffer.Get(), buffer.GetSize(), 1, f) != 1) {
                        return fsdevGetLastResult();
                    }
                }

                /* Try to delete the saved archive, but allow deletion failure. */
                std::remove(this->path.Get());

                /* Rename the path. */
                if (std::rename(this->temp_path.Get(), this->path.Get()) != 0) {
                    return fsdevGetLastResult();
                }

                return ResultSuccess;
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
                FILE *f = fopen(this->path, "rb");
                if (f == nullptr) {
                    return fsdevGetLastResult();
                }
                ON_SCOPE_EXIT { fclose(f); };

                /* Get the archive file size. */
                fseek(f, 0, SEEK_END);
                const size_t archive_size = ftell(f);
                fseek(f, 0, SEEK_SET);

                /* Make a new buffer, read the file. */
                R_TRY(dst->Initialize(archive_size));
                if (fread(dst->Get(), archive_size, 1, f) != 1) {
                    return fsdevGetLastResult();
                }

                return ResultSuccess;
            }
    };

}