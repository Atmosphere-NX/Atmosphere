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

namespace ams::crypto {

    namespace {

        bool g_initialized;
        os::Mutex g_lock(false);

        void InitializeCsrng() {
            AMS_ASSERT(!g_initialized);
            sm::DoWithSession([&]() {
                R_ABORT_UNLESS(::csrngInitialize());
            });
        }

    }

    void GenerateCryptographicallyRandomBytes(void *dst, size_t dst_size) {
        {
            std::scoped_lock lk(g_lock);

            if (AMS_UNLIKELY(!g_initialized)) {
                InitializeCsrng();
                g_initialized = true;
            }
        }

        R_ABORT_UNLESS(csrngGetRandomBytes(dst, dst_size));
    }

}
