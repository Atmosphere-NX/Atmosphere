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

#include <sys/stat.h>
#include <switch.h>
#include "kvdb_bounded_string.hpp"
#include "kvdb_file_key_value_store.hpp"

namespace sts::kvdb {

    namespace impl {

        template<class Key, size_t Capacity>
        class LruList {
            private:
                /* Subtypes. */
                struct LruHeader {
                    u32 entry_count;
                };
            public:
                static constexpr size_t BufferSize = sizeof(Key) * Capacity;
                static constexpr size_t FileSize = sizeof(LruHeader) + BufferSize;
                using Path = FileKeyValueStore::Path;
            private:
                Path file_path;
                Key *keys;
                LruHeader header;
            public:
                static Result CreateNewList(const char *path) {
                    /* Create new lru_list.dat. */
                    R_TRY(fsdevCreateFile(path, FileSize, 0));

                    /* Open the file. */
                    FILE *fp = fopen(path, "r+b");
                    if (fp == nullptr) {
                        return fsdevGetLastResult();
                    }
                    ON_SCOPE_EXIT { fclose(fp); };

                    /* Write new header with zero entries to the file. */
                    LruHeader new_header = { .entry_count = 0, };
                    if (fwrite(&new_header, sizeof(new_header), 1, fp) != 1) {
                        return fsdevGetLastResult();
                    }

                    return ResultSuccess;
                }
            private:
                void RemoveIndex(size_t i) {
                    if (i >= this->GetCount()) {
                        std::abort();
                    }
                    std::memmove(this->keys + i, this->keys + i + 1, sizeof(*this->keys) * (this->GetCount() - (i + 1)));
                    this->DecrementCount();
                }

                void IncrementCount() {
                    this->header.entry_count++;
                }

                void DecrementCount() {
                    this->header.entry_count--;
                }
            public:
                LruList() : keys(nullptr), header({}) { /* ... */ }

                Result Initialize(const char *path, void *buf, size_t size) {
                    /* Only initialize once, and ensure we have sufficient memory. */
                    if (this->keys != nullptr || size < BufferSize) {
                        std::abort();
                    }

                    /* Setup member variables. */
                    this->keys = static_cast<Key *>(buf);
                    this->file_path.Set(path);
                    std::memset(this->keys, 0, BufferSize);

                    /* Open file. */
                    FILE *fp = fopen(this->file_path, "rb");
                    if (fp == nullptr) {
                        return fsdevGetLastResult();
                    }
                    ON_SCOPE_EXIT { fclose(fp); };

                    /* Read header. */
                    if (fread(&this->header, sizeof(this->header), 1, fp) != 1) {
                        return fsdevGetLastResult();
                    }

                    /* Read entries. */
                    const size_t count = this->GetCount();
                    if (count > 0) {
                        if (fread(this->keys, std::min(BufferSize, sizeof(Key) * count), 1, fp) != 1) {
                            return fsdevGetLastResult();
                        }
                    }

                    return ResultSuccess;
                }

                Result Save() {
                    /* Open file. */
                    FILE *fp = fopen(this->file_path, "r+b");
                    if (fp == nullptr) {
                        return fsdevGetLastResult();
                    }
                    ON_SCOPE_EXIT { fclose(fp); };

                    /* Write header. */
                    if (fwrite(&this->header, sizeof(this->header), 1, fp) != 1) {
                        return fsdevGetLastResult();
                    }

                    /* Write entries. */
                    if (fwrite(this->keys, BufferSize, 1, fp) != 1) {
                        return fsdevGetLastResult();
                    }

                    /* Flush. */
                    fflush(fp);

                    return ResultSuccess;
                }

                size_t GetCount() const {
                    return this->header.entry_count;
                }

                bool IsEmpty() const {
                    return this->GetCount() == 0;
                }

                bool IsFull() const {
                    return this->GetCount() >= Capacity;
                }

                Key Get(size_t i) const {
                    if (i >= this->GetCount()) {
                        std::abort();
                    }
                    return this->keys[i];
                }

                Key Peek() const {
                    if (this->IsEmpty()) {
                        std::abort();
                    }
                    return this->Get(0);
                }

                void Push(const Key &key) {
                    if (this->IsFull()) {
                        std::abort();
                    }
                    this->keys[this->GetCount()] = key;
                    this->IncrementCount();
                }

                Key Pop() {
                    if (this->IsEmpty()) {
                        std::abort();
                    }
                    this->RemoveIndex(0);
                }

                bool Remove(const Key &key) {
                    const size_t count = this->GetCount();

                    /* Iterate over the list, removing the last entry that matches the key. */
                    for (size_t i = 0; i < count; i++) {
                        if (this->keys[count - 1 - i] == key) {
                            this->RemoveIndex(count - 1 - i);
                            return true;
                        }
                    }

                    return false;
                }

                bool Contains(const Key &key) const {
                    const size_t count = this->GetCount();

                    /* Iterate over the list, checking to see if we have the key. */
                    for (size_t i = 0; i < count; i++) {
                        if (this->keys[count - 1 - i] == key) {
                            return true;
                        }
                    }

                    return false;
                }

                bool Update(const Key &key) {
                    if (this->Remove(key)) {
                        this->Push(key);
                        return true;
                    }

                    return false;
                }
        };

    }

    template<class Key, size_t Capacity>
    class FileKeyValueCache {
        static_assert(std::is_pod<Key>::value, "FileKeyValueCache Key must be pod!");
        static_assert(sizeof(Key) <= FileKeyValueStore::MaxKeySize, "FileKeyValueCache Key is too big!");
        public:
            using LeastRecentlyUsedList = impl::LruList<Key, Capacity>;
            /* Note: Nintendo code in NS uses Path = BoundedString<0x180> here. */
            /* It's unclear why, since they use 0x300 everywhere else. */
            /* We'll just use 0x300, since it shouldn't make a difference, */
            /* as FileKeyValueStore paths are limited to 0x100 anyway. */
            using Path = typename LeastRecentlyUsedList::Path;
        private:
            FileKeyValueStore kvs;
            LeastRecentlyUsedList lru_list;
        private:
            static constexpr Path GetLeastRecentlyUsedListPath(const char *dir) {
                return Path::MakeFormat("%s/%s", dir, "lru_list.dat");
            }

            static constexpr Path GetFileKeyValueStorePath(const char *dir) {
                return Path::MakeFormat("%s/%s", dir, "kvs");
            }

            static Result Exists(bool *out, const char *path, bool is_dir) {
                /* Check if the path exists. */
                struct stat st;
                if (stat(path, &st) != 0) {
                    R_TRY_CATCH(fsdevGetLastResult()) {
                        R_CATCH(ResultFsPathNotFound) {
                            /* If the path doesn't exist, nothing has gone wrong. */
                            *out = false;
                            return ResultSuccess;
                        }
                    } R_END_TRY_CATCH;
                }

                /* Check that our entry type is correct. */
                if ((is_dir && !(S_ISDIR(st.st_mode))) || (!is_dir && !(S_ISREG(st.st_mode)))) {
                    return ResultKvdbInvalidFilesystemState;
                }

                *out = true;
                return ResultSuccess;
            }

            static Result DirectoryExists(bool *out, const char *path) {
                return Exists(out, path, true);
            }

            static Result FileExists(bool *out, const char *path) {
                return Exists(out, path, false);
            }
        public:
            static Result CreateNewCache(const char *dir) {
                /* Make a new key value store filesystem, and a new lru_list.dat. */
                R_TRY(LeastRecentlyUsedList::CreateNewList(GetLeastRecentlyUsedListPath(dir)));
                if (mkdir(GetFileKeyValueStorePath(dir), 0) != 0) {
                    return fsdevGetLastResult();
                }

                return ResultSuccess;
            }

            static Result ValidateExistingCache(const char *dir) {
                /* Check for existence. */
                bool has_lru = false, has_kvs = false;
                R_TRY(FileExists(&has_lru, GetLeastRecentlyUsedListPath(dir)));
                R_TRY(DirectoryExists(&has_kvs, GetFileKeyValueStorePath(dir)));

                /* If neither exists, CreateNewCache was never called. */
                if (!has_lru && !has_kvs) {
                    return ResultKvdbNotCreated;
                }

                /* If one exists but not the other, we have an invalid state. */
                if (has_lru ^ has_kvs) {
                    return ResultKvdbInvalidFilesystemState;
                }

                return ResultSuccess;
            }
        private:
            void RemoveOldestKey() {
                const Key &oldest_key = this->lru_list.Peek();
                this->lru_list.Pop();
                this->kvs.Remove(oldest_key);
            }
        public:
            Result Initialize(const char *dir, void *buf, size_t size) {
                /* Initialize list. */
                R_TRY(this->lru_list.Initialize(GetLeastRecentlyUsedListPath(dir), buf, size));

                /* Initialize kvs. */
                /* NOTE: Despite creating the kvs folder and returning an error if it does not exist, */
                /* Nintendo does not use the kvs folder, and instead uses the passed dir. */
                /* This causes lru_list.dat to be in the same directory as the store's .val files */
                /* instead of in the same directory as a folder containing the store's .val files. */
                /* This is probably a Nintendo bug, but because system saves contain data in the wrong */
                /* layout it can't really be fixed without breaking existing devices... */
                R_TRY(this->kvs.Initialize(dir));

                return ResultSuccess;
            }

            size_t GetCount() const {
                return this->lru_list.GetCount();
            }

            size_t GetCapacity() const {
                return Capacity;
            }

            Key GetKey(size_t i) const {
                return this->lru_list.Get(i);
            }

            bool Contains(const Key &key) const {
                return this->lru_list.Contains(key);
            }

            Result Get(size_t *out_size, void *out_value, size_t max_out_size, const Key &key) {
                /* Note that we accessed the key. */
                this->lru_list.Update(key);
                return this->kvs.Get(out_size, out_value, max_out_size, key);
            }

            template<typename Value>
            Result Get(Value *out_value, const Key &key) {
                /* Note that we accessed the key. */
                this->lru_list.Update(key);
                return this->kvs.Get(out_value, key);
            }

            Result GetSize(size_t *out_size, const Key &key) {
                return this->kvs.GetSize(out_size, key);
            }

            Result Set(const Key &key, const void *value, size_t value_size) {
                if (this->lru_list.Update(key)) {
                    /* If an entry for the key exists, delete the existing value file. */
                    this->kvs.Remove(key);
                } else {
                    /* If the list is full, we need to remove the oldest key. */
                    if (this->lru_list.IsFull()) {
                        this->RemoveOldestKey();
                    }

                    /* Add the key to the list. */
                    this->lru_list.Push(key);
                }

                /* Loop, trying to save the new value to disk. */
                while (true) {
                    /* Try to set the key. */
                    R_TRY_CATCH(this->kvs.Set(key, value, value_size)) {
                        R_CATCH_RANGE(ResultFsNotEnoughFreeSpace) {
                            /* If our entry is the only thing in the Lru list, remove it. */
                            if (this->lru_list.GetCount() == 1) {
                                this->lru_list.Pop();
                                R_TRY(this->lru_list.Save());
                                return R_TRY_CATCH_RESULT;
                            }

                            /* Otherwise, remove the oldest element from the cache and try again. */
                            this->RemoveOldestKey();
                            continue;
                        }
                    } R_END_TRY_CATCH;

                    /* If we got here, we succeeded. */
                    break;
                }

                /* Save the list. */
                R_TRY(this->lru_list.Save());

                return ResultSuccess;
            }

            template<typename Value>
            Result Set(const Key &key, const Value &value) {
                return this->Set(key, &value, sizeof(Value));
            }

            Result Remove(const Key &key) {
                /* Remove the key. */
                this->lru_list.Remove(key);
                R_TRY(this->kvs.Remove(key));
                R_TRY(this->lru_list.Save());

                return ResultSuccess;
            }

            Result RemoveAll() {
                /* TODO: Nintendo doesn't check errors here. Should we? */
                while (!this->lru_list.IsEmpty()) {
                    this->RemoveOldestKey();
                }
                R_TRY(this->lru_list.Save());

                return ResultSuccess;
            }
    };

}
