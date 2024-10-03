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

namespace ams::creport {

    class ScopedFile {
        NON_COPYABLE(ScopedFile);
        NON_MOVEABLE(ScopedFile);
        private:
            fs::FileHandle m_file;
            s64 m_offset;
            bool m_opened;
            u8 *m_cache;
            const size_t m_cache_size;
            s64 m_cache_offset;
        public:
            ScopedFile(const char *path, void *cache = nullptr, size_t cache_size = 0) : m_file(), m_offset(), m_opened(false), m_cache(static_cast<u8*>(cache)), m_cache_size(cache_size), m_cache_offset(0) {
                if (R_SUCCEEDED(fs::CreateFile(path, 0))) {
                    m_opened = R_SUCCEEDED(fs::OpenFile(std::addressof(m_file), path, fs::OpenMode_Write | fs::OpenMode_AllowAppend));
                }
            }

            ~ScopedFile() {
                if (m_opened) {
                    if (m_cache != nullptr) {
                        this->TryWriteCache();
                    }
                    fs::FlushFile(m_file);
                    fs::CloseFile(m_file);
                }
            }

            bool IsOpen() const {
                return m_opened;
            }

            void WriteString(const char *str);
            void WriteFormat(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
            void DumpMemory(const char *prefix, const void *data, size_t size);

            void Write(const void *data, size_t size);
        private:
            Result TryWriteCache();
    };

}