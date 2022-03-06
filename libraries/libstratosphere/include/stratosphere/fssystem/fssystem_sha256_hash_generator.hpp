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

    class Sha256HashGenerator final : public ::ams::fssystem::IHash256Generator, public ::ams::fs::impl::Newable {
        NON_COPYABLE(Sha256HashGenerator);
        NON_MOVEABLE(Sha256HashGenerator);
        private:
            crypto::Sha256Generator m_generator;
        public:
            Sha256HashGenerator() = default;
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

    class Sha256HashGeneratorFactory final : public IHash256GeneratorFactory, public ::ams::fs::impl::Newable {
        NON_COPYABLE(Sha256HashGeneratorFactory);
        NON_MOVEABLE(Sha256HashGeneratorFactory);
        public:
            Sha256HashGeneratorFactory() = default;
        protected:
            virtual std::unique_ptr<IHash256Generator> DoCreate() override {
                return std::unique_ptr<IHash256Generator>(new Sha256HashGenerator());
            }

            virtual void DoGenerateHash(void *dst, size_t dst_size, const void *src, size_t src_size) override {
                crypto::GenerateSha256(dst, dst_size, src, src_size);
            }
    };

    class Sha256HashGeneratorFactorySelector final : public IHash256GeneratorFactorySelector, public ::ams::fs::impl::Newable {
        NON_COPYABLE(Sha256HashGeneratorFactorySelector);
        NON_MOVEABLE(Sha256HashGeneratorFactorySelector);
        private:
            Sha256HashGeneratorFactory m_factory;
        public:
            Sha256HashGeneratorFactorySelector() = default;
        protected:
            virtual IHash256GeneratorFactory *DoGetFactory() override {
                return std::addressof(m_factory);
            }
    };

}
