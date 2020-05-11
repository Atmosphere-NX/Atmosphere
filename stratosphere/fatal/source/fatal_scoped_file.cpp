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
#include <stratosphere.hpp>
#include "fatal_scoped_file.hpp"

namespace ams::fatal::srv {

    namespace {

        /* Convenience definitions. */
        constexpr size_t MaximumLineLength = 0x20;

        os::Mutex g_format_lock(false);
        char g_format_buffer[2 * os::MemoryPageSize];

    }

    void ScopedFile::WriteString(const char *str) {
        this->Write(str, std::strlen(str));
    }

    void ScopedFile::WriteFormat(const char *fmt, ...) {
        /* Acquire exclusive access to the format buffer. */
        std::scoped_lock lk(g_format_lock);

        /* Format to the buffer. */
        {
            std::va_list vl;
            va_start(vl, fmt);
            std::vsnprintf(g_format_buffer, sizeof(g_format_buffer), fmt, vl);
            va_end(vl);
        }

        /* Write data. */
        this->WriteString(g_format_buffer);
    }

    void ScopedFile::DumpMemory(const char *prefix, const void *data, size_t size) {
        const u8 *data_u8 = reinterpret_cast<const u8 *>(data);
        const int prefix_len = std::strlen(prefix);
        size_t data_ofs = 0;
        size_t remaining = size;
        bool first = true;

        while (remaining) {
            const size_t cur_size = std::min(MaximumLineLength, remaining);

            /* Print the line prefix. */
            if (first) {
                this->WriteFormat("%s", prefix);
                first = false;
            } else {
                this->WriteFormat("%*s", prefix_len, "");
            }

            /* Dump up to 0x20 of hex memory. */
            {
                char hex[MaximumLineLength * 2 + 2] = {};
                for (size_t i = 0; i < cur_size; i++) {
                    std::snprintf(hex + i * 2, 3, "%02X", data_u8[data_ofs++]);
                }
                hex[cur_size * 2 + 0] = '\n';
                hex[cur_size * 2 + 1] = '\x00';

                this->WriteString(hex);
            }

            /* Continue. */
            remaining -= cur_size;
        }
    }

    void ScopedFile::Write(const void *data, size_t size) {
        /* If we're not open, we can't write. */
        if (!this->IsOpen()) {
            return;
        }

        /* Advance, if we write successfully. */
        if (R_SUCCEEDED(fs::WriteFile(this->file, this->offset, data, size, fs::WriteOption::Flush))) {
            this->offset += size;
        }
    }

}