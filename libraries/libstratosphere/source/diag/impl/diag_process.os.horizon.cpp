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

/* TODO: Rename, if we change to e.g. use amsMain? */
extern "C" int main(int argc, char **argv);

namespace ams::diag::impl {

    uintptr_t GetModuleInfoForHorizon(const char **out_path, size_t *out_path_length, size_t *out_module_size, uintptr_t address);

    namespace {

        const char *GetLastCharacterPointer(const char *str, size_t len, char c) {
            for (const char *last = str + len - 1; last >= str; --last) {
                if (*last == c) {
                    return last;
                }
            }
            return nullptr;
        }

        void GetFileNameWithoutExtension(const char **out, size_t *out_size, const char *path, size_t path_length) {
            const auto last_sep1 = GetLastCharacterPointer(path, path_length, '\\');
            const auto last_sep2 = GetLastCharacterPointer(path, path_length, '/');
            const auto ext       = GetLastCharacterPointer(path, path_length, '.');

            /* Handle last-separator. */
            if (last_sep1 && last_sep2) {
                if (last_sep1 > last_sep2) {
                    *out = last_sep1 + 1;
                } else {
                    *out = last_sep2 + 1;
                }
            } else if (last_sep1) {
                *out = last_sep1 + 1;
            } else if (last_sep2) {
                *out = last_sep2 + 1;
            } else {
                *out = path;
            }

            /* Handle extension. */
            if (ext && ext >= *out) {
                *out_size = ext - *out;
            } else {
                *out_size = (path + path_length) - *out;
            }
        }

        constinit const char *g_process_name = nullptr;
        constinit size_t g_process_name_size = 0;
        constinit os::SdkMutex g_process_name_lock;
        constinit bool g_got_process_name = false;

        void EnsureProcessNameCached() {
            /* Ensure process name. */
            if (AMS_UNLIKELY(!g_got_process_name)) {
                std::scoped_lock lk(g_process_name_lock);
                if (AMS_LIKELY(!g_got_process_name)) {
                    const char *path;
                    size_t path_length;
                    size_t module_size;

                    if (GetModuleInfoForHorizon(std::addressof(path), std::addressof(path_length), std::addressof(module_size), reinterpret_cast<uintptr_t>(main)) != 0) {
                        GetFileNameWithoutExtension(std::addressof(g_process_name), std::addressof(g_process_name_size), path, path_length);
                        AMS_ASSERT(g_process_name_size == 0 || util::VerifyUtf8String(g_process_name, g_process_name_size));
                    } else {
                        g_process_name = "";
                        g_process_name_size = 0;
                    }
                }
            }
        }

    }

    void GetProcessNamePointer(const char **out, size_t *out_size) {
        /* Ensure process name is cached. */
        EnsureProcessNameCached();

        /* Get cached process name. */
        *out      = g_process_name;
        *out_size = g_process_name_size;
    }

}
