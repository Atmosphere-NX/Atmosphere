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
#include <stratosphere/kvdb/kvdb_auto_buffer.hpp>

namespace ams::kvdb {

    /* Functionality for parsing/generating a key value archive. */
    class ArchiveReader {
        private:
            AutoBuffer &m_buffer;
            size_t m_offset;
        public:
            ArchiveReader(AutoBuffer &b) : m_buffer(b), m_offset(0) { /* ... */ }
        private:
            Result Peek(void *dst, size_t size);
            Result Read(void *dst, size_t size);
        public:
            Result ReadEntryCount(size_t *out);
            Result GetEntrySize(size_t *out_key_size, size_t *out_value_size);
            Result ReadEntry(void *out_key, size_t key_size, void *out_value, size_t value_size);
    };

    class ArchiveWriter {
        private:
            AutoBuffer &m_buffer;
            size_t m_offset;
        public:
            ArchiveWriter(AutoBuffer &b) : m_buffer(b), m_offset(0) { /* ... */ }
        private:
            Result Write(const void *src, size_t size);
        public:
            void WriteHeader(size_t entry_count);
            void WriteEntry(const void *key, size_t key_size, const void *value, size_t value_size);
    };

    class ArchiveSizeHelper {
        private:
            size_t m_size;
        public:
            ArchiveSizeHelper();

            void AddEntry(size_t key_size, size_t value_size);

            size_t GetSize() const {
                return m_size;
            }
    };

}