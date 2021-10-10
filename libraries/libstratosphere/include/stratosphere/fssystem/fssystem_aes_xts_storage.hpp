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
#include <stratosphere/os.hpp>

namespace ams::fssystem {

    class AesXtsStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(AesXtsStorage);
        NON_MOVEABLE(AesXtsStorage);
        public:
            static constexpr size_t AesBlockSize = crypto::Aes128XtsEncryptor::BlockSize;
            static constexpr size_t KeySize      = crypto::Aes128XtsEncryptor::KeySize;
            static constexpr size_t IvSize       = crypto::Aes128XtsEncryptor::IvSize;
        private:
            IStorage * const m_base_storage;
            char m_key[2][KeySize];
            char m_iv[IvSize];
            const size_t m_block_size;
            os::SdkMutex m_mutex;
        public:
            AesXtsStorage(IStorage *base, const void *key1, const void *key2, size_t key_size, const void *iv, size_t iv_size, size_t block_size);

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;

            virtual Result Flush() override;

            virtual Result SetSize(s64 size) override;
            virtual Result GetSize(s64 *out) override;

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;
    };

}
