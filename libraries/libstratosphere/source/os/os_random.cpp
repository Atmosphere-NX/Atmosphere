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
#include "impl/os_random_impl.hpp"

namespace ams::os {

    namespace {

        util::TinyMT g_random;
        os::Mutex g_random_mutex(false);
        bool g_initialized_random;

        template<typename T>
        inline T GenerateRandomTImpl(T max) {
            static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value);
            const T EffectiveMax = (std::numeric_limits<T>::max() / max) * max;
            T cur_rnd;
            while (true) {
                os::GenerateRandomBytes(&cur_rnd, sizeof(T));
                if (cur_rnd < EffectiveMax) {
                    return cur_rnd % max;
                }
            }
        }

    }

    void GenerateRandomBytes(void *dst, size_t size) {
        std::scoped_lock lk(g_random_mutex);

        if (AMS_UNLIKELY(!g_initialized_random)) {
            impl::InitializeRandomImpl(&g_random);
            g_initialized_random = true;
        }

        g_random.GenerateRandomBytes(dst, size);
    }

    u32 GenerateRandomU32(u32 max) {
        return GenerateRandomTImpl<u32>(max);
    }

    u64 GenerateRandomU64(u64 max) {
        return GenerateRandomTImpl<u64>(max);
    }

}
