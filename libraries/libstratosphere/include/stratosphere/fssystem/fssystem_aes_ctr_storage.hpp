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
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>

namespace ams::fssystem {

    template<typename BasePointer>
    class AesCtrStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(AesCtrStorage);
        NON_MOVEABLE(AesCtrStorage);
        public:
            static constexpr size_t BlockSize = crypto::Aes128CtrEncryptor::BlockSize;
            static constexpr size_t KeySize   = crypto::Aes128CtrEncryptor::KeySize;
            static constexpr size_t IvSize    = crypto::Aes128CtrEncryptor::IvSize;
        private:
            BasePointer m_base_storage;
            char m_key[KeySize];
            char m_iv[IvSize];
        public:
            static void MakeIv(void *dst, size_t dst_size, u64 upper, s64 offset);
        public:
            AesCtrStorage(BasePointer base, const void *key, size_t key_size, const void *iv, size_t iv_size);

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;

            virtual Result Flush() override;

            virtual Result SetSize(s64 size) override;
            virtual Result GetSize(s64 *out) override;

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;
    };

    using AesCtrStorageByPointer       = AesCtrStorage<fs::IStorage *>;
    using AesCtrStorageBySharedPointer = AesCtrStorage<std::shared_ptr<fs::IStorage>>;

}
