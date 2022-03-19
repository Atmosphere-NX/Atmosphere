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

namespace ams::ncm {

    class FileMapperFile {
        public:
            enum class OpenMode {
                Read,
                ReadWrite,
                ReadWriteAppend,
            };
        private:
            const char *m_path;
            OpenMode m_mode;
            util::optional<fs::FileHandle> m_file;
            size_t m_file_size;
            size_t m_max_size;
        public:
            FileMapperFile() : m_file(util::nullopt) { /* ... */ }

            ~FileMapperFile() {
                this->Finalize();
            }

            Result Initialize(const char *path, OpenMode mode) {
                /* Set our path/mode. */
                m_path = path;
                m_mode = mode;

                /* Ensure we're open. */
                R_TRY(this->EnsureOpen());

                /* Get the file size. */
                s64 size;
                R_TRY(fs::GetFileSize(std::addressof(size), m_file.value()));

                /* Set our file size/loaded size. */
                m_file_size = static_cast<size_t>(size);
                m_max_size  = static_cast<size_t>(size);

                R_SUCCEED();
            }

            void Finalize() {
                /* If we have a file, close (and flush) it. */
                if (m_file.has_value()) {
                    if (m_mode != OpenMode::Read) {
                        R_ABORT_UNLESS(fs::FlushFile(m_file.value()));
                    }
                    fs::CloseFile(m_file.value());
                    m_file = util::nullopt;
                }
            }

            size_t GetFileSize() const { return m_file_size; }
            size_t GetMaxSize() const { return m_max_size; }

            Result Read(size_t offset, void *dst, size_t size) {
                /* Determine the end offset. */
                const size_t end_offset = offset + size;

                /* Unless we're allowed to append, we need to have a big enough file. */
                if (m_mode != OpenMode::ReadWriteAppend) {
                    R_UNLESS(end_offset <= m_file_size, ncm::ResultMapperInvalidArgument());
                }

                /* Clear the output. */
                std::memset(dst, 0, size);

                /* Check that our offset is valid. */
                R_UNLESS(offset <= m_file_size, ncm::ResultMapperInvalidArgument());

                /* Ensure we're open. */
                R_TRY(this->EnsureOpen());

                /* Read what we can. */
                const size_t read_size = (offset + size >= m_file_size) ? (m_file_size - offset) : size;
                AMS_ASSERT(read_size >= size);

                R_TRY(fs::ReadFile(m_file.value(), offset, dst, read_size));

                /* Update our max size. */
                m_max_size = std::max<size_t>(m_max_size, offset + read_size);

                R_SUCCEED();
            }
        private:
            Result EnsureOpen() {
                /* If we've opened, we're done. */
                R_SUCCEED_IF(m_file.has_value());

                /* Open based on our mode. */
                fs::FileHandle file;
                switch (m_mode) {
                    case OpenMode::Read:            R_TRY(fs::OpenFile(std::addressof(file), m_path, fs::OpenMode_Read));      break;
                    case OpenMode::ReadWrite:       R_TRY(fs::OpenFile(std::addressof(file), m_path, fs::OpenMode_ReadWrite)); break;
                    case OpenMode::ReadWriteAppend: R_TRY(fs::OpenFile(std::addressof(file), m_path, fs::OpenMode_All));       break;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }

                /* Set our file. */
                m_file = file;
                R_SUCCEED();
            }
    };

}