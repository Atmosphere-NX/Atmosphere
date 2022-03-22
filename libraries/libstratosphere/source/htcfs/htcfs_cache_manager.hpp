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
#pragma once
#include <stratosphere.hpp>

namespace ams::htcfs {

    class CacheManager {
        private:
            os::SdkMutex m_mutex;
            void *m_cache;
            size_t m_cache_size;
            s64 m_cached_file_size;
            size_t m_cached_data_size;
            s32 m_cached_handle;
            bool m_has_cached_handle;
        public:
            CacheManager(void *cache, size_t cache_size) : m_mutex(), m_cache(cache), m_cache_size(cache_size), m_cached_file_size(), m_cached_data_size(), m_cached_handle(), m_has_cached_handle() { /* ... */ }
        public:
            bool GetFileSize(s64 *out, s32 handle) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Get the cached size, if we match. */
                if (m_has_cached_handle && m_cached_handle == handle) {
                    *out = m_cached_file_size;
                    return true;
                } else {
                    return false;
                }
            }

            void Invalidate() {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Note that we have no handle. */
                m_has_cached_handle = false;
            }

            void Invalidate(s32 handle) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                if (m_has_cached_handle && m_cached_handle == handle) {
                    /* Note that we have no handle. */
                    m_has_cached_handle = false;
                }
            }

            void Record(s64 file_size, const void *data, s32 handle, size_t data_size) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Set our cached file size. */
                m_cached_file_size = file_size;

                /* Set our cached data size. */
                m_cached_data_size = std::min(m_cache_size, data_size);

                /* Copy the data. */
                std::memcpy(m_cache, data, m_cached_data_size);

                /* Set our cache handle. */
                m_cached_handle = handle;

                /* Note that we have cached data. */
                m_has_cached_handle = true;
            }

            bool ReadFile(size_t *out, void *dst, s32 handle, size_t offset, size_t size) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Check that we have a cached file. */
                if (!m_has_cached_handle) {
                    return false;
                }

                /* Check the file is our cached one. */
                if (handle != m_cached_handle) {
                    return false;
                }

                /* Check that we can read data. */
                if (offset + size > m_cached_data_size) {
                    return false;
                }

                /* Copy the cached data. */
                std::memcpy(dst, static_cast<const u8 *>(m_cache) + offset, size);

                /* Set the output read size. */
                *out = size;

                return true;
            }
    };

}
