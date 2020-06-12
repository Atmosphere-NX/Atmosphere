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
#include <exosphere.hpp>
#include "../secmon_error.hpp"
#include "secmon_smc_device_unique_data.hpp"

namespace ams::secmon::smc {

    namespace {

        void GenerateIv(void *dst, size_t dst_size) {
            /* Flush the region we're about to fill to ensure consistency with the SE. */
            hw::FlushDataCache(dst, dst_size);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Generate random bytes. */
            se::GenerateRandomBytes(dst, dst_size);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Flush to ensure the CPU sees consistent data for the region. */
            hw::FlushDataCache(dst, dst_size);
            hw::DataSynchronizationBarrierInnerShareable();
        }

        void PrepareDeviceUniqueDataKey(const void *seal_key_source, size_t seal_key_source_size, const void *access_key, size_t access_key_size, const void *key_source, size_t key_source_size) {
            /* Derive the seal key. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, pkg1::AesKeySlot_RandomForUserWrap, seal_key_source, seal_key_source_size);

            /* Derive the device unique data kek. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, pkg1::AesKeySlot_Smc, access_key, access_key_size);

            /* Derive the actual device unique data key. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, pkg1::AesKeySlot_Smc, key_source, key_source_size);
        }

        void ComputeAes128Ctr(void *dst, size_t dst_size, int slot, const void *src, size_t src_size, const void *iv, size_t iv_size) {
            /* Ensure that the SE sees consistent data. */
            hw::FlushDataCache(src, src_size);
            hw::FlushDataCache(dst, dst_size);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Use the security engine to transform the data. */
            se::ComputeAes128Ctr(dst, dst_size, slot, src, src_size, iv, iv_size);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Ensure the CPU sees consistent data. */
            hw::FlushDataCache(dst, dst_size);
            hw::DataSynchronizationBarrierInnerShareable();
        }

        void ComputeGmac(void *dst, size_t dst_size, const void *data, size_t data_size, const void *iv, size_t iv_size) {
            /* Declare keyslot (as encryptor will need to take it by pointer/reference). */
            constexpr int Slot = pkg1::AesKeySlot_Smc;

            /* Calculate the mac. */
            crypto::Aes128GcmEncryptor gcm;
            gcm.Initialize(std::addressof(Slot), sizeof(Slot), iv, iv_size);
            gcm.UpdateAad(data, data_size);
            gcm.GetMac(dst, dst_size);
        }

        constexpr u64 GetDeviceIdLow(u64 device_id) {
            /* Mask out the top byte. */
            constexpr u64 ByteMask = (static_cast<u64>(1) << BITSIZEOF(u8)) - 1;
            constexpr u64 LowMask  = ~(ByteMask << (BITSIZEOF(u64) - BITSIZEOF(u8)));
            return device_id & LowMask;
        }

        constexpr u8 GetDeviceIdHigh(u64 device_id) {
            /* Get the top byte. */
            return static_cast<u8>(device_id >> (BITSIZEOF(u64) - BITSIZEOF(u8)));
        }

        constexpr u64 EncodeDeviceId(u8 device_id_high, u64 device_id_low) {
            return (static_cast<u64>(device_id_high) << (BITSIZEOF(u64) - BITSIZEOF(u8))) | device_id_low;
        }

    }

    bool DecryptDeviceUniqueData(void *dst, size_t dst_size, u8 *out_device_id_high, const void *seal_key_source, size_t seal_key_source_size, const void *access_key, size_t access_key_size, const void *key_source, size_t key_source_size, const void *src, size_t src_size, bool enforce_device_unique) {
        /* Determine how much decrypted data there will be. */
        const size_t enc_size = src_size - (enforce_device_unique ? DeviceUniqueDataOuterMetaSize : DeviceUniqueDataIvSize);
        const size_t dec_size = enc_size - DeviceUniqueDataInnerMetaSize;

        /* Ensure that our sizes are allowed. */
        AMS_ABORT_UNLESS(src_size > (enforce_device_unique ? DeviceUniqueDataTotalMetaSize : DeviceUniqueDataIvSize));
        AMS_ABORT_UNLESS(dst_size >= enc_size);

        /* Determine the extents of the data. */
        const u8 * const iv  = static_cast<const u8 *>(src);
        const u8 * const enc = iv  + DeviceUniqueDataIvSize;
        const u8 * const mac = enc + enc_size;

        /* Decrypt the data. */
        {
            /* Declare temporaries. */
            u8 temp_iv[DeviceUniqueDataIvSize];
            u8 calc_mac[DeviceUniqueDataMacSize];
            ON_SCOPE_EXIT { crypto::ClearMemory(temp_iv, sizeof(temp_iv)); crypto::ClearMemory(calc_mac, sizeof(calc_mac)); };

            /* Prepare the key used to decrypt the data. */
            PrepareDeviceUniqueDataKey(seal_key_source, seal_key_source_size, access_key, access_key_size, key_source, key_source_size);

            /* Copy the iv to stack. */
            std::memcpy(temp_iv, iv, sizeof(temp_iv));

            /* Decrypt the data. */
            ComputeAes128Ctr(dst, dst_size, pkg1::AesKeySlot_Smc, enc, enc_size, temp_iv, DeviceUniqueDataIvSize);

            /* If we're not enforcing device unique, there's no mac/device id. */
            if (!enforce_device_unique) {
                return true;
            }

            /* Compute the gmac. */
            ComputeGmac(calc_mac, DeviceUniqueDataMacSize, dst, enc_size, temp_iv, DeviceUniqueDataIvSize);

            /* Validate the gmac. */
            if (!crypto::IsSameBytes(mac, calc_mac, sizeof(calc_mac))) {
                return false;
            }
        }

        /* Validate device id, output device id if needed. */
        {
            /* Locate the device id in the decryption output. */
            const u8 * const padding   = static_cast<const u8 *>(dst) + dec_size;
            const u8 * const device_id = padding + DeviceUniqueDataPaddingSize;

            /* Load the big endian device id. */
            const u64 device_id_val = util::LoadBigEndian(static_cast<const u64 *>(static_cast<const void *>(device_id)));

            /* Validate that the device id low matches the value in fuses. */
            if (GetDeviceIdLow(device_id_val) != fuse::GetDeviceId()) {
                return false;
            }

            /* Set the output device id high, if needed. */
            if (out_device_id_high != nullptr) {
                *out_device_id_high = GetDeviceIdHigh(device_id_val);
            }
        }

        return true;
    }

    void EncryptDeviceUniqueData(void *dst, size_t dst_size, const void *seal_key_source, size_t seal_key_source_size, const void *access_key, size_t access_key_size, const void *key_source, size_t key_source_size, const void *src, size_t src_size, u8 device_id_high) {
        /* Determine metadata locations. */
        u8 * const dst_iv   = static_cast<u8 *>(dst);
        u8 * const dst_data = dst_iv + DeviceUniqueDataIvSize;
        u8 * const dst_pad  = dst_data + src_size;
        u8 * const dst_did  = dst_pad + DeviceUniqueDataPaddingSize;
        u8 * const dst_mac  = dst_did + DeviceUniqueDataDeviceIdSize;

        /* Verify that our sizes are okay. */
        const size_t enc_size = src_size + DeviceUniqueDataInnerMetaSize;
        const size_t res_size = src_size + DeviceUniqueDataTotalMetaSize;
        AMS_ABORT_UNLESS(res_size <= dst_size);

        /* Layout the image as expected. */
        {
            /* Generate a random iv. */
            util::AlignedBuffer<hw::DataCacheLineSize, DeviceUniqueDataIvSize> iv;
            GenerateIv(iv, DeviceUniqueDataIvSize);

            /* Move the data to the output image. */
            std::memmove(dst_data, src, src_size);

            /* Copy the iv. */
            std::memcpy(dst_iv, iv, DeviceUniqueDataIvSize);

            /* Clear the padding. */
            std::memset(dst_pad, 0, DeviceUniqueDataPaddingSize);

            /* Store the device id. */
            util::StoreBigEndian(reinterpret_cast<u64 *>(dst_did), EncodeDeviceId(device_id_high, fuse::GetDeviceId()));
        }

        /* Encrypt and mac. */
        {

            /* Prepare the key used to encrypt the data. */
            PrepareDeviceUniqueDataKey(seal_key_source, seal_key_source_size, access_key, access_key_size, key_source, key_source_size);

            /* Compute the gmac. */
            ComputeGmac(dst_mac, DeviceUniqueDataMacSize, dst_data, enc_size, dst_iv, DeviceUniqueDataIvSize);

            /* Encrypt the data. */
            ComputeAes128Ctr(dst_data, enc_size, pkg1::AesKeySlot_Smc, dst_data, enc_size, dst_iv, DeviceUniqueDataIvSize);
        }
    }

}
