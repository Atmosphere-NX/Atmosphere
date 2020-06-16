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
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <stratosphere/kvdb/kvdb_bounded_string.hpp>
#include <stratosphere/kvdb/kvdb_file_key_value_store.hpp>

namespace ams::kvdb {

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
                    R_TRY(fs::CreateFile(path, FileSize));

                    /* Open the file. */
                    fs::FileHandle file;
                    R_TRY(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Write));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    /* Write new header with zero entries to the file. */
                    LruHeader new_header = { .entry_count = 0, };
                    R_TRY(fs::WriteFile(file, 0, std::addressof(new_header), sizeof(new_header), fs::WriteOption::Flush));

                    return ResultSuccess();
                }
            private:
                void RemoveIndex(size_t i) {
                    AMS_ABORT_UNLESS(i < this->GetCount());
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
                    AMS_ABORT_UNLESS(this->keys == nullptr);
                    AMS_ABORT_UNLESS(size >= BufferSize);

                    /* Setup member variables. */
                    this->keys = static_cast<Key *>(buf);
                    this->file_path.Set(path);
                    std::memset(this->keys, 0, BufferSize);

                    /* Open file. */
                    fs::FileHandle file;
                    R_TRY(fs::OpenFile(std::addressof(file), this->file_path, fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    /* Read header. */
                    R_TRY(fs::ReadFile(file, 0, std::addressof(this->header), sizeof(this->header)));

                    /* Read entries. */
                    R_TRY(fs::ReadFile(file, sizeof(this->header), this->keys, BufferSize));

                    return ResultSuccess();
                }

                Result Save() {
                    /* Open file. */
                    fs::FileHandle file;
                    R_TRY(fs::OpenFile(std::addressof(file), this->file_path, fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    /* Write header. */
                    R_TRY(fs::WriteFile(file, 0, std::addressof(this->header), sizeof(this->header), fs::WriteOption::None));

                    /* Write entries. */
                    R_TRY(fs::WriteFile(file, sizeof(this->header), this->keys, BufferSize, fs::WriteOption::None));

                    /* Flush. */
                    R_TRY(fs::FlushFile(file));

                    return ResultSuccess();
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
                    AMS_ABORT_UNLESS(i < this->GetCount());
                    return this->keys[i];
                }

                Key Peek() const {
                    AMS_ABORT_UNLESS(!this->IsEmpty());
                    return this->Get(0);
                }

                void Push(const Key &key) {
                    AMS_ABORT_UNLESS(!this->IsFull());
                    this->keys[this->GetCount()] = key;
                    this->IncrementCount();
                }

                Key Pop() {
                    AMS_ABORT_UNLESS(!this->IsEmpty());
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
        static_assert(util::is_pod<Key>::value, "FileKeyValueCache Key must be pod!");
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

            static Result Exists(bool *out, const char *path, fs::DirectoryEntryType type) {
                /* Set out to false initially. */
                *out = false;

                /* Try to get the entry type. */
                fs::DirectoryEntryType entry_type;
                R_TRY_CATCH(fs::GetEntryType(std::addressof(entry_type), path)) {
                    /* If the path doesn't exist, nothing has gone wrong. */
                    R_CONVERT(fs::ResultPathNotFound, ResultSuccess());
                } R_END_TRY_CATCH;

                /* Check that the entry type is correct. */
                R_UNLESS(entry_type == type, ResultInvalidFilesystemState());

                /* The entry exists and is the correct type. */
                *out = true;
                return ResultSuccess();
            }

            static Result DirectoryExists(bool *out, const char *path) {
                return Exists(out, path, fs::DirectoryEntryType_Directory);
            }

            static Result FileExists(bool *out, const char *path) {
                return Exists(out, path, fs::DirectoryEntryType_File);
            }
        public:
            static Result CreateNewCache(const char *dir) {
                /* Make a new key value store filesystem, and a new lru_list.dat. */
                R_TRY(LeastRecentlyUsedList::CreateNewList(GetLeastRecentlyUsedListPath(dir)));
                R_TRY(fs::CreateDirectory(dir));

                return ResultSuccess();
            }

            static Result ValidateExistingCache(const char *dir) {
                /* Check for existence. */
                bool has_lru = false, has_kvs = false;
                R_TRY(FileExists(&has_lru, GetLeastRecentlyUsedListPath(dir)));
                R_TRY(DirectoryExists(&has_kvs, GetFileKeyValueStorePath(dir)));

                /* If neither exists, CreateNewCache was never called. */
                R_UNLESS(has_lru || has_kvs, ResultNotCreated());

                /* If one exists but not the other, we have an invalid state. */
                R_UNLESS(has_lru && has_kvs, ResultInvalidFilesystemState());

                return ResultSuccess();
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

                return ResultSuccess();
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
                        R_CATCH(fs::ResultNotEnoughFreeSpace) {
                            /* If our entry is the only thing in the Lru list, remove it. */
                            if (this->lru_list.GetCount() == 1) {
                                this->lru_list.Pop();
                                R_TRY(this->lru_list.Save());
                                return fs::ResultNotEnoughFreeSpace();
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

                return ResultSuccess();
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

                return ResultSuccess();
            }

            Result RemoveAll() {
                /* TODO: Nintendo doesn't check errors here. Should we? */
                while (!this->lru_list.IsEmpty()) {
                    this->RemoveOldestKey();
                }
                R_TRY(this->lru_list.Save());

                return ResultSuccess();
            }
    };

}
