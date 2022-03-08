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
#include "../spl/impl/spl_ctr_drbg.hpp"
#include <sys/random.h>

namespace ams::crypto {

    namespace {

        using Drbg = ::ams::spl::impl::CtrDrbg<crypto::AesEncryptor128, crypto::AesEncryptor128::KeySize, false>;

        constinit util::TypedStorage<Drbg> g_drbg = {};

        bool InitializeCsrng() {
            u8 seed[Drbg::SeedSize];
            AMS_ABORT_UNLESS(::getentropy(seed, sizeof(seed)) == 0);

            util::ConstructAt(g_drbg);
            util::GetReference(g_drbg).Initialize(seed, sizeof(seed), nullptr, 0, nullptr, 0);

            return true;
        }

    }

    void GenerateCryptographicallyRandomBytes(void *dst, size_t dst_size) {
        AMS_FUNCTION_LOCAL_STATIC(bool, s_initialized, InitializeCsrng());
        AMS_ABORT_UNLESS(s_initialized);

        AMS_ASSERT(dst_size <= Drbg::RequestSizeMax);

        if (!util::GetReference(g_drbg).Generate(dst, dst_size, nullptr, 0)) {
            /* Reseed, if needed. */
            {
                u8 seed[Drbg::SeedSize];
                AMS_ABORT_UNLESS(::getentropy(seed, sizeof(seed)) == 0);

                util::GetReference(g_drbg).Reseed(seed, sizeof(seed), nullptr, 0);
            }

            util::GetReference(g_drbg).Generate(dst, dst_size, nullptr, 0);
        }
    }

}
