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
        public:
            ScopedFile(const char *path) : m_file(), m_offset(), m_opened(false) {
                if (R_SUCCEEDED(fs::CreateFile(path, 0))) {
                    m_opened = R_SUCCEEDED(fs::OpenFile(std::addressof(m_file), path, fs::OpenMode_Write | fs::OpenMode_AllowAppend));
                }
            }

            ~ScopedFile() {
                if (m_opened) {
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
    };

}