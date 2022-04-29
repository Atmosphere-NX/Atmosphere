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
#include <vapours.hpp>

namespace ams::fssystem {

    enum HashAlgorithmType : u8 {
        HashAlgorithmType_Sha2 = 0,
        HashAlgorithmType_Sha3 = 1,
    };

    /* ACCURATE_TO_VERSION: 14.3.0.0 */
    class IHash256Generator {
        public:
            static constexpr size_t HashSize = 256 / BITSIZEOF(u8);
        public:
            constexpr IHash256Generator() = default;
            virtual constexpr ~IHash256Generator() { /* ... */ }
        public:
            void Initialize() {
                return this->DoInitialize();
            }

            void Update(const void *data, size_t size) {
                /* Check pre-conditions. */
                AMS_ASSERT(data != nullptr);

                return this->DoUpdate(data, size);
            }

            void GetHash(void *dst, size_t dst_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(dst_size == HashSize);

                return this->DoGetHash(dst, dst_size);
            }
        protected:
            virtual void DoInitialize() = 0;
            virtual void DoUpdate(const void *data, size_t size) = 0;
            virtual void DoGetHash(void *dst, size_t dst_size) = 0;
    };

    /* ACCURATE_TO_VERSION: 14.3.0.0 */
    class IHash256GeneratorFactory {
        public:
            constexpr IHash256GeneratorFactory() = default;
            virtual constexpr ~IHash256GeneratorFactory() { /* ... */ }

            Result Create(std::unique_ptr<IHash256Generator> *out) {
                return this->DoCreate(out);
            }

            void GenerateHash(void *dst, size_t dst_size, const void *src, size_t src_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(dst != nullptr);
                AMS_ASSERT(src != nullptr);
                AMS_ASSERT(dst_size == IHash256Generator::HashSize);

                return this->DoGenerateHash(dst, dst_size, src, src_size);
            }
        protected:
            virtual Result DoCreate(std::unique_ptr<IHash256Generator> *out) = 0;
            virtual void DoGenerateHash(void *dst, size_t dst_size, const void *src, size_t src_size) = 0;
    };

    /* ACCURATE_TO_VERSION: 14.3.0.0 */
    class IHash256GeneratorFactorySelector {
        public:
            constexpr IHash256GeneratorFactorySelector() = default;
            virtual constexpr ~IHash256GeneratorFactorySelector() { /* ... */ }

            IHash256GeneratorFactory *GetFactory(HashAlgorithmType alg) { return this->DoGetFactory(alg); }
        protected:
            virtual IHash256GeneratorFactory *DoGetFactory(HashAlgorithmType alg) = 0;
    };

}
