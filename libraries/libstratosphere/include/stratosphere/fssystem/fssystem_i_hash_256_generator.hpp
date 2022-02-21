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

                return this->GetHash(dst, dst_size);
            }
        protected:
            virtual void DoInitialize() = 0;
            virtual void DoUpdate(const void *data, size_t size) = 0;
            virtual void DoGetHash(void *dst, size_t dst_size) = 0;
    };

    class IHash256GeneratorFactory {
        public:
            constexpr IHash256GeneratorFactory() = default;
            virtual constexpr ~IHash256GeneratorFactory() { /* ... */ }

            std::unique_ptr<IHash256Generator> Create() {
                return this->DoCreate();
            }

            void GenerateHash(void *dst, size_t dst_size, const void *src, size_t src_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(dst != nullptr);
                AMS_ASSERT(src != nullptr);
                AMS_ASSERT(dst_size == IHash256Generator::HashSize);

                return this->DoGenerateHash(dst, dst_size, src, src_size);
            }
        protected:
            virtual std::unique_ptr<IHash256Generator> DoCreate() = 0;
            virtual void DoGenerateHash(void *dst, size_t dst_size, const void *src, size_t src_size) = 0;
    };

    class IHash256GeneratorFactorySelector {
        public:
            constexpr IHash256GeneratorFactorySelector() = default;
            virtual constexpr ~IHash256GeneratorFactorySelector() { /* ... */ }

            IHash256GeneratorFactory *GetFactory() { return this->DoGetFactory(); }
        protected:
            virtual IHash256GeneratorFactory *DoGetFactory() = 0;
    };

}
