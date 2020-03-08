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
#include <stratosphere.hpp>

namespace ams::creport {

    class ScopedFile {
        NON_COPYABLE(ScopedFile);
        NON_MOVEABLE(ScopedFile);
        private:
            fs::FileHandle file;
            s64 offset;
            bool opened;
        public:
            ScopedFile(const char *path) : file(), offset(), opened(false) {
                if (R_SUCCEEDED(fs::CreateFile(path, 0))) {
                    this->opened = R_SUCCEEDED(fs::OpenFile(std::addressof(this->file), path, fs::OpenMode_Write | fs::OpenMode_AllowAppend));
                }
            }

            ~ScopedFile() {
                if (this->opened) {
                    fs::CloseFile(file);
                }
            }

            bool IsOpen() const {
                return this->opened;
            }

            void WriteString(const char *str);
            void WriteFormat(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
            void DumpMemory(const char *prefix, const void *data, size_t size);

            void Write(const void *data, size_t size);
    };

}