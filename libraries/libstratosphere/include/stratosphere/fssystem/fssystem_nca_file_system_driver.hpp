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
#pragma once
#include <vapours.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fssystem/fssystem_nca_header.hpp>
#include <stratosphere/fssystem/buffers/fssystem_i_buffer_manager.hpp>

namespace ams::fssystem {

    class AesCtrCounterExtendedStorage;
    class IndirectStorage;
    class SparseStorage;

    struct NcaCryptoConfiguration;

    using KeyGenerationFunction = void (*)(void *dst_key, size_t dst_key_size, const void *src_key, size_t src_key_size, s32 key_type, const NcaCryptoConfiguration &cfg);
    using DecryptAesCtrFunction = void (*)(void *dst, size_t dst_size, s32 key_type, const void *src_key, size_t src_key_size, const void *iv, size_t iv_size, const void *src, size_t src_size);

    struct NcaCryptoConfiguration {
        static constexpr size_t Rsa2048KeyModulusSize         = crypto::Rsa2048PssSha256Verifier::ModulusSize;
        static constexpr size_t Rsa2048KeyPublicExponentSize  = crypto::Rsa2048PssSha256Verifier::MaximumExponentSize;
        static constexpr size_t Rsa2048KeyPrivateExponentSize = Rsa2048KeyModulusSize;

        static constexpr size_t Aes128KeySize = crypto::AesEncryptor128::KeySize;

        static constexpr size_t Header1SignatureKeyGenerationMax = 1;

        static constexpr s32 KeyAreaEncryptionKeyIndexCount = 3;
        static constexpr s32 HeaderEncryptionKeyCount       = 2;

        static constexpr size_t KeyGenerationMax = 32;

        const u8 *header_1_sign_key_moduli[Header1SignatureKeyGenerationMax + 1];
        u8 header_1_sign_key_public_exponent[Rsa2048KeyPublicExponentSize];
        u8 key_area_encryption_key_source[KeyAreaEncryptionKeyIndexCount][Aes128KeySize];
        u8 header_encryption_key_source[Aes128KeySize];
        u8 header_encrypted_encryption_keys[HeaderEncryptionKeyCount][Aes128KeySize];
        KeyGenerationFunction generate_key;
        DecryptAesCtrFunction decrypt_aes_ctr;
        DecryptAesCtrFunction decrypt_aes_ctr_external;
        bool is_plaintext_header_available;
    };
    static_assert(util::is_pod<NcaCryptoConfiguration>::value);

    constexpr inline bool IsInvalidKeyTypeValue(s32 key_type) {
        return key_type < 0;
    }

    constexpr inline s32 GetKeyTypeValue(u8 key_index, u8 key_generation) {
        constexpr s32 InvalidKeyTypeValue = -1;
        static_assert(IsInvalidKeyTypeValue(InvalidKeyTypeValue));

        if (key_index >= NcaCryptoConfiguration::KeyAreaEncryptionKeyIndexCount) {
            return InvalidKeyTypeValue;
        }

        return NcaCryptoConfiguration::KeyAreaEncryptionKeyIndexCount * key_generation + key_index;
    }

    constexpr inline s32 KeyAreaEncryptionKeyCount = NcaCryptoConfiguration::KeyAreaEncryptionKeyIndexCount * NcaCryptoConfiguration::KeyGenerationMax;

    enum class KeyType : s32 {
        NcaHeaderKey            = KeyAreaEncryptionKeyCount + 0,
        NcaExternalKey          = KeyAreaEncryptionKeyCount + 1,
        SaveDataDeviceUniqueMac = KeyAreaEncryptionKeyCount + 2,
        SaveDataSeedUniqueMac   = KeyAreaEncryptionKeyCount + 3,
    };

    class NcaReader : public ::ams::fs::impl::Newable {
        NON_COPYABLE(NcaReader);
        NON_MOVEABLE(NcaReader);
        private:
            NcaHeader header;
            u8 decryption_keys[NcaHeader::DecryptionKey_Count][NcaCryptoConfiguration::Aes128KeySize];
            std::shared_ptr<fs::IStorage> shared_base_storage;
            std::unique_ptr<fs::IStorage> header_storage;
            fs::IStorage *body_storage;
            u8 external_decryption_key[NcaCryptoConfiguration::Aes128KeySize];
            DecryptAesCtrFunction decrypt_aes_ctr;
            DecryptAesCtrFunction decrypt_aes_ctr_external;
            bool is_software_aes_prioritized;
            NcaHeader::EncryptionType header_encryption_type;
        public:
            NcaReader();
            ~NcaReader();

            Result Initialize(fs::IStorage *base_storage, const NcaCryptoConfiguration &crypto_cfg);
            Result Initialize(std::shared_ptr<fs::IStorage> base_storage, const NcaCryptoConfiguration &crypto_cfg);

            fs::IStorage *GetBodyStorage();
            u32 GetMagic() const;
            NcaHeader::DistributionType GetDistributionType() const;
            NcaHeader::ContentType GetContentType() const;
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
            bool HasInternalDecryptionKeyForAesHardwareSpeedEmulation() const;
            bool IsSoftwareAesPrioritized() const;
            void PrioritizeSoftwareAes();
            bool HasExternalDecryptionKey() const;
            const void *GetExternalDecryptionKey() const;
            void SetExternalDecryptionKey(const void *src, size_t size);
            void GetRawData(void *dst, size_t dst_size) const;
            DecryptAesCtrFunction GetExternalDecryptAesCtrFunction() const;
            DecryptAesCtrFunction GetExternalDecryptAesCtrFunctionForExternalKey() const;
            NcaHeader::EncryptionType GetEncryptionType() const;
            Result ReadHeader(NcaFsHeader *dst, s32 index) const;

            Result VerifyHeaderSign2(const void *key, size_t key_size);
    };

    class NcaFsHeaderReader : public ::ams::fs::impl::Newable {
        NON_COPYABLE(NcaFsHeaderReader);
        NON_MOVEABLE(NcaFsHeaderReader);
        private:
            NcaFsHeader data;
            s32 fs_index;
        public:
            NcaFsHeaderReader() : fs_index(-1) {
                std::memset(std::addressof(this->data), 0, sizeof(this->data));
            }

            Result Initialize(const NcaReader &reader, s32 index);
            bool IsInitialized() const { return this->fs_index >= 0; }

            NcaFsHeader &GetData() { return this->data; }
            const NcaFsHeader &GetData() const { return this->data; }
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
            bool ExistsSparseLayer() const;
            NcaSparseInfo &GetSparseInfo();
            const NcaSparseInfo &GetSparseInfo() const;
    };

    class NcaFileSystemDriver : public ::ams::fs::impl::Newable {
        NON_COPYABLE(NcaFileSystemDriver);
        NON_MOVEABLE(NcaFileSystemDriver);
        public:
            class StorageOption;
            class StorageOptionWithHeaderReader;
        private:
            std::shared_ptr<NcaReader> original_reader;
            std::shared_ptr<NcaReader> reader;
            MemoryResource * const allocator;
            fssystem::IBufferManager * const buffer_manager;
        public:
            static Result SetupFsHeaderReader(NcaFsHeaderReader *out, const NcaReader &reader, s32 fs_index);
        public:
            NcaFileSystemDriver(std::shared_ptr<NcaReader> reader, MemoryResource *allocator, IBufferManager *buffer_manager) : original_reader(), reader(reader), allocator(allocator), buffer_manager(buffer_manager) {
                AMS_ASSERT(this->reader != nullptr);
            }

            NcaFileSystemDriver(std::shared_ptr<NcaReader> original_reader, std::shared_ptr<NcaReader> reader, MemoryResource *allocator, IBufferManager *buffer_manager) : original_reader(original_reader), reader(reader), allocator(allocator), buffer_manager(buffer_manager) {
                AMS_ASSERT(this->reader != nullptr);
            }

            Result OpenRawStorage(std::shared_ptr<fs::IStorage> *out, s32 fs_index);

            Result OpenStorage(std::shared_ptr<fs::IStorage> *out, NcaFsHeaderReader *out_header_reader, s32 fs_index);
            Result OpenStorage(std::shared_ptr<fs::IStorage> *out, StorageOption *option);

            Result OpenStorage(std::shared_ptr<fs::IStorage> *out, s32 fs_index) {
                NcaFsHeaderReader dummy;
                return this->OpenStorage(out, std::addressof(dummy), fs_index);
            }

            Result OpenDecryptableStorage(std::shared_ptr<fs::IStorage> *out, StorageOption *option, bool indirect_needed);

        private:
            class BaseStorage;

            Result CreateBaseStorage(BaseStorage *out, StorageOption *option);

            Result CreateDecryptableStorage(std::unique_ptr<fs::IStorage> *out, StorageOption *option, BaseStorage *base_storage);
            Result CreateAesXtsStorage(std::unique_ptr<fs::IStorage> *out, BaseStorage *base_storage);
            Result CreateAesCtrStorage(std::unique_ptr<fs::IStorage> *out, BaseStorage *base_storage);
            Result CreateAesCtrExStorage(std::unique_ptr<fs::IStorage> *out, StorageOption *option, BaseStorage *base_storage);

            Result CreateIndirectStorage(std::unique_ptr<fs::IStorage> *out, StorageOption *option, std::unique_ptr<fs::IStorage> base_storage);

            Result CreateVerificationStorage(std::unique_ptr<fs::IStorage> *out, std::unique_ptr<fs::IStorage> base_storage, NcaFsHeaderReader *header_reader);
            Result CreateSha256Storage(std::unique_ptr<fs::IStorage> *out, std::unique_ptr<fs::IStorage> base_storage, NcaFsHeaderReader *header_reader);
            Result CreateIntegrityVerificationStorage(std::unique_ptr<fs::IStorage> *out, std::unique_ptr<fs::IStorage> base_storage, NcaFsHeaderReader *header_reader);
    };

}
