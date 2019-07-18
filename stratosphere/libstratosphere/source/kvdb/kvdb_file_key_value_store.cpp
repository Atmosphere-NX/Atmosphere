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
#include <stratosphere.hpp>
#include <stratosphere/kvdb/kvdb_file_key_value_store.hpp>

namespace sts::kvdb {

    /* Cache implementation. */
    void *FileKeyValueStore::Cache::Allocate(size_t size) {
        if (this->backing_buffer_size - this->backing_buffer_free_offset < size) {
            return nullptr;
        }
        ON_SCOPE_EXIT { this->backing_buffer_free_offset += size; };
        return this->backing_buffer + this->backing_buffer_free_offset;
    }

    Result FileKeyValueStore::Cache::Initialize(void *buffer, size_t buffer_size, size_t capacity) {
        this->backing_buffer = static_cast<u8 *>(buffer);
        this->backing_buffer_size = buffer_size;
        this->backing_buffer_free_offset = 0;
        this->entries = nullptr;
        this->count = 0;
        this->capacity = capacity;

        /* If we have memory to work with, ensure it's at least enough for the cache entries. */
        if (this->backing_buffer != nullptr) {
            this->entries = static_cast<decltype(this->entries)>(this->Allocate(sizeof(*this->entries) * this->capacity));
            if (this->entries == nullptr) {
                return ResultKvdbBufferInsufficient;
            }
        }

        return ResultSuccess;
    }

    void FileKeyValueStore::Cache::Invalidate() {
        if (!this->HasEntries()) {
            return;
        }

        /* Reset the allocation pool. */
        this->backing_buffer_free_offset = 0;
        this->count = 0;
        this->entries = static_cast<decltype(this->entries)>(this->Allocate(sizeof(*this->entries) * this->capacity));
        if (this->entries == nullptr) {
            std::abort();
        }
    }

    std::optional<size_t> FileKeyValueStore::Cache::TryGet(void *out_value, size_t max_out_size, const void *key, size_t key_size) {
        if (!this->HasEntries()) {
            return std::nullopt;
        }

        /* Try to find the entry. */
        for (size_t i = 0; i < this->count; i++) {
            const auto &entry = this->entries[i];
            if (entry.key_size == key_size && std::memcmp(entry.key, key, key_size) == 0) {
                /* If we don't have enough space, fail to read from cache. */
                if (max_out_size < entry.value_size) {
                    return std::nullopt;
                }

                std::memcpy(out_value, entry.value, entry.value_size);
                return entry.value_size;
            }
        }

        return std::nullopt;
    }

    std::optional<size_t> FileKeyValueStore::Cache::TryGetSize(const void *key, size_t key_size) {
        if (!this->HasEntries()) {
            return std::nullopt;
        }

        /* Try to find the entry. */
        for (size_t i = 0; i < this->count; i++) {
            const auto &entry = this->entries[i];
            if (entry.key_size == key_size && std::memcmp(entry.key, key, key_size) == 0) {
                return entry.value_size;
            }
        }

        return std::nullopt;
    }

    void FileKeyValueStore::Cache::Set(const void *key, size_t key_size, const void *value, size_t value_size) {
        if (!this->HasEntries()) {
            return;
        }

        /* Ensure key size is small enough. */
        if (key_size > MaxKeySize) {
            std::abort();
        }

        /* If we're at capacity, invalidate the cache. */
        if (this->count == this->capacity) {
            this->Invalidate();
        }

        /* Allocate memory for the value. */
        void *value_buf = this->Allocate(value_size);
        if (value_buf == nullptr) {
            /* We didn't have enough memory for the value. Invalidating might get us enough memory. */
            this->Invalidate();
            value_buf = this->Allocate(value_size);
            if (value_buf == nullptr) {
                /* If we still don't have enough memory, just fail to put the value in the cache. */
                return;
            }
        }

        auto &entry = this->entries[this->count++];
        std::memcpy(entry.key, key, key_size);
        entry.key_size = key_size;
        entry.value = value_buf;
        std::memcpy(entry.value, value, value_size);
        entry.value_size = value_size;
    }

    bool FileKeyValueStore::Cache::Contains(const void *key, size_t key_size) {
        return this->TryGetSize(key, key_size).has_value();
    }

    /* Store functionality. */
    FileKeyValueStore::Path FileKeyValueStore::GetPath(const void *_key, size_t key_size) {
        /* Format is "<dir>/<hex formatted key>.val" */
        FileKeyValueStore::Path key_path(this->dir_path.Get());
        key_path.Append('/');

        /* Append hex formatted key. */
        const u8 *key = static_cast<const u8 *>(_key);
        for (size_t i = 0; i < key_size; i++) {
            key_path.AppendFormat("%02x", key[i]);
        }

        /* Append extension. */
        key_path.Append(FileExtension);

        return key_path;
    }

    Result FileKeyValueStore::GetKey(size_t *out_size, void *_out_key, size_t max_out_size, const FileKeyValueStore::FileName &file_name) {
        /* Validate that the filename can be converted to a key. */
        /* TODO: Nintendo does not validate that the key is valid hex. Should we do this? */
        const size_t file_name_len = file_name.GetLength();
        const size_t key_name_len = file_name_len - FileExtensionLength;
        if (file_name_len < FileExtensionLength + 2 || !file_name.EndsWith(FileExtension) || key_name_len % 2 != 0) {
            return ResultKvdbInvalidKeyValue;
        }

        /* Validate that we have space for the converted key. */
        const size_t key_size = key_name_len / 2;
        if (key_size > max_out_size) {
            return ResultKvdbBufferInsufficient;
        }

        /* Convert the hex key back. */
        u8 *out_key = static_cast<u8 *>(_out_key);
        for (size_t i = 0; i < key_size; i++) {
            char substr[2 * sizeof(u8) + 1];
            file_name.GetSubstring(substr, sizeof(substr), 2 * i, sizeof(substr) - 1);
            out_key[i] = static_cast<u8>(std::strtoul(substr, nullptr, 0x10));
        }

        *out_size = key_size;
        return ResultSuccess;
    }

    Result FileKeyValueStore::Initialize(const char *dir) {
        return this->InitializeWithCache(dir, nullptr, 0, 0);
    }

    Result FileKeyValueStore::InitializeWithCache(const char *dir, void *cache_buffer, size_t cache_buffer_size, size_t cache_capacity) {
        /* Ensure that the passed path is a directory. */
        {
            struct stat st;
            if (stat(dir, &st) != 0 || !(S_ISDIR(st.st_mode))) {
                return ResultFsPathNotFound;
            }
        }

        /* Set path. */
        this->dir_path.Set(dir);

        /* Initialize our cache. */
        R_TRY(this->cache.Initialize(cache_buffer, cache_buffer_size, cache_capacity));
        return ResultSuccess;
    }

    Result FileKeyValueStore::Get(size_t *out_size, void *out_value, size_t max_out_size, const void *key, size_t key_size) {
        std::scoped_lock lk(this->lock);

        /* Ensure key size is small enough. */
        if (key_size > MaxKeySize) {
            return ResultKvdbKeyCapacityInsufficient;
        }

        /* Try to get from cache. */
        {
            auto size = this->cache.TryGet(out_value, max_out_size, key, key_size);
            if (size) {
                *out_size = *size;
                return ResultSuccess;
            }
        }

        /* Open the value file. */
        FILE *fp = fopen(this->GetPath(key, key_size), "rb");
        if (fp == nullptr) {
            R_TRY_CATCH(fsdevGetLastResult()) {
                R_CATCH(ResultFsPathNotFound) {
                    return ResultKvdbKeyNotFound;
                }
            } R_END_TRY_CATCH;
        }
        ON_SCOPE_EXIT { fclose(fp); };

        /* Get the value size. */
        fseek(fp, 0, SEEK_END);
        const size_t value_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        /* Ensure there's enough space for the value. */
        if (max_out_size < value_size) {
            return ResultKvdbBufferInsufficient;
        }

        /* Read the value. */
        if (fread(out_value, value_size, 1, fp) != 1) {
            return fsdevGetLastResult();
        }
        *out_size = value_size;

        /* Cache the newly read value. */
        this->cache.Set(key, key_size, out_value, value_size);
        return ResultSuccess;
    }

    Result FileKeyValueStore::GetSize(size_t *out_size, const void *key, size_t key_size) {
        std::scoped_lock lk(this->lock);

        /* Ensure key size is small enough. */
        if (key_size > MaxKeySize) {
            return ResultKvdbKeyCapacityInsufficient;
        }

        /* Try to get from cache. */
        {
            auto size = this->cache.TryGetSize(key, key_size);
            if (size) {
                *out_size = *size;
                return ResultSuccess;
            }
        }

        /* Open the value file. */
        FILE *fp = fopen(this->GetPath(key, key_size), "rb");
        if (fp == nullptr) {
            R_TRY_CATCH(fsdevGetLastResult()) {
                R_CATCH(ResultFsPathNotFound) {
                    return ResultKvdbKeyNotFound;
                }
            } R_END_TRY_CATCH;
        }
        ON_SCOPE_EXIT { fclose(fp); };

        /* Get the value size. */
        fseek(fp, 0, SEEK_END);
        *out_size = ftell(fp);
        return ResultSuccess;
    }

    Result FileKeyValueStore::Set(const void *key, size_t key_size, const void *value, size_t value_size) {
        std::scoped_lock lk(this->lock);

        /* Ensure key size is small enough. */
        if (key_size > MaxKeySize) {
            return ResultKvdbKeyCapacityInsufficient;
        }

        /* When the cache contains the key being set, Nintendo invalidates the cache. */
        if (this->cache.Contains(key, key_size)) {
            this->cache.Invalidate();
        }

        /* Delete the file, if it exists. Don't check result, since it's okay if it's already deleted. */
        auto key_path = this->GetPath(key, key_size);
        std::remove(key_path);

        /* Open the value file. */
        FILE *fp = fopen(key_path, "wb");
        if (fp == nullptr) {
            return fsdevGetLastResult();
        }
        ON_SCOPE_EXIT { fclose(fp); };

        /* Write the value file. */
        if (fwrite(value, value_size, 1, fp) != 1) {
            return fsdevGetLastResult();
        }

        /* Flush the value file. */
        fflush(fp);

        return ResultSuccess;
    }

    Result FileKeyValueStore::Remove(const void *key, size_t key_size) {
        std::scoped_lock lk(this->lock);

        /* Ensure key size is small enough. */
        if (key_size > MaxKeySize) {
            return ResultKvdbKeyCapacityInsufficient;
        }

        /* When the cache contains the key being set, Nintendo invalidates the cache. */
        if (this->cache.Contains(key, key_size)) {
            this->cache.Invalidate();
        }

        /* Remove the file. */
        if (std::remove(this->GetPath(key, key_size)) != 0) {
            R_TRY_CATCH(fsdevGetLastResult()) {
                R_CATCH(ResultFsPathNotFound) {
                    return ResultKvdbKeyNotFound;
                }
            } R_END_TRY_CATCH;
        }

        return ResultSuccess;
    }

}