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
#include <stratosphere.hpp>
#include "settings_static_object.hpp"
#include "settings_system_data.hpp"

namespace ams::settings::impl {

    namespace {

        constexpr inline const char MountNameSeparator[] = ":";
        constexpr inline const char DirectoryNameSeparator[] = "/";
        constexpr inline const char SystemDataFileName[] = "file";

        class LazyFileAccessor final {
            NON_COPYABLE(LazyFileAccessor);
            NON_MOVEABLE(LazyFileAccessor);
            private:
                bool m_is_cached;
                fs::FileHandle m_file;
                s64 m_offset;
                size_t m_size;
                u8 m_buffer[16_KB];
            public:
                LazyFileAccessor() : m_is_cached(false), m_file(), m_offset(), m_size(), m_buffer() {
                    /* ... */
                }

                Result Open(const char *path, int mode);
                void Close();
                Result Read(s64 offset, void *dst, size_t size);
            private:
                bool GetCache(s64 offset, void *dst, size_t size);
        };

        Result LazyFileAccessor::Open(const char *path, int mode) {
            /* Open our file. */
            return fs::OpenFile(std::addressof(m_file), path, mode);
        }

        void LazyFileAccessor::Close() {
            /* Close our file. */
            fs::CloseFile(m_file);

            /* Reset our state. */
            m_is_cached = false;
            m_file      = {};
            m_size      = 0;
            m_offset    = 0;
        }

        bool LazyFileAccessor::GetCache(s64 offset, void *dst, size_t size) {
            /* Check pre-conditions. */
            AMS_ASSERT(offset >= 0);
            AMS_ASSERT(dst != nullptr);

            /* Check that we're cached. */
            if (!m_is_cached) {
                return false;
            }

            /* Check that offset is within cache. */
            if (offset < m_offset) {
                return false;
            }

            /* Check that the read remains within range. */
            const size_t offset_in_cache = offset - m_offset;
            if (m_size < offset_in_cache + size) {
                return false;
            }

            /* Copy the cached data. */
            std::memcpy(dst, m_buffer + offset_in_cache, size);
            return true;
        }

        Result LazyFileAccessor::Read(s64 offset, void *dst, size_t size) {
            /* Check pre-conditions. */
            AMS_ASSERT(offset >= 0);
            AMS_ASSERT(dst != nullptr);

            /* Try to read from cache. */
            R_SUCCEED_IF(this->GetCache(offset, dst, size));

            /* If the read is too big for the cache, read the data directly. */
            if (size > sizeof(m_buffer)) {
                return fs::ReadFile(m_file, offset, dst, size);
            }

            /* Get the file size. */
            s64 file_size;
            R_TRY(fs::GetFileSize(std::addressof(file_size), m_file));

            /* If the file is too small, read the data directly. */
            if (file_size < offset + static_cast<s64>(size)) {
                return fs::ReadFile(m_file, offset, dst, size);
            }

            /* Determine the read size. */
            const size_t read_size = std::min<size_t>(file_size - offset, sizeof(m_buffer));

            /* Read into the cache. */
            if (read_size > 0) {
                R_TRY(fs::ReadFile(m_file, offset, m_buffer, read_size));
            }

            /* Update cache statement. */
            m_offset    = offset;
            m_size      = read_size;
            m_is_cached = true;

            /* Get the data from the cache. */
            const bool succeeded = this->GetCache(offset, dst, size);
            AMS_ASSERT(succeeded);
            AMS_UNUSED(succeeded);

            return ResultSuccess();
        }

        LazyFileAccessor &GetLazyFileAccessor() {
            return StaticObject<LazyFileAccessor, void>::Get();
        }

    }

    void SystemData::SetSystemDataId(ncm::SystemDataId id) {
        m_system_data_id = id;
    }

    void SystemData::SetMountName(const char *name) {
        util::Strlcpy(m_mount_name, name, sizeof(m_mount_name));

        int pos = 0;
        pos += util::Strlcpy(m_file_path + pos, name, sizeof(m_mount_name));
        pos += util::Strlcpy(m_file_path + pos, MountNameSeparator, sizeof(MountNameSeparator));
        pos += util::Strlcpy(m_file_path + pos, DirectoryNameSeparator, sizeof(DirectoryNameSeparator));
        pos += util::Strlcpy(m_file_path + pos, SystemDataFileName, sizeof(SystemDataFileName));
    }

    Result SystemData::Mount() {
        return fs::MountSystemData(m_mount_name, m_system_data_id);
    }

    Result SystemData::OpenToRead() {
        return GetLazyFileAccessor().Open(m_file_path, fs::OpenMode_Read);
    }

    void SystemData::Close() {
        return GetLazyFileAccessor().Close();
    }

    Result SystemData::Read(s64 offset, void *dst, size_t size) {
        return GetLazyFileAccessor().Read(offset, dst, size);
    }

}
