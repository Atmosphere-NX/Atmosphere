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
#include <stratosphere/fssystem/fssystem_i_hash_256_generator.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 14.3.0.0 */

    namespace impl {

        template<typename Traits>
        class ShaHashGenerator final : public ::ams::fssystem::IHash256Generator, public ::ams::fs::impl::Newable {
            static_assert(Traits::Generator::HashSize == IHash256Generator::HashSize);
            NON_COPYABLE(ShaHashGenerator);
            NON_MOVEABLE(ShaHashGenerator);
            private:
                using Generator = typename Traits::Generator;
            private:
                Generator m_generator;
            public:
                ShaHashGenerator() = default;
            protected:
                virtual void DoInitialize() override {
                    m_generator.Initialize();
                }

                virtual void DoUpdate(const void *data, size_t size) override {
                    m_generator.Update(data, size);
                }

                virtual void DoGetHash(void *dst, size_t dst_size) override {
                    m_generator.GetHash(dst, dst_size);
                }
        };

        template<typename Traits>
        class ShaHashGeneratorFactory final : public IHash256GeneratorFactory, public ::ams::fs::impl::Newable {
            static_assert(Traits::Generator::HashSize == IHash256Generator::HashSize);
            NON_COPYABLE(ShaHashGeneratorFactory);
            NON_MOVEABLE(ShaHashGeneratorFactory);
            public:
                constexpr ShaHashGeneratorFactory() = default;
            protected:
                virtual Result DoCreate(std::unique_ptr<IHash256Generator> *out) override {
                    auto generator = std::unique_ptr<IHash256Generator>(new ShaHashGenerator<Traits>());
                    R_UNLESS(generator != nullptr, fs::ResultAllocationMemoryFailedNew());

                    *out = std::move(generator);
                    R_SUCCEED();
                }

                virtual void DoGenerateHash(void *dst, size_t dst_size, const void *src, size_t src_size) override {
                    Traits::Generate(dst, dst_size, src, src_size);
                }
        };

        struct Sha256Traits {
            using Generator = crypto::Sha256Generator;

            static ALWAYS_INLINE void Generate(void *dst, size_t dst_size, const void *src, size_t src_size) {
                return crypto::GenerateSha256(dst, dst_size, src, src_size);
            }
        };

        struct Sha3256Traits {
            using Generator = crypto::Sha3256Generator;

            static ALWAYS_INLINE void Generate(void *dst, size_t dst_size, const void *src, size_t src_size) {
                return crypto::GenerateSha3256(dst, dst_size, src, src_size);
            }
        };
    }

    using Sha256HashGenerator        = impl::ShaHashGenerator<impl::Sha256Traits>;
    using Sha256HashGeneratorFactory = impl::ShaHashGeneratorFactory<impl::Sha256Traits>;

    using Sha3256HashGenerator        = impl::ShaHashGenerator<impl::Sha3256Traits>;
    using Sha3256HashGeneratorFactory = impl::ShaHashGeneratorFactory<impl::Sha3256Traits>;

    class ShaHashGeneratorFactorySelector final : public IHash256GeneratorFactorySelector, public ::ams::fs::impl::Newable {
        NON_COPYABLE(ShaHashGeneratorFactorySelector);
        NON_MOVEABLE(ShaHashGeneratorFactorySelector);
        private:
            Sha256HashGeneratorFactory m_sha256_factory;
            Sha3256HashGeneratorFactory m_sha3_256_factory;
        public:
            constexpr ShaHashGeneratorFactorySelector() = default;
        protected:
            virtual IHash256GeneratorFactory *DoGetFactory(HashAlgorithmType alg) override {
                switch (alg) {
                    case HashAlgorithmType_Sha2: return std::addressof(m_sha256_factory);
                    case HashAlgorithmType_Sha3: return std::addressof(m_sha3_256_factory);
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }
    };

}
