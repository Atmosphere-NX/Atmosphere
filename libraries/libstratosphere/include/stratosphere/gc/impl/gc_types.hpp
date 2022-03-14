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

namespace ams::gc::impl {

    struct CardInitialDataPayload  {
        u8 package_id[8];
        u8 reserved_8[8];
        u8 auth_data[0x10];
        u8 auth_mac[0x10];
        u8 auth_nonce[0xC];
    };
    static_assert(util::is_pod<CardInitialDataPayload>::value);
    static_assert(sizeof(CardInitialDataPayload) == 0x3C);

    struct CardInitialData {
        CardInitialDataPayload payload;
        u8 padding[0x200 - sizeof(CardInitialDataPayload)];
    };
    static_assert(util::is_pod<CardInitialData>::value);
    static_assert(sizeof(CardInitialData) == 0x200);

    enum FwVersion : u8 {
        FwVersion_ForDev = 0,
        FwVersion_1_0_0  = 1,
        FwVersion_4_0_0  = 2,
        FwVersion_9_0_0  = 3,
        FwVersion_11_0_0 = 4,
        FwVersion_12_0_0 = 5,
    };

    enum KekIndex : u8 {
        KekIndex_Version0      = 0,
        KekIndex_VersionForDev = 1,
    };

    struct CardHeaderKeyIndex {
        using KekIndex         = util::BitPack8::Field<0, 4, gc::impl::KekIndex>;
        using TitleKeyDecIndex = util::BitPack8::Field<KekIndex::Next, 4, u8>;

        static_assert(TitleKeyDecIndex::Next == BITSIZEOF(u8));
    };

    struct CardHeaderEncryptedData {
        u32 fw_version[2];
        u32 acc_ctrl_1;
        u32 wait_1_time_read;
        u32 wait_2_time_read;
        u32 wait_1_time_write;
        u32 wait_2_time_write;
        u32 fw_mode;
        u32 cup_version;
        u8 compatibility_type;
        u8 reserved_25;
        u8 reserved_26;
        u8 reserved_27;
        u8 upp_hash[8];
        u64 cup_id;
        u8 reserved_38[0x38];
    };
    static_assert(util::is_pod<CardHeaderEncryptedData>::value);
    static_assert(sizeof(CardHeaderEncryptedData) == 0x70);

    enum MemoryCapacity : u8 {
        MemoryCapacity_1GB  = 0xFA,
        MemoryCapacity_2GB  = 0xF8,
        MemoryCapacity_4GB  = 0xF0,
        MemoryCapacity_8GB  = 0xE0,
        MemoryCapacity_16GB = 0xE1,
        MemoryCapacity_32GB = 0xE2,
    };

    enum AccessControl1ClockRate : u32 {
        AccessControl1ClockRate_25MHz = 0x00A10011,
        AccessControl1ClockRate_50MHz = 0x00A10010,
    };

    enum SelSec : u8 {
        SelSec_T1 = 1,
        SelSec_T2 = 2,
    };

    struct CardHeader {
        static constexpr u32 Magic = util::FourCC<'H','E','A','D'>::Code;

        u32 magic;
        u32 rom_area_start_page;
        u32 backup_area_start_page;
        util::BitPack8 key_index;
        u8 rom_size;
        u8 version;
        u8 flags;
        u8 package_id[8];
        u32 valid_data_end_page;
        u8 reserved_11C[4];
        u8 iv[crypto::Aes128CbcDecryptor::IvSize];
        u64 partition_fs_header_address;
        u64 partition_fs_header_size;
        u8 partition_fs_header_hash[crypto::Sha256Generator::HashSize];
        u8 initial_data_hash[crypto::Sha256Generator::HashSize];
        u32 sel_sec;
        u32 sel_t1_key;
        u32 sel_key;
        u32 lim_area_page;

        union {
            u8 raw_encrypted_data[sizeof(CardHeaderEncryptedData)];
            CardHeaderEncryptedData encrypted_data;
        };
    };
    static_assert(util::is_pod<CardHeader>::value);
    static_assert(sizeof(CardHeader) == 0x100);

    struct CardHeaderWithSignature {
        u8 signature[crypto::Rsa2048Pkcs1Sha256Verifier::SignatureSize];
        CardHeader data;
    };
    static_assert(util::is_pod<CardHeaderWithSignature>::value);
    static_assert(sizeof(CardHeaderWithSignature) == 0x200);

    static constexpr size_t CardDeviceIdLength = 0x10;

    struct T1CardCertificate {
        static constexpr u32 Magic = util::FourCC<'C','E','R','T'>::Code;

        u8 signature[crypto::Rsa2048Pkcs1Sha256Verifier::SignatureSize];
        u32 magic;
        u32 version;
        u8 kek_index;
        u8 flags[7];
        u8 t1_card_device_id[CardDeviceIdLength];
        u8 iv[crypto::Aes128CtrEncryptor::IvSize];
        u8 hw_key[crypto::Aes128CtrEncryptor::KeySize];
        u8 reserved[0xC0];
        u8 padding[0x200];
    };
    static_assert(util::is_pod<T1CardCertificate>::value);
    static_assert(sizeof(T1CardCertificate) == 0x400);

    struct Ca10Certificate {
        u8 signature[crypto::Rsa2048Pkcs1Sha256Verifier::SignatureSize];
        u8 unk_100[0x30];
        u8 modulus[crypto::Rsa2048Pkcs1Sha256Verifier::ModulusSize];
        u8 unk_230[0x1D0];
    };
    static_assert(util::is_pod<Ca10Certificate>::value);
    static_assert(sizeof(Ca10Certificate) == 0x400);

}
