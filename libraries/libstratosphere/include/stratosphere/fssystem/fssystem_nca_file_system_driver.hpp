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
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fssystem/fssystem_compression_common.hpp>
#include <stratosphere/fssystem/fssystem_i_hash_256_generator.hpp>
#include <stratosphere/fssystem/fssystem_asynchronous_access.hpp>
#include <stratosphere/fssystem/fssystem_nca_header.hpp>
#include <stratosphere/fs/fs_i_buffer_manager.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 14.3.0.0 */

    class CompressedStorage;
    class AesCtrCounterExtendedStorage;
    class IndirectStorage;
    class SparseStorage;

    struct NcaCryptoConfiguration;

    using KeyGenerationFunction = void (*)(void *dst_key, size_t dst_key_size, const void *src_key, size_t src_key_size, s32 key_type);
    using DecryptAesCtrFunction = void (*)(void *dst, size_t dst_size, u8 key_index, u8 key_generation, const void *src_key, size_t src_key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);

    using CryptAesXtsFunction   = Result (*)(void *dst, size_t dst_size, const void *key1, const void *key2, size_t key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);
    using VerifySign1Function   = bool (*)(const void *sig, size_t sig_size, const void *data, size_t data_size, u8 generation);

    struct NcaCryptoConfiguration {
        static constexpr size_t Rsa2048KeyModulusSize         = crypto::Rsa2048PssSha256Verifier::ModulusSize;
        static constexpr size_t Rsa2048KeyPublicExponentSize  = crypto::Rsa2048PssSha256Verifier::MaximumExponentSize;
        static constexpr size_t Rsa2048KeyPrivateExponentSize = Rsa2048KeyModulusSize;

        static constexpr size_t Aes128KeySize = crypto::AesEncryptor128::KeySize;

        static constexpr size_t Header1SignatureKeyGenerationMax = 1;

        static constexpr s32 KeyAreaEncryptionKeyIndexCount = 3;
        static constexpr s32 HeaderEncryptionKeyCount       = 2;

        static constexpr u8 KeyAreaEncryptionKeyIndexZeroKey = 0xFF;

        static constexpr size_t KeyGenerationMax = 32;

        const u8 *header_1_sign_key_moduli[Header1SignatureKeyGenerationMax + 1];
        u8 header_1_sign_key_public_exponent[Rsa2048KeyPublicExponentSize];
        u8 key_area_encryption_key_source[KeyAreaEncryptionKeyIndexCount][Aes128KeySize];
        u8 header_encryption_key_source[Aes128KeySize];
        u8 header_encrypted_encryption_keys[HeaderEncryptionKeyCount][Aes128KeySize];
        KeyGenerationFunction generate_key;
        CryptAesXtsFunction decrypt_aes_xts_external;
        CryptAesXtsFunction encrypt_aes_xts_external;
        DecryptAesCtrFunction decrypt_aes_ctr;
        DecryptAesCtrFunction decrypt_aes_ctr_external;
        VerifySign1Function verify_sign1;
        bool is_plaintext_header_available;
        bool is_available_sw_key;

        #if !defined(ATMOSPHERE_BOARD_NINTENDO_NX)
        bool is_unsigned_header_available_for_host_tool;
        #endif
    };
    static_assert(util::is_pod<NcaCryptoConfiguration>::value);

    struct NcaCompressionConfiguration {
        GetDecompressorFunction get_decompressor;
    };
    static_assert(util::is_pod<NcaCompressionConfiguration>::value);

    constexpr inline s32 KeyAreaEncryptionKeyCount = NcaCryptoConfiguration::KeyAreaEncryptionKeyIndexCount * NcaCryptoConfiguration::KeyGenerationMax;

    enum class KeyType : s32 {
        ZeroKey                 = -2,
        InvalidKey              = -1,
        NcaHeaderKey1           = KeyAreaEncryptionKeyCount + 0,
        NcaHeaderKey2           = KeyAreaEncryptionKeyCount + 1,
        NcaExternalKey          = KeyAreaEncryptionKeyCount + 2,
        SaveDataDeviceUniqueMac = KeyAreaEncryptionKeyCount + 3,
        SaveDataSeedUniqueMac   = KeyAreaEncryptionKeyCount + 4,
        SaveDataTransferMac     = KeyAreaEncryptionKeyCount + 5,
    };

    constexpr inline bool IsInvalidKeyTypeValue(s32 key_type) {
        return key_type < 0;
    }

    constexpr inline s32 GetKeyTypeValue(u8 key_index, u8 key_generation) {
        if (key_index == NcaCryptoConfiguration::KeyAreaEncryptionKeyIndexZeroKey) {
            return util::ToUnderlying(KeyType::ZeroKey);
        }

        if (key_index >= NcaCryptoConfiguration::KeyAreaEncryptionKeyIndexCount) {
            return util::ToUnderlying(KeyType::InvalidKey);
        }

        return NcaCryptoConfiguration::KeyAreaEncryptionKeyIndexCount * key_generation + key_index;
    }

    class NcaReader : public ::ams::fs::impl::Newable {
        NON_COPYABLE(NcaReader);
        NON_MOVEABLE(NcaReader);
        private:
            NcaHeader m_header;
            u8 m_decryption_keys[NcaHeader::DecryptionKey_Count][NcaCryptoConfiguration::Aes128KeySize];
            std::shared_ptr<fs::IStorage> m_body_storage;
            std::unique_ptr<fs::IStorage> m_header_storage;
            u8 m_external_decryption_key[NcaCryptoConfiguration::Aes128KeySize];
            DecryptAesCtrFunction m_decrypt_aes_ctr;
            DecryptAesCtrFunction m_decrypt_aes_ctr_external;
            bool m_is_software_aes_prioritized;
            bool m_is_available_sw_key;
            NcaHeader::EncryptionType m_header_encryption_type;
            bool m_is_header_sign1_signature_valid;
            GetDecompressorFunction m_get_decompressor;
            IHash256GeneratorFactorySelector *m_hash_generator_factory_selector;
        public:
            NcaReader();
            ~NcaReader();

            Result Initialize(std::shared_ptr<fs::IStorage> base_storage, const NcaCryptoConfiguration &crypto_cfg, const NcaCompressionConfiguration &compression_cfg, IHash256GeneratorFactorySelector *hgf_selector);

            std::shared_ptr<fs::IStorage> GetSharedBodyStorage();
            u32 GetMagic() const;
            NcaHeader::DistributionType GetDistributionType() const;
            NcaHeader::ContentType GetContentType() const;
            u8  GetHeaderSign1KeyGeneration() const;
            u8  GetKeyGeneration() const;
            u8  GetKeyIndex() const;
            u64 GetContentSize() const;
            u64 GetProgramId() const;
            u32 GetContentIndex() const;
            u32 GetSdkAddonVersion() const;
            void GetRightsId(u8 *dst, size_t dst_size) const;
            bool HasFsInfo(s32 index) const;
            s32 GetFsCount() const;
            const Hash &GetFsHeaderHash(s32 index) const;
            void GetFsHeaderHash(Hash *dst, s32 index) const;
            void GetFsInfo(NcaHeader::FsInfo *dst, s32 index) const;
            u64 GetFsOffset(s32 index) const;
            u64 GetFsEndOffset(s32 index) const;
            u64 GetFsSize(s32 index) const;
            void GetEncryptedKey(void *dst, size_t size) const;
            const void *GetDecryptionKey(s32 index) const;
            bool HasValidInternalKey() const;
            bool HasInternalDecryptionKeyForAesHw() const;
            bool IsSoftwareAesPrioritized() const;
            void PrioritizeSoftwareAes();
            bool IsAvailableSwKey() const;
            bool HasExternalDecryptionKey() const;
            const void *GetExternalDecryptionKey() const;
            void SetExternalDecryptionKey(const void *src, size_t size);
            void GetRawData(void *dst, size_t dst_size) const;
            DecryptAesCtrFunction GetExternalDecryptAesCtrFunction() const;
            DecryptAesCtrFunction GetExternalDecryptAesCtrFunctionForExternalKey() const;
            NcaHeader::EncryptionType GetEncryptionType() const;
            Result ReadHeader(NcaFsHeader *dst, s32 index) const;

            GetDecompressorFunction GetDecompressor() const;
            IHash256GeneratorFactorySelector *GetHashGeneratorFactorySelector() const;

            bool GetHeaderSign1Valid() const;

            void GetHeaderSign2(void *dst, size_t size) const;
            void GetHeaderSign2TargetHash(void *dst, size_t size) const;
    };

    class NcaFsHeaderReader : public ::ams::fs::impl::Newable {
        NON_COPYABLE(NcaFsHeaderReader);
        NON_MOVEABLE(NcaFsHeaderReader);
        private:
            NcaFsHeader m_data;
            s32 m_fs_index;
        public:
            NcaFsHeaderReader() : m_fs_index(-1) {
                std::memset(std::addressof(m_data), 0, sizeof(m_data));
            }

            Result Initialize(const NcaReader &reader, s32 index);
            bool IsInitialized() const { return m_fs_index >= 0; }

            // NcaFsHeader &GetData() { return m_data; }
            // const NcaFsHeader &GetData() const { return m_data; }

            void GetRawData(void *dst, size_t dst_size) const;

            NcaFsHeader::HashData &GetHashData();
            const NcaFsHeader::HashData &GetHashData() const;
            u16 GetVersion() const;
            s32 GetFsIndex() const;
            NcaFsHeader::FsType GetFsType() const;
            NcaFsHeader::HashType GetHashType() const;
            NcaFsHeader::EncryptionType GetEncryptionType() const;
            NcaPatchInfo &GetPatchInfo();
            const NcaPatchInfo &GetPatchInfo() const;
            const NcaAesCtrUpperIv GetAesCtrUpperIv() const;

            bool IsSkipLayerHashEncryption() const;
            Result GetHashTargetOffset(s64 *out) const;

            bool ExistsSparseLayer() const;
            NcaSparseInfo &GetSparseInfo();
            const NcaSparseInfo &GetSparseInfo() const;

            bool ExistsCompressionLayer() const;
            NcaCompressionInfo &GetCompressionInfo();
            const NcaCompressionInfo &GetCompressionInfo() const;

            bool ExistsPatchMetaHashLayer() const;
            NcaMetaDataHashDataInfo &GetPatchMetaDataHashDataInfo();
            const NcaMetaDataHashDataInfo &GetPatchMetaDataHashDataInfo() const;
            NcaFsHeader::MetaDataHashType GetPatchMetaHashType() const;

            bool ExistsSparseMetaHashLayer() const;
            NcaMetaDataHashDataInfo &GetSparseMetaDataHashDataInfo();
            const NcaMetaDataHashDataInfo &GetSparseMetaDataHashDataInfo() const;
            NcaFsHeader::MetaDataHashType GetSparseMetaHashType() const;
    };

    class NcaFileSystemDriver : public ::ams::fs::impl::Newable {
        NON_COPYABLE(NcaFileSystemDriver);
        NON_MOVEABLE(NcaFileSystemDriver);
        #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
        private:
        #else
        public:
        #endif
            struct StorageContext {
                bool open_raw_storage;
                std::shared_ptr<fs::IStorage> body_substorage;
                std::shared_ptr<fssystem::SparseStorage> current_sparse_storage;
                std::shared_ptr<fs::IStorage> sparse_storage_meta_storage;
                std::shared_ptr<fssystem::SparseStorage> original_sparse_storage;
                void *external_current_sparse_storage;  /* TODO: Add real type? */
                void *external_original_sparse_storage; /* TODO: Add real type? */
                std::shared_ptr<fs::IStorage> aes_ctr_ex_storage_meta_storage;
                std::shared_ptr<fs::IStorage> aes_ctr_ex_storage_data_storage;
                std::shared_ptr<fssystem::AesCtrCounterExtendedStorage> aes_ctr_ex_storage;
                std::shared_ptr<fs::IStorage> indirect_storage_meta_storage;
                std::shared_ptr<fssystem::IndirectStorage> indirect_storage;
                std::shared_ptr<fs::IStorage> fs_data_storage;
                std::shared_ptr<fs::IStorage> compressed_storage_meta_storage;
                std::shared_ptr<fssystem::CompressedStorage> compressed_storage;

                std::shared_ptr<fs::IStorage> patch_layer_info_storage;
                std::shared_ptr<fs::IStorage> sparse_layer_info_storage;

                /* For tools. */
                std::shared_ptr<fs::IStorage> external_original_storage;
            };
        private:
            enum AlignmentStorageRequirement {
                /* TODO */
                AlignmentStorageRequirement_CacheBlockSize = 0,
                AlignmentStorageRequirement_None           = 1,
            };
        private:
            std::shared_ptr<NcaReader> m_original_reader;
            std::shared_ptr<NcaReader> m_reader;
            MemoryResource * const m_allocator;
            fs::IBufferManager * const m_buffer_manager;
            fssystem::IHash256GeneratorFactorySelector * const m_hash_generator_factory_selector;
        public:
            static Result SetupFsHeaderReader(NcaFsHeaderReader *out, const NcaReader &reader, s32 fs_index);
        public:
            NcaFileSystemDriver(std::shared_ptr<NcaReader> reader, MemoryResource *allocator, fs::IBufferManager *buffer_manager, IHash256GeneratorFactorySelector *hgf_selector) : m_original_reader(), m_reader(reader), m_allocator(allocator), m_buffer_manager(buffer_manager), m_hash_generator_factory_selector(hgf_selector) {
                AMS_ASSERT(m_reader != nullptr);
                AMS_ASSERT(m_hash_generator_factory_selector != nullptr);
            }

            NcaFileSystemDriver(std::shared_ptr<NcaReader> original_reader, std::shared_ptr<NcaReader> reader, MemoryResource *allocator, fs::IBufferManager *buffer_manager, IHash256GeneratorFactorySelector *hgf_selector) : m_original_reader(original_reader), m_reader(reader), m_allocator(allocator), m_buffer_manager(buffer_manager), m_hash_generator_factory_selector(hgf_selector) {
                AMS_ASSERT(m_reader != nullptr);
                AMS_ASSERT(m_hash_generator_factory_selector != nullptr);
            }

            Result OpenStorageWithContext(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<IAsynchronousAccessSplitter> *out_splitter, NcaFsHeaderReader *out_header_reader, s32 fs_index, StorageContext *ctx);

            Result OpenStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<IAsynchronousAccessSplitter> *out_splitter, NcaFsHeaderReader *out_header_reader, s32 fs_index) {
                /* Create a storage context. */
                StorageContext ctx{};

                /* Open the storage. */
                R_RETURN(OpenStorageWithContext(out, out_splitter, out_header_reader, fs_index, std::addressof(ctx)));
            }

        #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
        private:
        #else
        public:
        #endif
            Result CreateStorageByRawStorage(std::shared_ptr<fs::IStorage> *out, const NcaFsHeaderReader *header_reader, std::shared_ptr<fs::IStorage> raw_storage, StorageContext *ctx);
        private:
            Result OpenStorageImpl(std::shared_ptr<fs::IStorage> *out, NcaFsHeaderReader *out_header_reader, s32 fs_index, StorageContext *ctx);

            Result OpenIndirectableStorageAsOriginal(std::shared_ptr<fs::IStorage> *out, const NcaFsHeaderReader *header_reader, StorageContext *ctx);

            Result CreateBodySubStorage(std::shared_ptr<fs::IStorage> *out, s64 offset, s64 size);

            Result CreateAesCtrStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, s64 offset, const NcaAesCtrUpperIv &upper_iv, AlignmentStorageRequirement alignment_storage_requirement);
            Result CreateAesXtsStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, s64 offset);

            Result CreateSparseStorageMetaStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, s64 offset, const NcaAesCtrUpperIv &upper_iv, const NcaSparseInfo &sparse_info);
            Result CreateSparseStorageCore(std::shared_ptr<fssystem::SparseStorage> *out, std::shared_ptr<fs::IStorage> base_storage, s64 base_size, std::shared_ptr<fs::IStorage> meta_storage, const NcaSparseInfo &sparse_info, bool external_info);
            Result CreateSparseStorage(std::shared_ptr<fs::IStorage> *out, s64 *out_fs_data_offset, std::shared_ptr<fssystem::SparseStorage> *out_sparse_storage, std::shared_ptr<fs::IStorage> *out_meta_storage, s32 index, const NcaAesCtrUpperIv &upper_iv, const NcaSparseInfo &sparse_info);

            Result CreateSparseStorageMetaStorageWithVerification(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> *out_verification, std::shared_ptr<fs::IStorage> base_storage, s64 offset, const NcaAesCtrUpperIv &upper_iv, const NcaSparseInfo &sparse_info, const NcaMetaDataHashDataInfo &meta_data_hash_data_info, IHash256GeneratorFactory *hgf);
            Result CreateSparseStorageWithVerification(std::shared_ptr<fs::IStorage> *out, s64 *out_fs_data_offset, std::shared_ptr<fssystem::SparseStorage> *out_sparse_storage, std::shared_ptr<fs::IStorage> *out_meta_storage, std::shared_ptr<fs::IStorage> *out_verification, s32 index, const NcaAesCtrUpperIv &upper_iv, const NcaSparseInfo &sparse_info, const NcaMetaDataHashDataInfo &meta_data_hash_data_info, NcaFsHeader::MetaDataHashType meta_data_hash_type);

            Result CreateAesCtrExStorageMetaStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, s64 offset, NcaFsHeader::EncryptionType encryption_type, const NcaAesCtrUpperIv &upper_iv, const NcaPatchInfo &patch_info);
            Result CreateAesCtrExStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::AesCtrCounterExtendedStorage> *out_ext, std::shared_ptr<fs::IStorage> base_storage, std::shared_ptr<fs::IStorage> meta_storage, s64 counter_offset, const NcaAesCtrUpperIv &upper_iv, const NcaPatchInfo &patch_info);

            Result CreateIndirectStorageMetaStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, const NcaPatchInfo &patch_info);
            Result CreateIndirectStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::IndirectStorage> *out_ind, std::shared_ptr<fs::IStorage> base_storage, std::shared_ptr<fs::IStorage> original_data_storage, std::shared_ptr<fs::IStorage> meta_storage, const NcaPatchInfo &patch_info);

            Result CreatePatchMetaStorage(std::shared_ptr<fs::IStorage> *out_aes_ctr_ex_meta, std::shared_ptr<fs::IStorage> *out_indirect_meta, std::shared_ptr<fs::IStorage> *out_verification, std::shared_ptr<fs::IStorage> base_storage, s64 offset, const NcaAesCtrUpperIv &upper_iv, const NcaPatchInfo &patch_info, const NcaMetaDataHashDataInfo &meta_data_hash_data_info, IHash256GeneratorFactory *hgf);

            Result CreateSha256Storage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, const NcaFsHeader::HashData::HierarchicalSha256Data &sha256_data, IHash256GeneratorFactory *hgf);

            Result CreateIntegrityVerificationStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, const NcaFsHeader::HashData::IntegrityMetaInfo &meta_info, IHash256GeneratorFactory *hgf);
            Result CreateIntegrityVerificationStorageForMeta(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> *out_verification, std::shared_ptr<fs::IStorage> base_storage, s64 offset, const NcaMetaDataHashDataInfo &meta_data_hash_data_info, IHash256GeneratorFactory *hgf);
            Result CreateIntegrityVerificationStorageImpl(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, const NcaFsHeader::HashData::IntegrityMetaInfo &meta_info, s64 layer_info_offset, int max_data_cache_entries, int max_hash_cache_entries, s8 buffer_level, IHash256GeneratorFactory *hgf);

            Result CreateRegionSwitchStorage(std::shared_ptr<fs::IStorage> *out, const NcaFsHeaderReader *header_reader, std::shared_ptr<fs::IStorage> inside_storage, std::shared_ptr<fs::IStorage> outside_storage);

            Result CreateCompressedStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::CompressedStorage> *out_cmp, std::shared_ptr<fs::IStorage> *out_meta, std::shared_ptr<fs::IStorage> base_storage, const NcaCompressionInfo &compression_info);
        public:
            Result CreateCompressedStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::CompressedStorage> *out_cmp, std::shared_ptr<fs::IStorage> *out_meta, std::shared_ptr<fs::IStorage> base_storage, const NcaCompressionInfo &compression_info, GetDecompressorFunction get_decompressor, MemoryResource *allocator, fs::IBufferManager *buffer_manager);
    };

}
