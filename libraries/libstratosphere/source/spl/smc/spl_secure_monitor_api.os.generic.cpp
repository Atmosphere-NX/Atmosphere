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
#include <stratosphere.hpp>
#include <exosphere/pkg1.hpp>

namespace ams::spl::smc {

    #define SMC_R_SUCCEEEDED(res) (res == smc::Result::Success)
    #define SMC_R_FAILED(res)     (res != smc::Result::Success)

    #define SMC_R_TRY(res_expr)     ({ const auto _tmp_r_try_rc = (res_expr); if (SMC_R_FAILED(_tmp_r_try_rc)) { return _tmp_r_try_rc; } })
    #define SMC_R_UNLESS(cond, RES) ({ if (!(cond)) { return smc::Result::RES; }})

    namespace {

        enum SealKey {
            SealKey_LoadAesKey                  = 0,
            SealKey_DecryptDeviceUniqueData     = 1,
            SealKey_ImportLotusKey              = 2,
            SealKey_ImportEsDeviceKey           = 3,
            SealKey_ReencryptDeviceUniqueData   = 4,
            SealKey_ImportSslKey                = 5,
            SealKey_ImportEsClientCertKey       = 6,

            SealKey_Count,
        };

        enum KeyType {
            KeyType_Default           = 0,
            KeyType_NormalOnly        = 1,
            KeyType_RecoveryOnly      = 2,
            KeyType_NormalAndRecovery = 3,

            KeyType_Count,
        };

        enum EsCommonKeyType {
            EsCommonKeyType_TitleKey   = 0,
            EsCommonKeyType_ArchiveKey = 1,
            EsCommonKeyType_Unknown2   = 2,

            EsCommonKeyType_Count,
        };

        struct GenerateAesKekOption {
            using IsDeviceUnique = util::BitPack32::Field<0,  1, bool>;
            using KeyTypeIndex   = util::BitPack32::Field<1,  4, KeyType>;
            using SealKeyIndex   = util::BitPack32::Field<5,  3, SealKey>;
            using Reserved       = util::BitPack32::Field<8, 24, u32>;
        };

        struct ComputeAesOption {
            using KeySlot         = util::BitPack32::Field<0, 3, int>;
            using CipherModeIndex = util::BitPack32::Field<4, 2, CipherMode>;
        };

        constexpr const u8 KeyTypeSources[KeyType_Count][crypto::AesEncryptor128::KeySize] = {
            [KeyType_Default]           = { 0x4D, 0x87, 0x09, 0x86, 0xC4, 0x5D, 0x20, 0x72, 0x2F, 0xBA, 0x10, 0x53, 0xDA, 0x92, 0xE8, 0xA9 },
            [KeyType_NormalOnly]        = { 0x25, 0x03, 0x31, 0xFB, 0x25, 0x26, 0x0B, 0x79, 0x8C, 0x80, 0xD2, 0x69, 0x98, 0xE2, 0x22, 0x77 },
            [KeyType_RecoveryOnly]      = { 0x76, 0x14, 0x1D, 0x34, 0x93, 0x2D, 0xE1, 0x84, 0x24, 0x7B, 0x66, 0x65, 0x55, 0x04, 0x65, 0x81 },
            [KeyType_NormalAndRecovery] = { 0xAF, 0x3D, 0xB7, 0xF3, 0x08, 0xA2, 0xD8, 0xA2, 0x08, 0xCA, 0x18, 0xA8, 0x69, 0x46, 0xC9, 0x0B },
        };

        constexpr const u8 SealKeyMasks[SealKey_Count][crypto::AesEncryptor128::KeySize] = {
            [SealKey_LoadAesKey]                  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
            [SealKey_DecryptDeviceUniqueData]     = { 0xA2, 0xAB, 0xBF, 0x9C, 0x92, 0x2F, 0xBB, 0xE3, 0x78, 0x79, 0x9B, 0xC0, 0xCC, 0xEA, 0xA5, 0x74 },
            [SealKey_ImportLotusKey]              = { 0x57, 0xE2, 0xD9, 0x45, 0xE4, 0x92, 0xF4, 0xFD, 0xC3, 0xF9, 0x86, 0x38, 0x89, 0x78, 0x9F, 0x3C },
            [SealKey_ImportEsDeviceKey]           = { 0xE5, 0x4D, 0x9A, 0x02, 0xF0, 0x4F, 0x5F, 0xA8, 0xAD, 0x76, 0x0A, 0xF6, 0x32, 0x95, 0x59, 0xBB },
            [SealKey_ReencryptDeviceUniqueData]   = { 0x59, 0xD9, 0x31, 0xF4, 0xA7, 0x97, 0xB8, 0x14, 0x40, 0xD6, 0xA2, 0x60, 0x2B, 0xED, 0x15, 0x31 },
            [SealKey_ImportSslKey]                = { 0xFD, 0x6A, 0x25, 0xE5, 0xD8, 0x38, 0x7F, 0x91, 0x49, 0xDA, 0xF8, 0x59, 0xA8, 0x28, 0xE6, 0x75 },
            [SealKey_ImportEsClientCertKey]       = { 0x89, 0x96, 0x43, 0x9A, 0x7C, 0xD5, 0x59, 0x55, 0x24, 0xD5, 0x24, 0x18, 0xAB, 0x6C, 0x04, 0x61 },
        };

        constexpr const u8 EsCommonKeySources[EsCommonKeyType_Count][AesKeySize] = {
            [EsCommonKeyType_TitleKey]   = { 0x1E, 0xDC, 0x7B, 0x3B, 0x60, 0xE6, 0xB4, 0xD8, 0x78, 0xB8, 0x17, 0x15, 0x98, 0x5E, 0x62, 0x9B },
            [EsCommonKeyType_ArchiveKey] = { 0x3B, 0x78, 0xF2, 0x61, 0x0F, 0x9D, 0x5A, 0xE2, 0x7B, 0x4E, 0x45, 0xAF, 0xCB, 0x0B, 0x67, 0x4D },
            [EsCommonKeyType_Unknown2]   = { 0x42, 0x64, 0x0B, 0xE3, 0x5F, 0xC6, 0xBE, 0x47, 0xC7, 0xB4, 0x84, 0xC5, 0xEB, 0x63, 0xAA, 0x02 },
        };

        constexpr u64 InvalidAsyncKey = 0;

        constinit os::SdkMutex g_crypto_lock;
        constinit u64 g_async_key = InvalidAsyncKey;
        constinit u8 g_async_result_buffer[1_KB];

        u64 GenerateRandomU64() {
            u64 v = -1;
            crypto::GenerateCryptographicallyRandomBytes(std::addressof(v), sizeof(v));
            return v;
        }

        constinit u8 g_master_keys[pkg1::KeyGeneration_Max][crypto::AesEncryptor128::KeySize]{};
        constinit u8 g_device_keys[pkg1::KeyGeneration_Max][crypto::AesEncryptor128::KeySize]{};

        class KeySlotManager {
            private:
                u8 m_key_slot_contents[pkg1::AesKeySlot_Count][crypto::AesEncryptor256::KeySize];
            public:
                constexpr KeySlotManager() : m_key_slot_contents{} { /* ... */ }
            public:
                const u8 *GetKey(s32 slot) const {
                    return m_key_slot_contents[slot];
                }

                void LoadAesKey(s32 slot, const AccessKey &access_key, const KeySource &key_source) {
                    crypto::AesDecryptor128 aes;
                    aes.Initialize(std::addressof(access_key), sizeof(access_key));
                    aes.DecryptBlock(m_key_slot_contents[slot], crypto::AesEncryptor128::KeySize, std::addressof(key_source), sizeof(key_source));
                }

                void LoadPreparedAesKey(s32 slot, const AccessKey &access_key) {
                    this->SetAesKey128(slot, std::addressof(access_key), sizeof(access_key));
                }

                s32 PrepareDeviceMasterKey(s32 generation) {
                    constexpr s32 Slot = pkg1::AesKeySlot_Smc;
                    this->SetAesKey128(Slot, g_device_keys[generation], crypto::AesEncryptor128::KeySize);
                    return Slot;
                }

                s32 PrepareMasterKey(s32 generation) {
                    constexpr s32 Slot = pkg1::AesKeySlot_Smc;
                    this->SetAesKey128(Slot, g_master_keys[generation], crypto::AesEncryptor128::KeySize);
                    return Slot;
                }

                void SetAesKey128(s32 slot, const void *key, size_t key_size) {
                    std::memcpy(m_key_slot_contents[slot], key, key_size);
                }

                void SetEncryptedAesKey128(s32 dst, s32 src, const void *key_source, size_t key_source_size) {
                    crypto::AesDecryptor128 aes;
                    aes.Initialize(this->GetKey(src), crypto::AesDecryptor128::KeySize);
                    aes.DecryptBlock(m_key_slot_contents[dst], crypto::AesEncryptor128::KeySize, key_source, key_source_size);
                }

                void DecryptAes128(void *dst, size_t dst_size, s32 slot, const void *src, size_t src_size) {
                    crypto::AesDecryptor128 aes;
                    aes.Initialize(this->GetKey(slot), crypto::AesDecryptor128::KeySize);
                    aes.DecryptBlock(dst, dst_size, src, src_size);
                }
        };

        constexpr bool IsUserAesKeySlot(s32 slot) {
            return pkg1::IsUserAesKeySlot(slot);
        }

        constinit KeySlotManager g_key_slot_manager;

        void DecryptWithEsCommonKey(void *dst, size_t dst_size, const void *src, size_t src_size, EsCommonKeyType type, int generation) {
            /* Validate pre-conditions. */
            AMS_ASSERT(dst_size == crypto::AesEncryptor128::KeySize);
            AMS_ASSERT(src_size == crypto::AesEncryptor128::KeySize);
            AMS_ASSERT(0 <= type && type < EsCommonKeyType_Count);

            /* Prepare the master key for the generation. */
            const int slot = g_key_slot_manager.PrepareMasterKey(generation);

            /* Derive the es common key. */
            g_key_slot_manager.SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, slot, EsCommonKeySources[type], crypto::AesEncryptor128::KeySize);

            /* Decrypt the input using the common key. */
            g_key_slot_manager.DecryptAes128(dst, dst_size, pkg1::AesKeySlot_Smc, src, src_size);
        }

    }

    void PresetInternalKey(const AesKey *key, u32 generation, bool device) {
        if (device) {
            std::memcpy(g_device_keys[generation], key, sizeof(*key));
        } else {
            std::memcpy(g_master_keys[generation], key, sizeof(*key));
        }
    }

    //Result SetConfig(AsyncOperationKey *out_op, spl::ConfigItem key, const u64 *value, size_t num_qwords, const void *sign) {
    //    svc::SecureMonitorArguments args;
    //
    //    args.r[0] = static_cast<u64>(FunctionId::SetConfig);
    //    args.r[1] = static_cast<u64>(key);
    //    args.r[2] = reinterpret_cast<u64>(sign);
    //
    //    for (size_t i = 0; i < std::min(static_cast<size_t>(4), num_qwords); i++) {
    //        args.r[3 + i] = value[i];
    //    }
    //    svc::CallSecureMonitor(std::addressof(args));
    //
    //    out_op->value = args.r[1];
    //    return static_cast<Result>(args.r[0]);
    //}

    //Result GetConfig(u64 *out, size_t num_qwords, spl::ConfigItem key) {
    //    svc::SecureMonitorArguments args;
    //
    //    args.r[0] = static_cast<u64>(FunctionId::GetConfig);
    //    args.r[1] = static_cast<u64>(key);
    //    svc::CallSecureMonitor(std::addressof(args));
    //
    //    for (size_t i = 0; i < std::min(static_cast<size_t>(4), num_qwords); i++) {
    //        out[i] = args.r[1 + i];
    //    }
    //    return static_cast<Result>(args.r[0]);
    //}

    Result GetResult(Result *out, AsyncOperationKey op) {
        SMC_R_UNLESS(g_async_key != InvalidAsyncKey,      NoAsyncOperation);
        SMC_R_UNLESS(g_async_key == op.value,        InvalidAsyncOperation);

        g_async_key = InvalidAsyncKey;

        *out = smc::Result::Success;
        return smc::Result::Success;
    }

    Result GetResultData(Result *out, void *out_buf, size_t out_buf_size, AsyncOperationKey op) {
        SMC_R_UNLESS(g_async_key != InvalidAsyncKey,                     NoAsyncOperation);
        SMC_R_UNLESS(g_async_key == op.value,                       InvalidAsyncOperation);
        SMC_R_UNLESS(out_buf_size <= sizeof(g_async_result_buffer),       InvalidArgument);

        g_async_key = InvalidAsyncKey;
        std::memcpy(out_buf, g_async_result_buffer, out_buf_size);

        *out = smc::Result::Success;
        return smc::Result::Success;
    }

    //Result ModularExponentiate(AsyncOperationKey *out_op, const void *base, const void *exp, size_t exp_size, const void *mod) {
    //    svc::SecureMonitorArguments args;
    //
    //    args.r[0] = static_cast<u64>(FunctionId::ModularExponentiate);
    //    args.r[1] = reinterpret_cast<u64>(base);
    //    args.r[2] = reinterpret_cast<u64>(exp);
    //    args.r[3] = reinterpret_cast<u64>(mod);
    //    args.r[4] = exp_size;
    //    svc::CallSecureMonitor(std::addressof(args));
    //
    //    out_op->value = args.r[1];
    //    return static_cast<Result>(args.r[0]);
    //}

    Result GenerateRandomBytes(void *out, size_t size) {
        crypto::GenerateCryptographicallyRandomBytes(out, size);
        return smc::Result::Success;
    }

    Result GenerateAesKek(AccessKey *out, const KeySource &source, u32 user_generation, u32 option_value) {
        std::scoped_lock lk(g_crypto_lock);

        const int pkg1_generation = std::max<int>(static_cast<int>(user_generation) - 1, pkg1::KeyGeneration_1_0_0);

        const util::BitPack32 option = { option_value };
        const bool is_device_unique  = option.Get<GenerateAesKekOption::IsDeviceUnique>();
        const auto key_type          = option.Get<GenerateAesKekOption::KeyTypeIndex>();
        const auto seal_key          = option.Get<GenerateAesKekOption::SealKeyIndex>();
        const u32  reserved          = option.Get<GenerateAesKekOption::Reserved>();

        /* Validate arguments. */
        SMC_R_UNLESS(reserved == 0, InvalidArgument);

        if (is_device_unique) {
            SMC_R_UNLESS(pkg1::IsValidDeviceUniqueKeyGeneration(pkg1_generation), InvalidArgument);
        } else {
            SMC_R_UNLESS(pkg1_generation < pkg1::KeyGeneration_Max, InvalidArgument);
        }

        SMC_R_UNLESS(0 <= key_type && key_type < KeyType_Count, InvalidArgument);
        SMC_R_UNLESS(0 <= seal_key && seal_key < SealKey_Count, InvalidArgument);

        /* Here N might check if key type is normal or recovery only, but we're not going to enforce that. */
        u8 static_source[crypto::AesEncryptor128::KeySize];

        /* Derive the static source. */
        for (size_t i = 0; i < sizeof(static_source); ++i) {
            static_source[i] = KeyTypeSources[key_type][i] ^ SealKeyMasks[seal_key][i];
        }

        /* Get the slot. */
        const int slot = is_device_unique ? g_key_slot_manager.PrepareDeviceMasterKey(pkg1_generation) : g_key_slot_manager.PrepareMasterKey(pkg1_generation);

        /* Derive a static generation kek. */
        g_key_slot_manager.SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, slot, static_source, sizeof(static_source));

        /* Decrypt the input using the static-derived key. */
        g_key_slot_manager.DecryptAes128(out, sizeof(*out), pkg1::AesKeySlot_Smc, std::addressof(source), sizeof(source));

        return smc::Result::Success;
    }

    Result LoadAesKey(u32 keyslot, const AccessKey &access_key, const KeySource &source) {
        std::scoped_lock lk(g_crypto_lock);

        /* Check args. */
        SMC_R_UNLESS(IsUserAesKeySlot(keyslot), InvalidArgument);

        /* Unseal the access key. */
        g_key_slot_manager.SetAesKey128(pkg1::AesKeySlot_Smc, std::addressof(access_key), sizeof(access_key));

        /* Derive the key. */
        g_key_slot_manager.SetEncryptedAesKey128(keyslot, pkg1::AesKeySlot_Smc, std::addressof(source), sizeof(source));

        return smc::Result::Success;
    }

    Result ComputeAes(AsyncOperationKey *out_op, u64 dst_addr, u32 mode, const IvCtr &iv_ctr, u64 src_addr, size_t size) {
        std::scoped_lock lk(g_crypto_lock);

        /* Check size. */
        SMC_R_UNLESS(util::IsAligned(size, crypto::AesEncryptor128::BlockSize), InvalidArgument);

        const util::BitPack32 option = { mode };
        const int  slot        = option.Get<ComputeAesOption::KeySlot>();
        const auto cipher_mode = option.Get<ComputeAesOption::CipherModeIndex>();
        SMC_R_UNLESS(IsUserAesKeySlot(slot), InvalidArgument);

        /* Set a random async key. */
        g_async_key = GenerateRandomU64();

        switch (cipher_mode) {
            case CipherMode::CbcEncrypt: crypto::EncryptAes128Cbc(reinterpret_cast<void *>(dst_addr), size, g_key_slot_manager.GetKey(slot), crypto::AesEncryptor128::KeySize, iv_ctr.data, sizeof(iv_ctr.data), reinterpret_cast<const void *>(src_addr), size); break;
            case CipherMode::CbcDecrypt: crypto::DecryptAes128Cbc(reinterpret_cast<void *>(dst_addr), size, g_key_slot_manager.GetKey(slot), crypto::AesEncryptor128::KeySize, iv_ctr.data, sizeof(iv_ctr.data), reinterpret_cast<const void *>(src_addr), size); break;
            case CipherMode::Ctr:        crypto::EncryptAes128Ctr(reinterpret_cast<void *>(dst_addr), size, g_key_slot_manager.GetKey(slot), crypto::AesEncryptor128::KeySize, iv_ctr.data, sizeof(iv_ctr.data), reinterpret_cast<const void *>(src_addr), size); break;
            default:
                return smc::Result::InvalidArgument;
        }

        *out_op = AsyncOperationKey{g_async_key};
        return smc::Result::Success;
    }

    //Result GenerateSpecificAesKey(AesKey *out_key, const KeySource &source, u32 generation, u32 which) {
    //    svc::SecureMonitorArguments args;
    //
    //    args.r[0] = static_cast<u64>(FunctionId::GenerateSpecificAesKey);
    //    args.r[1] = source.data64[0];
    //    args.r[2] = source.data64[1];
    //    args.r[3] = generation;
    //    args.r[4] = which;
    //    svc::CallSecureMonitor(std::addressof(args));
    //
    //    out_key->data64[0] = args.r[1];
    //    out_key->data64[1] = args.r[2];
    //    return static_cast<Result>(args.r[0]);
    //}

    //Result ComputeCmac(Cmac *out_mac, u32 keyslot, const void *data, size_t size) {
    //    svc::SecureMonitorArguments args;
    //
    //    args.r[0] = static_cast<u64>(FunctionId::ComputeCmac);
    //    args.r[1] = keyslot;
    //    args.r[2] = reinterpret_cast<u64>(data);
    //    args.r[3] = size;
    //    svc::CallSecureMonitor(std::addressof(args));
    //
    //    out_mac->data64[0] = args.r[1];
    //    out_mac->data64[1] = args.r[2];
    //    return static_cast<Result>(args.r[0]);
    //}

    //Result ReencryptDeviceUniqueData(void *data, size_t size, const AccessKey &access_key_dec, const KeySource &source_dec, const AccessKey &access_key_enc, const KeySource &source_enc, u32 option) {
    //    svc::SecureMonitorArguments args;
    //
    //    args.r[0] = static_cast<u64>(FunctionId::ReencryptDeviceUniqueData);
    //    args.r[1] = reinterpret_cast<u64>(std::addressof(access_key_dec));
    //    args.r[2] = reinterpret_cast<u64>(std::addressof(access_key_enc));
    //    args.r[3] = option;
    //    args.r[4] = reinterpret_cast<u64>(data);
    //    args.r[5] = size;
    //    args.r[6] = reinterpret_cast<u64>(std::addressof(source_dec));
    //    args.r[7] = reinterpret_cast<u64>(std::addressof(source_enc));
    //    svc::CallSecureMonitor(std::addressof(args));
    //
    //    return static_cast<Result>(args.r[0]);
    //}

    //Result DecryptDeviceUniqueData(void *data, size_t size, const AccessKey &access_key, const KeySource &source, DeviceUniqueDataMode mode) {
    //    svc::SecureMonitorArguments args;
    //
    //    args.r[0] = static_cast<u64>(FunctionId::DecryptDeviceUniqueData);
    //    args.r[1] = access_key.data64[0];
    //    args.r[2] = access_key.data64[1];
    //    args.r[3] = static_cast<u32>(mode);
    //    args.r[4] = reinterpret_cast<u64>(data);
    //    args.r[5] = size;
    //    args.r[6] = source.data64[0];
    //    args.r[7] = source.data64[1];
    //    svc::CallSecureMonitor(std::addressof(args));
    //
    //    return static_cast<Result>(args.r[0]);
    //}

    //Result ModularExponentiateWithStorageKey(AsyncOperationKey *out_op, const void *base, const void *mod, ModularExponentiateWithStorageKeyMode mode) {
    //    svc::SecureMonitorArguments args;
    //
    //    args.r[0] = static_cast<u64>(FunctionId::ModularExponentiateWithStorageKey);
    //    args.r[1] = reinterpret_cast<u64>(base);
    //    args.r[2] = reinterpret_cast<u64>(mod);
    //    args.r[3] = static_cast<u32>(mode);
    //    svc::CallSecureMonitor(std::addressof(args));
    //
    //    out_op->value = args.r[1];
    //    return static_cast<Result>(args.r[0]);
    //}

    //Result PrepareEsDeviceUniqueKey(AsyncOperationKey *out_op, const void *base, const void *mod, const void *label_digest, size_t label_digest_size, u32 option) {
    //    svc::SecureMonitorArguments args;
    //
    //    args.r[0] = static_cast<u64>(FunctionId::PrepareEsDeviceUniqueKey);
    //    args.r[1] = reinterpret_cast<u64>(base);
    //    args.r[2] = reinterpret_cast<u64>(mod);
    //    std::memset(std::addressof(args.r[3]), 0, 4 * sizeof(args.r[3]));
    //    std::memcpy(std::addressof(args.r[3]), label_digest, std::min(static_cast<size_t>(4 * sizeof(args.r[3])), label_digest_size));
    //    args.r[7] = option;
    //    svc::CallSecureMonitor(std::addressof(args));
    //
    //    out_op->value = args.r[1];
    //    return static_cast<Result>(args.r[0]);
    //}

    Result LoadPreparedAesKey(u32 keyslot, const AccessKey &access_key) {
        std::scoped_lock lk(g_crypto_lock);

        /* Check args. */
        SMC_R_UNLESS(IsUserAesKeySlot(keyslot), InvalidArgument);

        /* Unseal the key. */
        g_key_slot_manager.SetAesKey128(keyslot, std::addressof(access_key), sizeof(access_key));

        return smc::Result::Success;
    }

    Result PrepareCommonEsTitleKey(AccessKey *out, const KeySource &source, u32 generation) {
        /* Decode arguments. */
        const int pkg1_gen = std::max<int>(pkg1::KeyGeneration_1_0_0, static_cast<int>(generation) - 1);

        /* Validate arguments. */
        SMC_R_UNLESS(pkg1_gen < pkg1::KeyGeneration_Max, InvalidArgument);

        /* Derive the key. */
        u8 key[crypto::AesEncryptor128::KeySize];
        DecryptWithEsCommonKey(key, sizeof(key), std::addressof(source), sizeof(source), EsCommonKeyType_TitleKey, pkg1_gen);

        /* Copy the access key to the output. */
        std::memcpy(out, key, sizeof(key));

        return smc::Result::Success;
    }

    //
    ///* Deprecated functions. */
    //Result LoadEsDeviceKey(const void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option) {
    //    svc::SecureMonitorArguments args;
    //
    //    args.r[0] = static_cast<u64>(FunctionId::LoadEsDeviceKey);
    //    args.r[1] = access_key.data64[0];
    //    args.r[2] = access_key.data64[1];
    //    args.r[3] = option;
    //    args.r[4] = reinterpret_cast<u64>(data);
    //    args.r[5] = size;
    //    args.r[6] = source.data64[0];
    //    args.r[7] = source.data64[1];
    //    svc::CallSecureMonitor(std::addressof(args));
    //
    //    return static_cast<Result>(args.r[0]);
    //}

    //Result DecryptDeviceUniqueData(size_t *out_size, void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option) {
    //    svc::SecureMonitorArguments args;
    //
    //    args.r[0] = static_cast<u64>(FunctionId::DecryptDeviceUniqueData);
    //    args.r[1] = access_key.data64[0];
    //    args.r[2] = access_key.data64[1];
    //    args.r[3] = option;
    //    args.r[4] = reinterpret_cast<u64>(data);
    //    args.r[5] = size;
    //    args.r[6] = source.data64[0];
    //    args.r[7] = source.data64[1];
    //    svc::CallSecureMonitor(std::addressof(args));
    //
    //    *out_size = static_cast<size_t>(args.r[1]);
    //    return static_cast<Result>(args.r[0]);
    //}

    //Result DecryptAndStoreGcKey(const void *data, size_t size, const AccessKey &access_key, const KeySource &source, u32 option) {
    //    svc::SecureMonitorArguments args;
    //
    //    args.r[0] = static_cast<u64>(FunctionId::DecryptAndStoreGcKey);
    //    args.r[1] = access_key.data64[0];
    //    args.r[2] = access_key.data64[1];
    //    args.r[3] = option;
    //    args.r[4] = reinterpret_cast<u64>(data);
    //    args.r[5] = size;
    //    args.r[6] = source.data64[0];
    //    args.r[7] = source.data64[1];
    //    svc::CallSecureMonitor(std::addressof(args));
    //
    //    return static_cast<Result>(args.r[0]);
    //}

    Result AtmosphereCopyToIram(uintptr_t, const void *, size_t ) {
        AMS_ABORT("AtmosphereCopyToIram not supported on generic SecureMonitor api.");
    }

    Result AtmosphereCopyFromIram(void *, uintptr_t, size_t) {
        AMS_ABORT("AtmosphereCopyToIram not supported on generic SecureMonitor api.");
    }

    Result AtmosphereReadWriteRegister(uint64_t, uint32_t, uint32_t, uint32_t *) {
        AMS_ABORT("AtmosphereReadWriteRegister not supported on generic SecureMonitor api.");
    }

    Result AtmosphereGetEmummcConfig(void *out_config, void *out_paths, u32 storage_id) {
        /* TODO: We actually probably should support this one on generic? */
        AMS_UNUSED(out_config, out_paths, storage_id);
        AMS_ABORT("AtmosphereGetEmummcConfig not supported on generic SecureMonitor api.");
    }

}
