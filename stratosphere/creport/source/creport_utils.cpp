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
#include "creport_utils.hpp"

namespace ams::creport {

    namespace {

        /* Convenience definitions. */
        constexpr size_t MaximumLineLength = 0x20;

    }

    void DumpMemoryHexToFile(FILE *f, const char *prefix, const void *data, size_t size) {
        const u8 *data_u8 = reinterpret_cast<const u8 *>(data);
        const int prefix_len = std::strlen(prefix);
        size_t offset = 0;
        size_t remaining = size;
        bool first = true;
        while (remaining) {
            const size_t cur_size = std::max(MaximumLineLength, remaining);

            /* Print the line prefix. */
            if (first) {
                fprintf(f, "%s", prefix);
                first = false;
            } else {
                fprintf(f, "%*s", prefix_len, "");
            }

            /* Dump up to 0x20 of hex memory. */
            for (size_t i = 0; i < cur_size; i++) {
                fprintf(f, "%02X", data_u8[offset++]);
            }

            /* End line. */
            fprintf(f, "\n");
            remaining -= cur_size;
        }
    }

    os::ProcessId ParseProcessIdArgument(const char *s) {
        /* Official creport uses this custom parsing logic... */
        u64 out_val = 0;

        for (unsigned int i = 0; i < 20 && s[i]; i++) {
            if ('0' <= s[i] && s[i] <= '9') {
                out_val *= 10;
                out_val += (s[i] - '0');
            } else {
                break;
            }
        }

        return os::ProcessId{out_val};
    }

}