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
                Path m_file_path;
                Key *m_keys;
                LruHeader m_header;
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
                    std::memmove(m_keys + i, m_keys + i + 1, sizeof(*m_keys) * (this->GetCount() - (i + 1)));
                    this->DecrementCount();
                }

                void IncrementCount() {
                    m_header.entry_count++;
                }

                void DecrementCount() {
                    m_header.entry_count--;
                }
            public:
                LruList() : m_keys(nullptr), m_header() { /* ... */ }

                Result Initialize(const char *path, void *buf, size_t size) {
                    /* Only initialize once, and ensure we have sufficient memory. */
                    AMS_ABORT_UNLESS(m_keys == nullptr);
                    AMS_ABORT_UNLESS(size >= BufferSize);

                    /* Setup member variables. */
                    m_keys = static_cast<Key *>(buf);
                    m_file_path.Set(path);
                    std::memset(m_keys, 0, BufferSize);

                    /* Open file. */
                    fs::FileHandle file;
                    R_TRY(fs::OpenFile(std::addressof(file), m_file_path, fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    /* Read header. */
                    R_TRY(fs::ReadFile(file, 0, std::addressof(m_header), sizeof(m_header)));

                    /* Read entries. */
                    R_TRY(fs::ReadFile(file, sizeof(m_header), m_keys, BufferSize));

                    return ResultSuccess();
                }

                Result Save() {
                    /* Open file. */
                    fs::FileHandle file;
                    R_TRY(fs::OpenFile(std::addressof(file), m_file_path, fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    /* Write header. */
                    R_TRY(fs::WriteFile(file, 0, std::addressof(m_header), sizeof(m_header), fs::WriteOption::None));

                    /* Write entries. */
                    R_TRY(fs::WriteFile(file, sizeof(m_header), m_keys, BufferSize, fs::WriteOption::None));

                    /* Flush. */
                    R_TRY(fs::FlushFile(file));

                    return ResultSuccess();
                }

                size_t GetCount() const {
                    return m_header.entry_count;
                }

                bool IsEmpty() const {
                    return this->GetCount() == 0;
                }

                bool IsFull() const {
                    return this->GetCount() >= Capacity;
                }

                Key Get(size_t i) const {
                    AMS_ABORT_UNLESS(i < this->GetCount());
                    return m_keys[i];
                }

                Key Peek() const {
                    AMS_ABORT_UNLESS(!this->IsEmpty());
                    return this->Get(0);
                }

                void Push(const Key &key) {
                    AMS_ABORT_UNLESS(!this->IsFull());
                    m_keys[this->GetCount()] = key;
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
                        if (m_keys[count - 1 - i] == key) {
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
                        if (m_keys[count - 1 - i] == key) {
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
            FileKeyValueStore m_kvs;
            LeastRecentlyUsedList m_lru_list;
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
                R_UNLESS(entry_type == type, kvdb::ResultInvalidFilesystemState());

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
                R_TRY(FileExists(std::addressof(has_lru), GetLeastRecentlyUsedListPath(dir)));
                R_TRY(DirectoryExists(std::addressof(has_kvs), GetFileKeyValueStorePath(dir)));

                /* If neither exists, CreateNewCache was never called. */
                R_UNLESS(has_lru || has_kvs, kvdb::ResultNotCreated());

                /* If one exists but not the other, we have an invalid state. */
                R_UNLESS(has_lru && has_kvs, kvdb::ResultInvalidFilesystemState());

                return ResultSuccess();
            }
        private:
            void RemoveOldestKey() {
                const Key &oldest_key = m_lru_list.Peek();
                m_lru_list.Pop();
                m_kvs.Remove(oldest_key);
            }
        public:
            Result Initialize(const char *dir, void *buf, size_t size) {
                /* Initialize list. */
                R_TRY(m_lru_list.Initialize(GetLeastRecentlyUsedListPath(dir), buf, size));

                /* Initialize kvs. */
                /* NOTE: Despite creating the kvs folder and returning an error if it does not exist, */
                /* Nintendo does not use the kvs folder, and instead uses the passed dir. */
                /* This causes lru_list.dat to be in the same directory as the store's .val files */
                /* instead of in the same directory as a folder containing the store's .val files. */
                /* This is probably a Nintendo bug, but because system saves contain data in the wrong */
                /* layout it can't really be fixed without breaking existing devices... */
                R_TRY(m_kvs.Initialize(dir));

                return ResultSuccess();
            }

            size_t GetCount() const {
                return m_lru_list.GetCount();
            }

            size_t GetCapacity() const {
                return Capacity;
            }

            Key GetKey(size_t i) const {
                return m_lru_list.Get(i);
            }

            bool Contains(const Key &key) const {
                return m_lru_list.Contains(key);
            }

            Result Get(size_t *out_size, void *out_value, size_t max_out_size, const Key &key) {
                /* Note that we accessed the key. */
                m_lru_list.Update(key);
                return m_kvs.Get(out_size, out_value, max_out_size, key);
            }

            template<typename Value>
            Result Get(Value *out_value, const Key &key) {
                /* Note that we accessed the key. */
                m_lru_list.Update(key);
                return m_kvs.Get(out_value, key);
            }

            Result GetSize(size_t *out_size, const Key &key) {
                return m_kvs.GetSize(out_size, key);
            }

            Result Set(const Key &key, const void *value, size_t value_size) {
                if (m_lru_list.Update(key)) {
                    /* If an entry for the key exists, delete the existing value file. */
                    m_kvs.Remove(key);
                } else {
                    /* If the list is full, we need to remove the oldest key. */
                    if (m_lru_list.IsFull()) {
                        this->RemoveOldestKey();
                    }

                    /* Add the key to the list. */
                    m_lru_list.Push(key);
                }

                /* Loop, trying to save the new value to disk. */
                while (true) {
                    /* Try to set the key. */
                    R_TRY_CATCH(m_kvs.Set(key, value, value_size)) {
                        R_CATCH(fs::ResultNotEnoughFreeSpace) {
                            /* If our entry is the only thing in the Lru list, remove it. */
                            if (m_lru_list.GetCount() == 1) {
                                m_lru_list.Pop();
                                R_TRY(m_lru_list.Save());
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
                R_TRY(m_lru_list.Save());

                return ResultSuccess();
            }

            template<typename Value>
            Result Set(const Key &key, const Value &value) {
                return this->Set(key, &value, sizeof(Value));
            }

            Result Remove(const Key &key) {
                /* Remove the key. */
                m_lru_list.Remove(key);
                R_TRY(m_kvs.Remove(key));
                R_TRY(m_lru_list.Save());

                return ResultSuccess();
            }

            Result RemoveAll() {
                /* TODO: Nintendo doesn't check errors here. Should we? */
                while (!m_lru_list.IsEmpty()) {
                    this->RemoveOldestKey();
                }
                R_TRY(m_lru_list.Save());

                return ResultSuccess();
            }
    };

}
