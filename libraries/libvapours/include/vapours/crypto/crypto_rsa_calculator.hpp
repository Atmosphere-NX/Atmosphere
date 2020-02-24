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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util.hpp>
#include <vapours/crypto/impl/crypto_bignum.hpp>

namespace ams::crypto {

    template<size_t ModulusSize, size_t ExponentSize>
    class RsaCalculator {
        NON_COPYABLE(RsaCalculator);
        NON_MOVEABLE(RsaCalculator);
        public:
            static constexpr inline size_t RequiredWorkBufferSize = 0x10 * ModulusSize;
        private:
            impl::StaticBigNum<ModulusSize * BITSIZEOF(u8)> modulus;
            impl::StaticBigNum<ExponentSize * BITSIZEOF(u8)> exponent;
        public:
            RsaCalculator() { /* ... */ }
            ~RsaCalculator() { this->exponent.ClearToZero(); }

            bool Initialize(const void *mod, size_t mod_size, const void *exp, size_t exp_size) {
                if (!this->modulus.Import(mod, mod_size) || this->modulus.IsZero()) {
                    return false;
                }
                if (!this->exponent.Import(exp, exp_size) || this->exponent.IsZero()) {
                    return false;
                }
                return true;
            }

            bool ExpMod(void *dst, const void *src, size_t size, void *work_buf, size_t work_buf_size) {
                AMS_ASSERT(work_buf_size >= RequiredWorkBufferSize);

                return this->modulus.ExpMod(dst, src, size, this->exponent, static_cast<u32 *>(work_buf), work_buf_size);
            }

            bool ExpMod(void *dst, const void *src, size_t size) {
                u32 work_buf[RequiredWorkBufferSize / sizeof(u32)];
                return this->ExpMod(dst, src, size, work_buf, sizeof(work_buf));
            }
    };

}
