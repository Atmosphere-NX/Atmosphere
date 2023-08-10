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

            R_SUCCEED();
        }

    }

    NcaReader::NcaReader() : m_body_storage(), m_header_storage(), m_decrypt_aes_ctr(), m_decrypt_aes_ctr_external(), m_is_software_aes_prioritized(false), m_is_available_sw_key(false), m_header_encryption_type(NcaHeader::EncryptionType::Auto), m_get_decompressor(), m_hash_generator_factory_selector() {
        std::memset(std::addressof(m_header), 0, sizeof(m_header));
        std::memset(std::addressof(m_decryption_keys), 0, sizeof(m_decryption_keys));
        std::memset(std::addressof(m_external_decryption_key), 0, sizeof(m_external_decryption_key));
    }

    NcaReader::~NcaReader() {
        /* ... */
    }

    Result NcaReader::Initialize(std::shared_ptr<fs::IStorage> base_storage, const NcaCryptoConfiguration &crypto_cfg, const NcaCompressionConfiguration &compression_cfg, IHash256GeneratorFactorySelector *hgf_selector) {
        /* Validate preconditions. */
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(hgf_selector != nullptr);
        AMS_ASSERT(m_body_storage == nullptr);

        /* Check that the crypto config is valid. */
        R_UNLESS(crypto_cfg.verify_sign1 != nullptr, fs::ResultInvalidArgument());

        /* Create the work header storage storage. */
        std::unique_ptr<fs::IStorage> work_header_storage;
        if (crypto_cfg.is_available_sw_key) {
            /* If software key is available, we need to be able to generate keys. */
            R_UNLESS(crypto_cfg.generate_key != nullptr, fs::ResultInvalidArgument());

            /* Generate keys for header. */
            using AesXtsStorageForNcaHeader = AesXtsStorageBySharedPointer;

            constexpr const s32 HeaderKeyTypeValues[NcaCryptoConfiguration::HeaderEncryptionKeyCount] = {
                static_cast<s32>(KeyType::NcaHeaderKey1),
                static_cast<s32>(KeyType::NcaHeaderKey2),
            };

            u8 header_decryption_keys[NcaCryptoConfiguration::HeaderEncryptionKeyCount][NcaCryptoConfiguration::Aes128KeySize];
            for (size_t i = 0; i < NcaCryptoConfiguration::HeaderEncryptionKeyCount; i++) {
                crypto_cfg.generate_key(header_decryption_keys[i], AesXtsStorageForNcaHeader::KeySize, crypto_cfg.header_encrypted_encryption_keys[i], AesXtsStorageForNcaHeader::KeySize, HeaderKeyTypeValues[i]);
            }

            /* Create the header storage. */
            const u8 header_iv[AesXtsStorageForNcaHeader::IvSize] = {};
            work_header_storage = std::make_unique<AesXtsStorageForNcaHeader>(base_storage, header_decryption_keys[0], header_decryption_keys[1], AesXtsStorageForNcaHeader::KeySize, header_iv, AesXtsStorageForNcaHeader::IvSize, NcaHeader::XtsBlockSize);
        } else {
            /* Software key isn't available, so we need to be able to decrypt externally. */
            R_UNLESS(crypto_cfg.decrypt_aes_xts_external, fs::ResultInvalidArgument());

            /* Create the header storage. */
            using AesXtsStorageExternalForNcaHeader = AesXtsStorageExternalByPointer;

            const u8 header_iv[AesXtsStorageExternalForNcaHeader::IvSize] = {};
            work_header_storage = std::make_unique<AesXtsStorageExternalForNcaHeader>(base_storage.get(), nullptr, nullptr, AesXtsStorageExternalForNcaHeader::KeySize, header_iv, AesXtsStorageExternalForNcaHeader::IvSize, NcaHeader::XtsBlockSize, crypto_cfg.encrypt_aes_xts_external, crypto_cfg.decrypt_aes_xts_external);
        }

        /* Check that we successfully created the storage. */
        R_UNLESS(work_header_storage != nullptr, fs::ResultAllocationMemoryFailedInNcaReaderA());

        /* Read the header. */
        R_TRY(work_header_storage->Read(0, std::addressof(m_header), sizeof(m_header)));

        /* Validate the magic. */
        if (const Result magic_result = CheckNcaMagic(m_header.magic); R_FAILED(magic_result)) {
            /* If we're not allowed to use plaintext headers, stop here. */
            R_UNLESS(crypto_cfg.is_plaintext_header_available, magic_result);

            /* Try to use a plaintext header. */
            R_TRY(base_storage->Read(0, std::addressof(m_header), sizeof(m_header)));
            R_UNLESS(R_SUCCEEDED(CheckNcaMagic(m_header.magic)), magic_result);

            /* Configure to use the plaintext header. */
            s64 base_storage_size;
            R_TRY(base_storage->GetSize(std::addressof(base_storage_size)));
            work_header_storage.reset(new fs::SubStorage(base_storage, 0, base_storage_size));
            R_UNLESS(work_header_storage != nullptr, fs::ResultAllocationMemoryFailedInNcaReaderA());

            /* Set encryption type as plaintext. */
            m_header_encryption_type = NcaHeader::EncryptionType::None;
        }

        /* Validate the fixed key signature. */
        R_UNLESS(m_header.header1_signature_key_generation <= NcaCryptoConfiguration::Header1SignatureKeyGenerationMax, fs::ResultInvalidNcaHeader1SignatureKeyGeneration());

        /* Verify the header sign1. */
        {
            const u8 *sig         = m_header.header_sign_1;
            const size_t sig_size = NcaHeader::HeaderSignSize;
            const u8 *msg         = static_cast<const u8 *>(static_cast<const void *>(std::addressof(m_header.magic)));
            const size_t msg_size = NcaHeader::Size - NcaHeader::HeaderSignSize * NcaHeader::HeaderSignCount;

            m_is_header_sign1_signature_valid = crypto_cfg.verify_sign1(sig, sig_size, msg, msg_size, m_header.header1_signature_key_generation);

            #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
            R_UNLESS(m_is_header_sign1_signature_valid, fs::ResultNcaHeaderSignature1VerificationFailed());
            #else
            R_UNLESS(m_is_header_sign1_signature_valid || crypto_cfg.is_unsigned_header_available_for_host_tool, fs::ResultNcaHeaderSignature1VerificationFailed());
            #endif
        }

        /* Validate the sdk version. */
        R_UNLESS(m_header.sdk_addon_version >= SdkAddonVersionMin, fs::ResultUnsupportedSdkVersion());

        /* Validate the key index. */
        R_UNLESS(m_header.key_index < NcaCryptoConfiguration::KeyAreaEncryptionKeyIndexCount || m_header.key_index == NcaCryptoConfiguration::KeyAreaEncryptionKeyIndexZeroKey, fs::ResultInvalidNcaKeyIndex());

        /* Set our hash generator factory selector. */
        m_hash_generator_factory_selector = hgf_selector;

        /* Check if we have a rights id. */
        constexpr const u8 ZeroRightsId[NcaHeader::RightsIdSize] = {};
        if (crypto::IsSameBytes(ZeroRightsId, m_header.rights_id, NcaHeader::RightsIdSize)) {
            /* If we don't, then we don't have an external key, so we need to generate decryption keys if software keys are available. */
            if (crypto_cfg.is_available_sw_key) {
                crypto_cfg.generate_key(m_decryption_keys[NcaHeader::DecryptionKey_AesCtr], crypto::AesDecryptor128::KeySize, m_header.encrypted_key_area + NcaHeader::DecryptionKey_AesCtr * crypto::AesDecryptor128::KeySize, crypto::AesDecryptor128::KeySize, GetKeyTypeValue(m_header.key_index, m_header.GetProperKeyGeneration()));

                /* If we're building for non-nx board (i.e., a host tool), generate all keys for debug. */
                #if !defined(ATMOSPHERE_BOARD_NINTENDO_NX)
                crypto_cfg.generate_key(m_decryption_keys[NcaHeader::DecryptionKey_AesXts1], crypto::AesDecryptor128::KeySize, m_header.encrypted_key_area + NcaHeader::DecryptionKey_AesXts1 * crypto::AesDecryptor128::KeySize, crypto::AesDecryptor128::KeySize, GetKeyTypeValue(m_header.key_index, m_header.GetProperKeyGeneration()));
                crypto_cfg.generate_key(m_decryption_keys[NcaHeader::DecryptionKey_AesXts2], crypto::AesDecryptor128::KeySize, m_header.encrypted_key_area + NcaHeader::DecryptionKey_AesXts2 * crypto::AesDecryptor128::KeySize, crypto::AesDecryptor128::KeySize, GetKeyTypeValue(m_header.key_index, m_header.GetProperKeyGeneration()));
                crypto_cfg.generate_key(m_decryption_keys[NcaHeader::DecryptionKey_AesCtrEx], crypto::AesDecryptor128::KeySize, m_header.encrypted_key_area + NcaHeader::DecryptionKey_AesCtrEx * crypto::AesDecryptor128::KeySize, crypto::AesDecryptor128::KeySize, GetKeyTypeValue(m_header.key_index, m_header.GetProperKeyGeneration()));
                #endif
            }

            /* Copy the hardware speed emulation key. */
            std::memcpy(m_decryption_keys[NcaHeader::DecryptionKey_AesCtrHw], m_header.encrypted_key_area + NcaHeader::DecryptionKey_AesCtrHw * crypto::AesDecryptor128::KeySize, crypto::AesDecryptor128::KeySize);
        }

        /* Clear the external decryption key. */
        std::memset(m_external_decryption_key, 0, sizeof(m_external_decryption_key));

        /* Set software key availability. */
        m_is_available_sw_key = crypto_cfg.is_available_sw_key;

        /* Set our decryptor functions. */
        m_decrypt_aes_ctr          = crypto_cfg.decrypt_aes_ctr;
        m_decrypt_aes_ctr_external = crypto_cfg.decrypt_aes_ctr_external;

        /* Set our decompressor function getter. */
        m_get_decompressor = compression_cfg.get_decompressor;

        /* Set our storages. */
        m_header_storage = std::move(work_header_storage);
        m_body_storage   = std::move(base_storage);

        R_SUCCEED();
    }

    std::shared_ptr<fs::IStorage> NcaReader::GetSharedBodyStorage() {
        AMS_ASSERT(m_body_storage != nullptr);
        return m_body_storage;
    }

    u32 NcaReader::GetMagic() const {
        AMS_ASSERT(m_body_storage != nullptr);
        return m_header.magic;
    }

    NcaHeader::DistributionType NcaReader::GetDistributionType() const {
        AMS_ASSERT(m_body_storage != nullptr);
        return m_header.distribution_type;
    }

    NcaHeader::ContentType NcaReader::GetContentType() const {
        AMS_ASSERT(m_body_storage != nullptr);
        return m_header.content_type;
    }

    u8 NcaReader::GetHeaderSign1KeyGeneration() const {
        AMS_ASSERT(m_body_storage != nullptr);
        return m_header.header1_signature_key_generation;
    }

    u8 NcaReader::GetKeyGeneration() const {
        AMS_ASSERT(m_body_storage != nullptr);
        return m_header.GetProperKeyGeneration();
    }

    u8 NcaReader::GetKeyIndex() const {
        AMS_ASSERT(m_body_storage != nullptr);
        return m_header.key_index;
    }

    u64 NcaReader::GetContentSize() const {
        AMS_ASSERT(m_body_storage != nullptr);
        return m_header.content_size;
    }

    u64 NcaReader::GetProgramId() const {
        AMS_ASSERT(m_body_storage != nullptr);
        return m_header.program_id;
    }

    u32 NcaReader::GetContentIndex() const {
        AMS_ASSERT(m_body_storage != nullptr);
        return m_header.content_index;
    }

    u32 NcaReader::GetSdkAddonVersion() const {
        AMS_ASSERT(m_body_storage != nullptr);
        return m_header.sdk_addon_version;
    }

    void NcaReader::GetRightsId(u8 *dst, size_t dst_size) const {
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(dst_size >= NcaHeader::RightsIdSize);
        AMS_UNUSED(dst_size);

        std::memcpy(dst, m_header.rights_id, NcaHeader::RightsIdSize);
    }

    bool NcaReader::HasFsInfo(s32 index) const {
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        return m_header.fs_info[index].start_sector != 0 || m_header.fs_info[index].end_sector != 0;
    }

    s32 NcaReader::GetFsCount() const {
        AMS_ASSERT(m_body_storage != nullptr);
        for (s32 i = 0; i < NcaHeader::FsCountMax; i++) {
            if (!this->HasFsInfo(i)) {
                return i;
            }
        }
        return NcaHeader::FsCountMax;
    }

    const Hash &NcaReader::GetFsHeaderHash(s32 index) const {
        AMS_ASSERT(m_body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        return m_header.fs_header_hash[index];
    }

    void NcaReader::GetFsHeaderHash(Hash *dst, s32 index) const {
        AMS_ASSERT(m_body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        AMS_ASSERT(dst != nullptr);
        std::memcpy(dst, std::addressof(m_header.fs_header_hash[index]), sizeof(*dst));
    }

    void NcaReader::GetFsInfo(NcaHeader::FsInfo *dst, s32 index) const {
        AMS_ASSERT(m_body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        AMS_ASSERT(dst != nullptr);
        std::memcpy(dst, std::addressof(m_header.fs_info[index]), sizeof(*dst));
    }

    u64 NcaReader::GetFsOffset(s32 index) const {
        AMS_ASSERT(m_body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        return NcaHeader::SectorToByte(m_header.fs_info[index].start_sector);
    }

    u64 NcaReader::GetFsEndOffset(s32 index) const {
        AMS_ASSERT(m_body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        return NcaHeader::SectorToByte(m_header.fs_info[index].end_sector);
    }

    u64 NcaReader::GetFsSize(s32 index) const {
        AMS_ASSERT(m_body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
        return NcaHeader::SectorToByte(m_header.fs_info[index].end_sector - m_header.fs_info[index].start_sector);
    }

    void NcaReader::GetEncryptedKey(void *dst, size_t size) const {
        AMS_ASSERT(m_body_storage != nullptr);
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(size >= NcaHeader::EncryptedKeyAreaSize);
        AMS_UNUSED(size);

        std::memcpy(dst, m_header.encrypted_key_area, NcaHeader::EncryptedKeyAreaSize);
    }

    const void *NcaReader::GetDecryptionKey(s32 index) const {
        AMS_ASSERT(m_body_storage != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::DecryptionKey_Count);
        return m_decryption_keys[index];
    }

    bool NcaReader::HasValidInternalKey() const {
        constexpr const u8 ZeroKey[crypto::AesDecryptor128::KeySize] = {};
        for (s32 i = 0; i < NcaHeader::DecryptionKey_Count; i++) {
            if (!crypto::IsSameBytes(ZeroKey, m_header.encrypted_key_area + i * crypto::AesDecryptor128::KeySize, crypto::AesDecryptor128::KeySize)) {
                return true;
            }
        }
        return false;
    }

    bool NcaReader::HasInternalDecryptionKeyForAesHw() const {
        constexpr const u8 ZeroKey[crypto::AesDecryptor128::KeySize] = {};
        return !crypto::IsSameBytes(ZeroKey, this->GetDecryptionKey(NcaHeader::DecryptionKey_AesCtrHw), crypto::AesDecryptor128::KeySize);
    }

    bool NcaReader::IsSoftwareAesPrioritized() const {
        return m_is_software_aes_prioritized;
    }

    void NcaReader::PrioritizeSoftwareAes() {
        m_is_software_aes_prioritized = true;
    }

    bool NcaReader::IsAvailableSwKey() const {
        return m_is_available_sw_key;
    }

    bool NcaReader::HasExternalDecryptionKey() const {
        constexpr const u8 ZeroKey[crypto::AesDecryptor128::KeySize] = {};
        return !crypto::IsSameBytes(ZeroKey, this->GetExternalDecryptionKey(), crypto::AesDecryptor128::KeySize);
    }

    const void *NcaReader::GetExternalDecryptionKey() const {
        return m_external_decryption_key;
    }

    void NcaReader::SetExternalDecryptionKey(const void *src, size_t size) {
        AMS_ASSERT(src != nullptr);
        AMS_ASSERT(size == sizeof(m_external_decryption_key));
        AMS_UNUSED(size);

        std::memcpy(m_external_decryption_key, src, sizeof(m_external_decryption_key));
    }

    void NcaReader::GetRawData(void *dst, size_t dst_size) const {
        AMS_ASSERT(m_body_storage != nullptr);
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(dst_size >= sizeof(NcaHeader));
        AMS_UNUSED(dst_size);

        std::memcpy(dst, std::addressof(m_header), sizeof(NcaHeader));
    }

    DecryptAesCtrFunction NcaReader::GetExternalDecryptAesCtrFunction() const {
        AMS_ASSERT(m_decrypt_aes_ctr != nullptr);
        return m_decrypt_aes_ctr;
    }

    DecryptAesCtrFunction NcaReader::GetExternalDecryptAesCtrFunctionForExternalKey() const {
        AMS_ASSERT(m_decrypt_aes_ctr_external != nullptr);
        return m_decrypt_aes_ctr_external;
    }

    GetDecompressorFunction NcaReader::GetDecompressor() const {
        AMS_ASSERT(m_get_decompressor != nullptr);
        return m_get_decompressor;
    }

    IHash256GeneratorFactorySelector *NcaReader::GetHashGeneratorFactorySelector() const {
        AMS_ASSERT(m_hash_generator_factory_selector != nullptr);
        return m_hash_generator_factory_selector;
    }

    NcaHeader::EncryptionType NcaReader::GetEncryptionType() const {
        return m_header_encryption_type;
    }

    Result NcaReader::ReadHeader(NcaFsHeader *dst, s32 index) const {
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);

        const s64 offset = sizeof(NcaHeader) + sizeof(NcaFsHeader) * index;
        R_RETURN(m_header_storage->Read(offset, dst, sizeof(NcaFsHeader)));
    }

    bool NcaReader::GetHeaderSign1Valid() const {
        #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
        AMS_ABORT_UNLESS(m_is_header_sign1_signature_valid);
        #endif

        return m_is_header_sign1_signature_valid;
    }

    void NcaReader::GetHeaderSign2(void *dst, size_t size) const {
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(size == NcaHeader::HeaderSignSize);

        std::memcpy(dst, m_header.header_sign_2, size);
    }

    void NcaReader::GetHeaderSign2TargetHash(void *dst, size_t size) const {
        AMS_ASSERT(m_hash_generator_factory_selector!= nullptr);
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(size == IHash256Generator::HashSize);

        auto * const factory = m_hash_generator_factory_selector->GetFactory(fssystem::HashAlgorithmType_Sha2);
        return factory->GenerateHash(dst, size, static_cast<const void *>(std::addressof(m_header.magic)), NcaHeader::Size - NcaHeader::HeaderSignSize * NcaHeader::HeaderSignCount);
    }

    Result NcaFsHeaderReader::Initialize(const NcaReader &reader, s32 index) {
        /* Reset ourselves to uninitialized. */
        m_fs_index = -1;

        /* Read the header. */
        R_TRY(reader.ReadHeader(std::addressof(m_data), index));

        /* Generate the hash. */
        Hash hash;
        crypto::GenerateSha256(std::addressof(hash), sizeof(hash), std::addressof(m_data), sizeof(NcaFsHeader));

        /* Validate the hash. */
        R_UNLESS(crypto::IsSameBytes(std::addressof(reader.GetFsHeaderHash(index)), std::addressof(hash), sizeof(Hash)), fs::ResultNcaFsHeaderHashVerificationFailed());

        /* Set our index. */
        m_fs_index = index;
        R_SUCCEED();
    }

    void NcaFsHeaderReader::GetRawData(void *dst, size_t dst_size) const {
        AMS_ASSERT(this->IsInitialized());
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(dst_size >= sizeof(NcaFsHeader));
        AMS_UNUSED(dst_size);

        std::memcpy(dst, std::addressof(m_data), sizeof(NcaFsHeader));
    }

    NcaFsHeader::HashData &NcaFsHeaderReader::GetHashData() {
        AMS_ASSERT(this->IsInitialized());
        return m_data.hash_data;
    }

    const NcaFsHeader::HashData &NcaFsHeaderReader::GetHashData() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.hash_data;
    }

    u16 NcaFsHeaderReader::GetVersion() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.version;
    }

    s32 NcaFsHeaderReader::GetFsIndex() const {
        AMS_ASSERT(this->IsInitialized());
        return m_fs_index;
    }

    NcaFsHeader::FsType NcaFsHeaderReader::GetFsType() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.fs_type;
    }

    NcaFsHeader::HashType NcaFsHeaderReader::GetHashType() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.hash_type;
    }

    NcaFsHeader::EncryptionType NcaFsHeaderReader::GetEncryptionType() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.encryption_type;
    }

    NcaPatchInfo &NcaFsHeaderReader::GetPatchInfo() {
        AMS_ASSERT(this->IsInitialized());
        return m_data.patch_info;
    }

    const NcaPatchInfo &NcaFsHeaderReader::GetPatchInfo() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.patch_info;
    }

    const NcaAesCtrUpperIv NcaFsHeaderReader::GetAesCtrUpperIv() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.aes_ctr_upper_iv;
    }

    bool NcaFsHeaderReader::IsSkipLayerHashEncryption() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.IsSkipLayerHashEncryption();
    }

    Result NcaFsHeaderReader::GetHashTargetOffset(s64 *out) const {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(this->IsInitialized());

        R_RETURN(m_data.GetHashTargetOffset(out));
    }

    bool NcaFsHeaderReader::ExistsSparseLayer() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.sparse_info.generation != 0;
    }

    NcaSparseInfo &NcaFsHeaderReader::GetSparseInfo() {
        AMS_ASSERT(this->IsInitialized());
        return m_data.sparse_info;
    }

    const NcaSparseInfo &NcaFsHeaderReader::GetSparseInfo() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.sparse_info;
    }

    bool NcaFsHeaderReader::ExistsCompressionLayer() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.compression_info.bucket.offset != 0 && m_data.compression_info.bucket.size != 0;
    }

    NcaCompressionInfo &NcaFsHeaderReader::GetCompressionInfo() {
        AMS_ASSERT(this->IsInitialized());
        return m_data.compression_info;
    }

    const NcaCompressionInfo &NcaFsHeaderReader::GetCompressionInfo() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.compression_info;
    }

    bool NcaFsHeaderReader::ExistsPatchMetaHashLayer() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.meta_data_hash_data_info.size != 0 && this->GetPatchInfo().HasIndirectTable();
    }

    NcaMetaDataHashDataInfo &NcaFsHeaderReader::GetPatchMetaDataHashDataInfo() {
        AMS_ASSERT(this->IsInitialized());
        return m_data.meta_data_hash_data_info;
    }

    const NcaMetaDataHashDataInfo &NcaFsHeaderReader::GetPatchMetaDataHashDataInfo() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.meta_data_hash_data_info;
    }

    NcaFsHeader::MetaDataHashType NcaFsHeaderReader::GetPatchMetaHashType() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.meta_data_hash_type;
    }

    bool NcaFsHeaderReader::ExistsSparseMetaHashLayer() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.meta_data_hash_data_info.size != 0 && this->ExistsSparseLayer();
    }

    NcaMetaDataHashDataInfo &NcaFsHeaderReader::GetSparseMetaDataHashDataInfo() {
        AMS_ASSERT(this->IsInitialized());
        return m_data.meta_data_hash_data_info;
    }

    const NcaMetaDataHashDataInfo &NcaFsHeaderReader::GetSparseMetaDataHashDataInfo() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.meta_data_hash_data_info;
    }

    NcaFsHeader::MetaDataHashType NcaFsHeaderReader::GetSparseMetaHashType() const {
        AMS_ASSERT(this->IsInitialized());
        return m_data.meta_data_hash_type;
    }

}
