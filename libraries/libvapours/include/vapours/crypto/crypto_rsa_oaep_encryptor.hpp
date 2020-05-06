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
#include <vapours/crypto/impl/crypto_rsa_oaep_impl.hpp>

namespace ams::crypto {

    template<size_t ModulusSize, typename Hash> requires impl::HashFunction<Hash>
    class RsaOaepEncryptor {
        NON_COPYABLE(RsaOaepEncryptor);
        NON_MOVEABLE(RsaOaepEncryptor);
        public:
            static constexpr size_t HashSize               = Hash::HashSize;
            static constexpr size_t BlockSize              = ModulusSize;
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
            bool set_label_digest;
            u8   label_digest[HashSize];
            State state;
        public:
            RsaOaepEncryptor() : set_label_digest(false), state(State::None) { std::memset(this->label_digest, 0, sizeof(this->label_digest)); }

            ~RsaOaepEncryptor() {
                ClearMemory(this->label_digest, sizeof(this->label_digest));
            }

            bool Initialize(const void *mod, size_t mod_size, const void *exp, size_t exp_size) {
                this->hash.Initialize();
                this->set_label_digest = false;
                if (this->calculator.Initialize(mod, mod_size, exp, exp_size)) {
                    this->state = State::Initialized;
                    return true;
                } else {
                    return false;
                }
            }

            void UpdateLabel(const void *data, size_t size) {
                AMS_ASSERT(this->state == State::Initialized);

                this->hash.Update(data, size);
            }

            void SetLabelDigest(const void *digest, size_t digest_size) {
                AMS_ASSERT(this->state == State::Initialized);
                AMS_ABORT_UNLESS(digest_size == sizeof(this->label_digest));

                std::memcpy(this->label_digest, digest, digest_size);
                this->set_label_digest = true;
            }

            bool Encrypt(void *dst, size_t dst_size, const void *src, size_t src_size, const void *salt, size_t salt_size) {
                AMS_ASSERT(this->state == State::Initialized);

                impl::RsaOaepImpl<Hash> impl;
                if (!this->set_label_digest) {
                    this->hash.GetHash(this->label_digest, sizeof(this->label_digest));
                }

                impl.Encode(dst, dst_size, this->label_digest, sizeof(this->label_digest), src, src_size, salt, salt_size);

                if (!this->calculator.ExpMod(dst, dst, dst_size)) {
                    std::memset(dst, 0, dst_size);
                    return false;
                }

                this->state = State::Done;
                return true;
            }

            bool Encrypt(void *dst, size_t dst_size, const void *src, size_t src_size, const void *salt, size_t salt_size, void *work, size_t work_size) {
                AMS_ASSERT(this->state == State::Initialized);

                impl::RsaOaepImpl<Hash> impl;
                if (!this->set_label_digest) {
                    this->hash.GetHash(this->label_digest, sizeof(this->label_digest));
                }

                impl.Encode(dst, dst_size, this->label_digest, sizeof(this->label_digest), src, src_size, salt, salt_size);

                if (!this->calculator.ExpMod(dst, dst, dst_size, work, work_size)) {
                    std::memset(dst, 0, dst_size);
                    return false;
                }

                this->state = State::Done;
                return true;
            }

            static bool Encrypt(void *dst, size_t dst_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size, const void *seed, size_t seed_size, const void *lab, size_t lab_size) {
                RsaOaepEncryptor<ModulusSize, Hash> oaep;
                if (!oaep.Initialize(mod, mod_size, exp, exp_size)) {
                    return false;
                }
                oaep.UpdateLabel(lab, lab_size);
                return oaep.Encrypt(dst, dst_size, msg, msg_size, seed, seed_size);
            }

            static bool Encrypt(void *dst, size_t dst_size, const void *mod, size_t mod_size, const void *exp, size_t exp_size, const void *msg, size_t msg_size, const void *seed, size_t seed_size, const void *lab, size_t lab_size, void *work, size_t work_size) {
                RsaOaepEncryptor<ModulusSize, Hash> oaep;
                if (!oaep.Initialize(mod, mod_size, exp, exp_size)) {
                    return false;
                }
                oaep.UpdateLabel(lab, lab_size);
                return oaep.Encrypt(dst, dst_size, msg, msg_size, seed, seed_size, work, work_size);
            }

    };

}
