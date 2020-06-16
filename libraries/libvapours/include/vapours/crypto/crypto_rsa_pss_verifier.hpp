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
#include <vapours/crypto/crypto_rsa_calculator.hpp>
#include <vapours/crypto/impl/crypto_rsa_pss_impl.hpp>

namespace ams::crypto {

    template<size_t _ModulusSize, typename Hash> requires impl::HashFunction<Hash>
    class RsaPssVerifier {
        NON_COPYABLE(RsaPssVerifier);
        NON_MOVEABLE(RsaPssVerifier);
        public:
            static constexpr size_t HashSize               = Hash::HashSize;
            static constexpr size_t SaltSize               = Hash::HashSize;
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
            RsaCalculator<ModulusSize, MaximumExponentSize> calculator;
            Hash hash;
            State state;
        public:
            RsaPssVerifier() : state(State::None) { /* ... */ }
            ~RsaPssVerifier() { }

            bool Initialize(const void *mod, size_t mod_size, const void *exp, size_t exp_size) {
                this->hash.Initialize();
                if (this->calculator.Initialize(mod, mod_size, exp, exp_size)) {
                    this->state = State::Initialized;
                    return true;
                } else {
                    return false;
                }
            }

            void Update(const void *data, size_t size) {
                return this->hash.Update(data, size);
            }

            bool Verify(const void *signature, size_t size) {
                AMS_ASSERT(this->state == State::Initialized);
                AMS_ASSERT(size == SignatureSize);
                ON_SCOPE_EXIT { this->state = State::Done; };

                impl::RsaPssImpl<Hash> impl;
                u8 message[SignatureSize];
                ON_SCOPE_EXIT { ClearMemory(message, sizeof(message)); };

                if (!this->calculator.ExpMod(message, signature, SignatureSize)) {
                    return false;
                }

                u8 calc_hash[Hash::HashSize];
                this->hash.GetHash(calc_hash, sizeof(calc_hash));
                ON_SCOPE_EXIT { ClearMemory(calc_hash, sizeof(calc_hash)); };

                return impl.Verify(message, sizeof(message), calc_hash, sizeof(calc_hash));
            }

            bool Verify(const void *signature, size_t size, void *work_buf, size_t work_buf_size) {
                AMS_ASSERT(this->state == State::Initialized);
                AMS_ASSERT(size == SignatureSize);
                ON_SCOPE_EXIT { this->state = State::Done; };

                impl::RsaPssImpl<Hash> impl;
                u8 message[SignatureSize];
                ON_SCOPE_EXIT { ClearMemory(message, sizeof(message)); };

                if (!this->calculator.ExpMod(message, signature, SignatureSize, work_buf, work_buf_size)) {
                    return false;
                }

                u8 calc_hash[Hash::HashSize];
                this->hash.GetHash(calc_hash, sizeof(calc_hash));
                ON_SCOPE_EXIT { ClearMemory(calc_hash, sizeof(calc_hash)); };

                return impl.Verify(message, sizeof(message), calc_hash, sizeof(calc_hash));
            }

            bool VerifyWithHash(const void *signature, size_t size, const void *hash, size_t hash_size) {
                AMS_ASSERT(this->state == State::Initialized);
                AMS_ASSERT(size == SignatureSize);
                ON_SCOPE_EXIT { this->state = State::Done; };

                impl::RsaPssImpl<Hash> impl;
                u8 message[SignatureSize];
                ON_SCOPE_EXIT { ClearMemory(message, sizeof(message)); };

                if (!this->calculator.ExpMod(message, signature, SignatureSize)) {
                    return false;
                }

                return impl.Verify(message, sizeof(message), static_cast<const u8 *>(hash), hash_size);
            }

            bool VerifyWithHash(const void *signature, size_t size, const void *hash, size_t hash_size, void *work_buf, size_t work_buf_size) {
                AMS_ASSERT(this->state == State::Initialized);
                AMS_ASSERT(size == SignatureSize);
                ON_SCOPE_EXIT { this->state = State::Done; };

                impl::RsaPssImpl<Hash> impl;
                u8 message[SignatureSize];
                ON_SCOPE_EXIT { ClearMemory(message, sizeof(message)); };

                if (!this->calculator.ExpMod(message, signature, SignatureSize, work_buf, work_buf_size)) {
                    return false;
                }

                return impl.Verify(message, sizeof(message), static_cast<const u8 *>(hash), hash_size);
            }

            static bool Verify(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size) {
                RsaPssVerifier<ModulusSize, Hash> verifier;
                if (!verifier.Initialize(mod, mod_size, exp, exp_size)) {
                    return false;
                }
                verifier.Update(msg, msg_size);
                return verifier.Verify(sig, sig_size);
            }

            static bool Verify(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size, void *work_buf, size_t work_buf_size) {
                RsaPssVerifier<ModulusSize, Hash> verifier;
                if (!verifier.Initialize(mod, mod_size, exp, exp_size)) {
                    return false;
                }
                verifier.Update(msg, msg_size);
                return verifier.Verify(sig, sig_size, work_buf, work_buf_size);
            }

            static bool VerifyWithHash(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *hash, size_t hash_size) {
                RsaPssVerifier<ModulusSize, Hash> verifier;
                if (!verifier.Initialize(mod, mod_size, exp, exp_size)) {
                    return false;
                }
                return verifier.VerifyWithHash(sig, sig_size, hash, hash_size);
            }

            static bool VerifyWithHash(const void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *hash, size_t hash_size, void *work_buf, size_t work_buf_size) {
                RsaPssVerifier<ModulusSize, Hash> verifier;
                if (!verifier.Initialize(mod, mod_size, exp, exp_size)) {
                    return false;
                }
                return verifier.VerifyWithHash(sig, sig_size, hash, hash_size, work_buf, work_buf_size);
            }
    };

}
