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
#include <stratosphere.hpp>

namespace ams::fssystem {

    namespace {

        constexpr inline u32 SdkAddonVersionMin = 0x000B0000;

        constexpr Result CheckNcaMagic(u32 magic) {
            /* Verify the magic is not a deprecated one. */
            R_UNLESS(magic != NcaHeader::Magic0, fs::ResultUnsupportedSdkVersion());
            R_UNLESS(magic != NcaHeader::Magic1, fs::ResultUnsupportedSdkVersion());
            R_UNLESS(magic != NcaHeader::Magic2, fs::ResultUnsupportedSdkVersion());

            /* Verify the magic is the current one. */
            R_UNLESS(magic == NcaHeader::Magic3, fs::ResultInvalidNcaSignature());

            return ResultSuccess();
        }

    }

    NcaReader::NcaReader() : shared_base_storage(), header_storage(), body_storage(), decrypt_aes_ctr(), decrypt_aes_ctr_external(), is_software_aes_prioritized(false), header_encryption_type(NcaHeader::EncryptionType::Auto) {
        std::memset(std::addressof(this->header), 0, sizeof(this->header));
        std::memset(std::addressof(this->decryption_keys), 0, sizeof(this->decryption_keys));
        std::memset(std::addressof(this->external_decryption_key), 0, sizeof(this->external_decryption_key));
    }

    NcaReader::~NcaReader() {
        /* ... */
    }

    Result NcaReader::Initialize(std::shared_ptr<fs::IStorage> base_storage, const NcaCryptoConfiguration &crypto_cfg) {
        this->shared_base_storage = base_storage;
        return this->Initialize(this->shared_base_storage.get(), crypto_cfg);
    }

    Result NcaReader::Initialize(fs::IStorage *base_storage, const NcaCryptoConfiguration &crypto_cfg) {
        /* Validate preconditions. */
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(this->body_storage == nullptr);
        R_UNLESS(crypto_cfg.generate_key != nullptr, fs::ResultInvalidArgument());

        /* Generate keys for header. */
        u8 header_decryption_keys[NcaCryptoConfiguration::HeaderEncryptionKeyCount][NcaCryptoConfiguration::Aes128KeySize];
        for (size_t i = 0; i < NcaCryptoConfiguration::HeaderEncryptionKeyCount; i++) {
            crypto_cfg.generate_key(header_decryption_keys[i], AesXtsStorage::KeySize, crypto_cfg.header_encrypted_encryption_keys[i], AesXtsStorage::KeySize, static_cast<s32>(KeyType::NcaHeaderKey), crypto_cfg);
        }

        /* Create the header storage. */
        const u8 header_iv[AesXtsStorage::IvSize] = {};
        std::unique_ptr<fs::IStorage> work_header_storage = std::make_unique<AesXtsStorage>(base_storage, header_decryption_keys[0], header_decryption_keys[1], AesXtsStorage::KeySize, header_iv, AesXtsStorage::IvSize, NcaHeader::XtsBlockSize);
        R_UNLESS(work_header_storage != nullptr, fs::ResultAllocationFailureInNcaReaderA());

        /* Read the header. */
        R_TRY(work_header_storage->Read(0, std::addressof(this->header), sizeof(this->header)));

        /* Validate the magic. */
        if (Result magic_result = CheckNcaMagic(this->header.magic); R_FAILED(magic_result)) {
            /* If we're not allowed to use plaintext headers, stop here. */
            R_UNLESS(crypto_cfg.is_plaintext_header_available, magic_result);

            /* Try to use a plaintext header. */
            R_TRY(base_storage->Read(0, std::addressof(this->header), sizeof(this->header)));
            R_UNLESS(R_SUCCEEDED(CheckNcaMagic(this->header.magic)), magic_result);

            /* Configure to use the plaintext header. */
            s64 base_storage_size;
            R_TRY(base_storage->GetSize(std::addressof(base_storage_size)));
            work_header_storage.reset(new fs::SubStorage(base_storage, 0, base_storage_size));
            R_UNLESS(work_header_storage != nullptr, fs::ResultAllocationFailureInNcaReaderA());

            this->header_encryption_type = NcaHeader::EncryptionType::None;
        }

        /* Validate the fixed key signature. */
        R_UNLESS(this->header.header1_signature_key_generation <= NcaCryptoConfiguration::Header1SignatureKeyGenerationMax, fs::ResultInvalidNcaHeader1SignatureKeyGeneration());
        const u8 *header_1_sign_key_modulus = crypto_cfg.header_1_sign_key_moduli[this->header.header1_signature_key_generation];
        AMS_ABORT_UNLESS(header_1_sign_key_modulus != nullptr);
        {
            const u8 *sig         = this->header.header_sign_1;
            const size_t sig_size = NcaHeader::HeaderSignSize;
            const u8 *mod         = header_1_sign_key_modulus;
            const size_t mod_size = NcaCryptoConfiguration::Rsa2048KeyModulusSize;
            const u8 *exp         = crypto_cfg.header_1_sign_key_public_exponent;
            const size_t exp_size = NcaCryptoConfiguration::Rsa2048KeyPublicExponentSize;
            const u8 *msg         = static_cast<const u8 *>(static_cast<const void *>(std::addressof(this->header.magic)));
            const size_t msg_size = NcaHeader::Size - NcaHeader::HeaderSignSize * NcaHeader::HeaderSignCount;
            const bool is_signature_valid = crypto::VerifyRsa2048PssSha256(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size);
            R_UNLESS(is_signature_valid, fs::ResultNcaHeaderSignature1VerificationFailed());
        }

        /* Validate the sdk version. */
        R_UNLESS(this->header.sdk_addon_version >= SdkAddonVersionMin, fs::ResultUnsupportedSdkVersion());

        /* Validate the key index. */
        R_UNLESS(this->header.key_index < NcaCryptoConfiguration::KeyAreaEncryptionKeyIndexCount, fs::ResultInvalidNcaKeyIndex());

        /* Check if we have a rights id. */
        constexpr const u8 ZeroRightsId[NcaHeader::RightsIdSize] = {};
        if (crypto::IsSameBytes(ZeroRightsId, this->header.rights_id, NcaHeader::RightsIdSize)) {
            /* If we do, then we don't have an external key, so we need to generate decryption keys. */
            crypto_cfg.generate_key(this->decryption_keys[NcaHeader::DecryptionKey_AesCtr], crypto::AesDecryptor128::KeySize, this->header.encrypted_key_area + NcaHeader::DecryptionKey_AesCtr * crypto::AesDecryptor128::KeySize, crypto::AesDecryptor128::KeySize, GetKeyTypeValue(this->header.key_index, this->header.GetProperKeyGeneration()), crypto_cfg);

            /* Copy the hardware speed emulation key. */
            std::memcpy(this->decryption_keys[NcaHeader::DecryptionKey_AesCtrHw], this->header.encrypted_key_area + NcaHeader::DecryptionKey_AesCtrHw * crypto::AesDecryptor128::KeySize, crypto::AesDecryptor128::KeySize);
        }

        /* Clear the external decryption key. */
        std::memset(this->external_decryption_key, 0, sizeof(this->external_decryption_key));

        /* Set our decryptor functions. */
        this->decrypt_aes_ctr          = crypto_cfg.decrypt_aes_ctr;
        this->decrypt_aes_ctr_external = crypto_cfg.decrypt_aes_ctr_external;

        /* Set our storages. */
        this->header_storage = std::move(work_header_storage);
        this->body_storage   = base_storage;

        return ResultSuccess();
    }

    fs::IStorage *NcaReader::GetBodyStorage() {
        return this->body_storage;
    }

    u32 NcaReader::GetMagic() const {
        AMS_ASSERT(this->body_storage != nullptr);
        return this->header.magic;
    }

    NcaHeader::DistributionType NcaReader::GetDistributionType() const {
        AMS_ASSERT(this->body_storage != nullptr);
        return this->header.distribution_type;
    }

    NcaHeader::ContentType NcaReader::GetContentType() const {
        AMS_ASSERT(this->body_storage != nullptr);
        return this->header.content_type;
    }

    u8 NcaReader::GetKeyGeneration() const {
        AMS_ASSERT(this->body_storage != nullptr);
        return this->header.GetProperKeyGeneration();
    }

    u8 NcaReader::GetKeyIndex() const {
        AMS_ASSERT(this->body_storage != nullptr);
        return this->header.key_index;
    }

    u64 NcaReader::GetContentSize() const {
        AMS_ASSERT(this->body_storage != nullptr);
        return this->header.content_size;
    }

    u64 NcaReader::GetProgramId() const {
        AMS_ASSERT(this->body_storage != nullptr);
        return this->header.program_id;
    }

    u32 NcaReader::GetContentIndex() const {
        AMS_ASSERT(this->body_storage != nullptr);
        return this->header.content_index;
    }

    u32 NcaReader::GetSdkAddonVersion() const {
        AMS_ASSERT(this->body_storage != nullptr);
        return this->header.sdk_addon_version;
    }

    void NcaReader::GetRightsId(u8 *dst, size_t dst_size) const {
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(dst_size >= NcaHeader::RightsIdSize);
        std::memcpy(dst, this->header.rights_id, NcaHeader::RightsIdSize);
    }

    bool NcaReader::HasFsInfo(s32 index) const {
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        return this->header.fs_info[index].start_sector != 0 || this->header.fs_info[index].end_sector != 0;
    }

    s32 NcaReader::GetFsCount() const {
        AMS_ASSERT(this->body_storage != nullptr);
        for (s32 i = 0; i < NcaHeader::FsCountMax; i++) {
            if (!this->HasFsInfo(i)) {
                return i;
            }
        }
        return NcaHeader::FsCountMax;
    }

    const Hash &NcaReader::GetFsHeaderHash(s32 index) const {
        AMS_ASSERT(this->body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        return this->header.fs_header_hash[index];
    }

    void NcaReader::GetFsHeaderHash(Hash *dst, s32 index) const {
        AMS_ASSERT(this->body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        AMS_ASSERT(dst != nullptr);
        std::memcpy(dst, std::addressof(this->header.fs_header_hash[index]), sizeof(*dst));
    }

    void NcaReader::GetFsInfo(NcaHeader::FsInfo *dst, s32 index) const {
        AMS_ASSERT(this->body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        AMS_ASSERT(dst != nullptr);
        std::memcpy(dst, std::addressof(this->header.fs_info[index]), sizeof(*dst));
    }

    u64 NcaReader::GetFsOffset(s32 index) const {
        AMS_ASSERT(this->body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        return NcaHeader::SectorToByte(this->header.fs_info[index].start_sector);
    }

    u64 NcaReader::GetFsEndOffset(s32 index) const {
        AMS_ASSERT(this->body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        return NcaHeader::SectorToByte(this->header.fs_info[index].end_sector);
    }

    u64 NcaReader::GetFsSize(s32 index) const {
        AMS_ASSERT(this->body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        return NcaHeader::SectorToByte(this->header.fs_info[index].end_sector - this->header.fs_info[index].start_sector);
    }

    void NcaReader::GetEncryptedKey(void *dst, size_t size) const {
        AMS_ASSERT(this->body_storage != nullptr);
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(size >= NcaHeader::EncryptedKeyAreaSize);
        std::memcpy(dst, this->header.encrypted_key_area, NcaHeader::EncryptedKeyAreaSize);
    }

    const void *NcaReader::GetDecryptionKey(s32 index) const {
        AMS_ASSERT(this->body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::DecryptionKey_Count);
        return this->decryption_keys[index];
    }

    bool NcaReader::HasValidInternalKey() const {
        constexpr const u8 ZeroKey[crypto::AesDecryptor128::KeySize] = {};
        for (s32 i = 0; i < NcaHeader::DecryptionKey_Count; i++) {
            if (!crypto::IsSameBytes(ZeroKey, this->header.encrypted_key_area + i * crypto::AesDecryptor128::KeySize, crypto::AesDecryptor128::KeySize)) {
                return true;
            }
        }
        return false;
    }

    bool NcaReader::HasInternalDecryptionKeyForAesHardwareSpeedEmulation() const {
        constexpr const u8 ZeroKey[crypto::AesDecryptor128::KeySize] = {};
        return !crypto::IsSameBytes(ZeroKey, this->GetDecryptionKey(NcaHeader::DecryptionKey_AesCtrHw), crypto::AesDecryptor128::KeySize);
    }

    bool NcaReader::IsSoftwareAesPrioritized() const {
        return this->is_software_aes_prioritized;
    }

    void NcaReader::PrioritizeSoftwareAes() {
        this->is_software_aes_prioritized = true;
    }

    bool NcaReader::HasExternalDecryptionKey() const {
        constexpr const u8 ZeroKey[crypto::AesDecryptor128::KeySize] = {};
        return !crypto::IsSameBytes(ZeroKey, this->GetExternalDecryptionKey(), crypto::AesDecryptor128::KeySize);
    }

    const void *NcaReader::GetExternalDecryptionKey() const {
        return this->external_decryption_key;
    }

    void NcaReader::SetExternalDecryptionKey(const void *src, size_t size) {
        AMS_ASSERT(src != nullptr);
        AMS_ASSERT(size == sizeof(this->external_decryption_key));
        std::memcpy(this->external_decryption_key, src, sizeof(this->external_decryption_key));
    }

    void NcaReader::GetRawData(void *dst, size_t dst_size) const {
        AMS_ASSERT(this->body_storage != nullptr);
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(dst_size >= sizeof(NcaHeader));

        std::memcpy(dst, std::addressof(this->header), sizeof(NcaHeader));
    }

    DecryptAesCtrFunction NcaReader::GetExternalDecryptAesCtrFunction() const {
        AMS_ASSERT(this->decrypt_aes_ctr != nullptr);
        return this->decrypt_aes_ctr;
    }

    DecryptAesCtrFunction NcaReader::GetExternalDecryptAesCtrFunctionForExternalKey() const {
        AMS_ASSERT(this->decrypt_aes_ctr_external != nullptr);
        return this->decrypt_aes_ctr_external;
    }

    NcaHeader::EncryptionType NcaReader::GetEncryptionType() const {
        return this->header_encryption_type;
    }

    Result NcaReader::ReadHeader(NcaFsHeader *dst, s32 index) const {
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);

        const s64 offset = sizeof(NcaHeader) + sizeof(NcaFsHeader) * index;
        return this->header_storage->Read(offset, dst, sizeof(NcaFsHeader));
    }

    Result NcaReader::VerifyHeaderSign2(const void *mod, size_t mod_size) {
        AMS_ASSERT(this->body_storage != nullptr);
        constexpr const u8 HeaderSign2KeyPublicExponent[] = { 0x01, 0x00, 0x01 };

        const u8 *sig         = this->header.header_sign_2;
        const size_t sig_size = NcaHeader::HeaderSignSize;
        const u8 *exp         = HeaderSign2KeyPublicExponent;
        const size_t exp_size = sizeof(HeaderSign2KeyPublicExponent);
        const u8 *msg         = static_cast<const u8 *>(static_cast<const void *>(std::addressof(this->header.magic)));
        const size_t msg_size = NcaHeader::Size - NcaHeader::HeaderSignSize * NcaHeader::HeaderSignCount;
        const bool is_signature_valid = crypto::VerifyRsa2048PssSha256(sig, sig_size, mod, mod_size, exp, exp_size, msg, msg_size);
        R_UNLESS(is_signature_valid, fs::ResultNcaHeaderSignature2VerificationFailed());

        return ResultSuccess();
    }

    Result NcaFsHeaderReader::Initialize(const NcaReader &reader, s32 index) {
        /* Reset ourselves to uninitialized. */
        this->fs_index = -1;

        /* Read the header. */
        R_TRY(reader.ReadHeader(std::addressof(this->data), index));

        /* Generate the hash. */
        Hash hash;
        crypto::GenerateSha256Hash(std::addressof(hash), sizeof(hash), std::addressof(this->data), sizeof(NcaFsHeader));

        /* Validate the hash. */
        R_UNLESS(crypto::IsSameBytes(std::addressof(reader.GetFsHeaderHash(index)), std::addressof(hash), sizeof(Hash)), fs::ResultNcaFsHeaderHashVerificationFailed());

        /* Set our index. */
        this->fs_index = index;
        return ResultSuccess();
    }

    void NcaFsHeaderReader::GetRawData(void *dst, size_t dst_size) const {
        AMS_ASSERT(this->IsInitialized());
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(dst_size >= sizeof(NcaFsHeader));
        std::memcpy(dst, std::addressof(this->data), sizeof(NcaFsHeader));
    }

    NcaFsHeader::HashData &NcaFsHeaderReader::GetHashData() {
        AMS_ASSERT(this->IsInitialized());
        return this->data.hash_data;
    }

    const NcaFsHeader::HashData &NcaFsHeaderReader::GetHashData() const {
        AMS_ASSERT(this->IsInitialized());
        return this->data.hash_data;
    }

    u16 NcaFsHeaderReader::GetVersion() const {
        AMS_ASSERT(this->IsInitialized());
        return this->data.version;
    }

    s32 NcaFsHeaderReader::GetFsIndex() const {
        AMS_ASSERT(this->IsInitialized());
        return this->fs_index;
    }

    NcaFsHeader::FsType NcaFsHeaderReader::GetFsType() const {
        AMS_ASSERT(this->IsInitialized());
        return this->data.fs_type;
    }

    NcaFsHeader::HashType NcaFsHeaderReader::GetHashType() const {
        AMS_ASSERT(this->IsInitialized());
        return this->data.hash_type;
    }

    NcaFsHeader::EncryptionType NcaFsHeaderReader::GetEncryptionType() const {
        AMS_ASSERT(this->IsInitialized());
        return this->data.encryption_type;
    }

    NcaPatchInfo &NcaFsHeaderReader::GetPatchInfo() {
        AMS_ASSERT(this->IsInitialized());
        return this->data.patch_info;
    }

    const NcaPatchInfo &NcaFsHeaderReader::GetPatchInfo() const {
        AMS_ASSERT(this->IsInitialized());
        return this->data.patch_info;
    }

    const NcaAesCtrUpperIv NcaFsHeaderReader::GetAesCtrUpperIv() const {
        AMS_ASSERT(this->IsInitialized());
        return this->data.aes_ctr_upper_iv;
    }

    bool NcaFsHeaderReader::ExistsSparseLayer() const {
        AMS_ASSERT(this->IsInitialized());
        return this->data.sparse_info.generation != 0;
    }

    NcaSparseInfo &NcaFsHeaderReader::GetSparseInfo() {
        AMS_ASSERT(this->IsInitialized());
        return this->data.sparse_info;
    }

    const NcaSparseInfo &NcaFsHeaderReader::GetSparseInfo() const {
        AMS_ASSERT(this->IsInitialized());
        return this->data.sparse_info;
    }

}
