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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util.hpp>
#include <vapours/crypto/crypto_rsa_calculator.hpp>
#include <vapours/crypto/impl/crypto_rsa_pkcs1_impl.hpp>

namespace ams::crypto {

    template<size_t _ModulusSize, impl::HashFunction Hash>
    class RsaPkcs1Verifier {
        NON_COPYABLE(RsaPkcs1Verifier);
        NON_MOVEABLE(RsaPkcs1Verifier);
        public:
            static constexpr size_t HashSize               = Hash::HashSize;
            static constexpr size_t ModulusSize            = _ModulusSize;
            static constexpr size_t SignatureSize          = ModulusSize;
            static constexpr size_t MaximumExponentSize    = 3;
            static constexpr size_t RequiredWorkBufferSize = RsaCalculator<ModulusSize, MaximumExponentSize>::RequiredWorkBufferSize;
        private:
            enum class State {
                None,
                Initialized,
                Done,
            };
        private:
            RsaCalculator<ModulusSize, MaximumExponentSize> m_calculator;
            Hash m_hash;
            State m_state;
        public:
            RsaPkcs1Verifier() : m_state(State::None) { /* ... */ }

            bool Initialize(const void *mod, size_t mod_size, const void *exp, size_t exp_size) {
                m_hash.Initialize();
                if (m_calculator.Initialize(mod, mod_size, exp, exp_size)) {
                    m_state = State::Initialized;
                    return true;
                } else {
                    return false;
                }
            }

            void Update(const void *data, size_t size) {
                AMS_ASSERT(m_state == State::Initialized);
                return m_hash.Update(data, size);
            }

            bool Verify(const void *signature, size_t size) {
                AMS_ASSERT(m_state == State::Initialized);
                AMS_ASSERT(size == SignatureSize);
                AMS_UNUSED(size);
                ON_SCOPE_EXIT { m_state = State::Done; };

                impl::RsaPkcs1Impl<Hash> impl;
                u8 message[SignatureSize];

                return m_calculator.ExpMod(message, signature, SignatureSize) && impl.CheckPad(message, sizeof(message), std::addressof(m_hash));
            }

            bool Verify(const void *signature, size_t size, void *work_buf, size_t work_buf_size) {
                AMS_ASSERT(m_state == State::Initialized);
                AMS_ASSERT(size == SignatureSize);
                AMS_UNUSED(size);
                ON_SCOPE_EXIT { m_state = State::Done; };

                impl::RsaPkcs1Impl<Hash> impl;
                u8 message[SignatureSize];

                return m_calculator.ExpMod(message, signature, SignatureSize, work_buf, work_buf_size) && impl.CheckPad(message, sizeof(message), std::addressof(m_hash));
            }

            void GetHash(void *dst, size_t dst_size) {
                AMS_ASSERT(m_state == State::Done);

                if (m_state == State::Done) {
                    m_hash.GetHash(dst, dst_size);
                }
            }

            static bool Verify(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size) {
                RsaPkcs1Verifier<ModulusSize, Hash> verifier;
                if (!verifier.Initialize(mod, mod_size, exp, exp_size)) {
                    return false;
                }
                verifier.Update(msg, msg_size);
                return verifier.Verify(sig, sig_size);
            }

            static bool Verify(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size, void *work_buf, size_t work_buf_size) {
                RsaPkcs1Verifier<ModulusSize, Hash> verifier;
                if (!verifier.Initialize(mod, mod_size, exp, exp_size)) {
                    return false;
                }
                verifier.Update(msg, msg_size);
                return verifier.Verify(sig, sig_size, work_buf, work_buf_size);
            }
    };

}
