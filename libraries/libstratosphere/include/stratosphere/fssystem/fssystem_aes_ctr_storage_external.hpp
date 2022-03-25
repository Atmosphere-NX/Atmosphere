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
#include <stratosphere/fssystem/fssystem_nca_file_system_driver.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 14.3.0.0 */
    class AesCtrStorageExternal : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(AesCtrStorageExternal);
        NON_MOVEABLE(AesCtrStorageExternal);
        public:
            static constexpr size_t BlockSize = crypto::Aes128CtrEncryptor::BlockSize;
            static constexpr size_t KeySize   = crypto::Aes128CtrEncryptor::KeySize;
            static constexpr size_t IvSize    = crypto::Aes128CtrEncryptor::IvSize;
        private:
            std::shared_ptr<fs::IStorage> m_base_storage;
            u8 m_iv[IvSize];
            DecryptAesCtrFunction m_decrypt_function;
            s32 m_key_index;
            s32 m_key_generation;
            u8 m_encrypted_key[KeySize];
        public:
            AesCtrStorageExternal(std::shared_ptr<fs::IStorage> bs, const void *enc_key, size_t enc_key_size, const void *iv, size_t iv_size, DecryptAesCtrFunction df, s32 kidx, s32 kgen);

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;
            virtual Result GetSize(s64 *out) override;
            virtual Result Flush() override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;
            virtual Result SetSize(s64 size) override;
    };

}
