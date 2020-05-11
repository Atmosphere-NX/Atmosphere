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
#include <stratosphere/os.hpp>
#include <stratosphere/fs/fs_directory.hpp>
#include <stratosphere/kvdb/kvdb_bounded_string.hpp>

namespace ams::kvdb {

    class FileKeyValueStore {
        NON_COPYABLE(FileKeyValueStore);
        NON_MOVEABLE(FileKeyValueStore);
        public:
            static constexpr size_t MaxFileLength = 0xFF;
            static constexpr char FileExtension[5] = ".val";
            static constexpr size_t FileExtensionLength = sizeof(FileExtension) - 1;
            static constexpr size_t MaxKeySize = (MaxFileLength - FileExtensionLength) / 2;
            using Path = kvdb::BoundedString<fs::EntryNameLengthMax>;
            using FileName = kvdb::BoundedString<MaxFileLength>;
        private:
            /* Subtypes. */
            struct Entry {
                u8 key[MaxKeySize];
                void *value;
                size_t key_size;
                size_t value_size;
            };
            static_assert(util::is_pod<Entry>::value, "FileKeyValueStore::Entry definition!");

            class Cache {
                private:
                    u8 *backing_buffer = nullptr;
                    size_t backing_buffer_size = 0;
                    size_t backing_buffer_free_offset = 0;
                    Entry *entries = nullptr;
                    size_t count = 0;
                    size_t capacity = 0;
                private:
                    void *Allocate(size_t size);

                    bool HasEntries() const {
                        return this->entries != nullptr && this->capacity != 0;
                    }
                public:
                    Result Initialize(void *buffer, size_t buffer_size, size_t capacity);
                    void Invalidate();
                    std::optional<size_t> TryGet(void *out_value, size_t max_out_size, const void *key, size_t key_size);
                    std::optional<size_t> TryGetSize(const void *key, size_t key_size);
                    void Set(const void *key, size_t key_size, const void *value, size_t value_size);
                    bool Contains(const void *key, size_t key_size);
            };
        private:
            os::Mutex lock;
            Path dir_path;
            Cache cache;
        private:
            Path GetPath(const void *key, size_t key_size);
            Result GetKey(size_t *out_size, void *out_key, size_t max_out_size, const FileName &file_name);
        public:
            FileKeyValueStore() : lock(false) { /* ... */ }

            /* Basic accessors. */
            Result Initialize(const char *dir);
            Result InitializeWithCache(const char *dir, void *cache_buffer, size_t cache_buffer_size, size_t cache_capacity);
            Result Get(size_t *out_size, void *out_value, size_t max_out_size, const void *key, size_t key_size);
            Result GetSize(size_t *out_size, const void *key, size_t key_size);
            Result Set(const void *key, size_t key_size, const void *value, size_t value_size);
            Result Remove(const void *key, size_t key_size);

            /* Niceties. */
            template<typename Key>
            Result Get(size_t *out_size, void *out_value, size_t max_out_size, const Key &key) {
                static_assert(util::is_pod<Key>::value && sizeof(Key) <= MaxKeySize, "Invalid FileKeyValueStore Key!");
                return this->Get(out_size, out_value, max_out_size, &key, sizeof(Key));
            }

            template<typename Key, typename Value>
            Result Get(Value *out_value, const Key &key) {
                static_assert(util::is_pod<Value>::value && !std::is_pointer<Value>::value, "Invalid FileKeyValueStore Value!");
                size_t size = 0;
                R_TRY(this->Get(&size, out_value, sizeof(Value), key));
                AMS_ABORT_UNLESS(size >= sizeof(Value));
                return ResultSuccess();
            }

            template<typename Key>
            Result GetSize(size_t *out_size, const Key &key) {
                return this->GetSize(out_size, &key, sizeof(Key));
            }

            template<typename Key>
            Result Set(const Key &key, const void *value, size_t value_size) {
                static_assert(util::is_pod<Key>::value && sizeof(Key) <= MaxKeySize, "Invalid FileKeyValueStore Key!");
                return this->Set(&key, sizeof(Key), value, value_size);
            }

            template<typename Key, typename Value>
            Result Set(const Key &key, const Value &value) {
                static_assert(util::is_pod<Value>::value && !std::is_pointer<Value>::value, "Invalid FileKeyValueStore Value!");
                return this->Set(key, &value, sizeof(Value));
            }

            template<typename Key>
            Result Remove(const Key &key) {
                return this->Remove(&key, sizeof(Key));
            }
    };

}