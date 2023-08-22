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
#include "fssystem_read_only_block_cache_storage.hpp"
#include "fssystem_hierarchical_sha256_storage.hpp"
#include "fssystem_memory_resource_buffer_hold_storage.hpp"

namespace ams::fssystem {

    namespace {

        constexpr inline s32 AesCtrExTableCacheBlockSize = AesCtrCounterExtendedStorage::NodeSize;
        constexpr inline s32 AesCtrExTableCacheCount     = 8;
        constexpr inline s32 IndirectTableCacheBlockSize = IndirectStorage::NodeSize;
        constexpr inline s32 IndirectTableCacheCount     = 8;
        constexpr inline s32 IndirectDataCacheBlockSize  = 32_KB;
        constexpr inline s32 IndirectDataCacheCount      = 16;
        constexpr inline s32 SparseTableCacheBlockSize   = SparseStorage::NodeSize;
        constexpr inline s32 SparseTableCacheCount       = 4;

        constexpr inline s32 IntegrityDataCacheCount = 24;
        constexpr inline s32 IntegrityHashCacheCount = 8;

        constexpr inline s32 IntegrityDataCacheCountForMeta = 16;
        constexpr inline s32 IntegrityHashCacheCountForMeta = 2;

        //TODO: Better names for these?
        //constexpr inline s32 CompressedDataBlockSize            = 64_KB;
        //constexpr inline s32 CompressedContinuousReadingSizeMax = 640_KB;
        //constexpr inline s32 CompressedCacheBlockSize           = 16_KB;
        //constexpr inline s32 CompressedCacheCount               = 32;

        constexpr inline s32 AesCtrStorageCacheBlockSize = 0x200;
        constexpr inline s32 AesCtrStorageCacheCount     = 9;

        class SharedNcaBodyStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
            NON_COPYABLE(SharedNcaBodyStorage);
            NON_MOVEABLE(SharedNcaBodyStorage);
            private:
                std::shared_ptr<fs::IStorage> m_storage;
                std::shared_ptr<fssystem::NcaReader> m_nca_reader;
            public:
                SharedNcaBodyStorage(std::shared_ptr<fs::IStorage> s, std::shared_ptr<fssystem::NcaReader> r) : m_storage(std::move(s)), m_nca_reader(std::move(r)) {
                    /* ... */
                }

                virtual Result Read(s64 offset, void *buffer, size_t size) override {
                    /* Validate pre-conditions. */
                    AMS_ASSERT(m_storage != nullptr);

                    /* Read from the base storage. */
                    R_RETURN(m_storage->Read(offset, buffer, size));
                }

                virtual Result GetSize(s64 *out) override {
                    /* Validate pre-conditions. */
                    AMS_ASSERT(m_storage != nullptr);

                    R_RETURN(m_storage->GetSize(out));
                }

                virtual Result Flush() override {
                    /* Validate pre-conditions. */
                    AMS_ASSERT(m_storage != nullptr);

                    R_RETURN(m_storage->Flush());
                }

                virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                    /* Validate pre-conditions. */
                    AMS_ASSERT(m_storage != nullptr);

                    /* Read from the base storage. */
                    R_RETURN(m_storage->Write(offset, buffer, size));
                }

                virtual Result SetSize(s64 size) override {
                    /* Validate pre-conditions. */
                    AMS_ASSERT(m_storage != nullptr);

                    R_RETURN(m_storage->SetSize(size));
                }

                virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                    /* Validate pre-conditions. */
                    AMS_ASSERT(m_storage != nullptr);

                    R_RETURN(m_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                }
        };

        inline s64 GetFsOffset(const NcaReader &reader, s32 fs_index) {
            return static_cast<s64>(reader.GetFsOffset(fs_index));
        }

        inline s64 GetFsEndOffset(const NcaReader &reader, s32 fs_index) {
            return static_cast<s64>(reader.GetFsEndOffset(fs_index));
        }

        inline bool IsUsingHwAesCtrForSpeedEmulation() {
            auto mode = fssystem::SpeedEmulationConfiguration::GetSpeedEmulationMode();
            return mode == fs::SpeedEmulationMode::None || mode == fs::SpeedEmulationMode::Slower;
        }

        using Sha256DataRegion   = NcaFsHeader::Region;
        using IntegrityLevelInfo = NcaFsHeader::HashData::IntegrityMetaInfo::LevelHashInfo;
        using IntegrityDataInfo  = IntegrityLevelInfo::HierarchicalIntegrityVerificationLevelInformation;

    }

    Result NcaFileSystemDriver::OpenStorageWithContext(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<IAsynchronousAccessSplitter> *out_splitter, NcaFsHeaderReader *out_header_reader, s32 fs_index, StorageContext *ctx) {
        /* Open storage. */
        R_TRY(this->OpenStorageImpl(out, out_header_reader, fs_index, ctx));

        /* If we have a compressed storage, use it as splitter. */
        if (ctx->compressed_storage != nullptr) {
            *out_splitter = ctx->compressed_storage;
        } else {
            /* Otherwise, allocate a default splitter. */
            *out_splitter = fssystem::AllocateShared<DefaultAsynchronousAccessSplitter>();
            R_UNLESS(*out_splitter != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());
        }

        R_SUCCEED();
    }

    Result NcaFileSystemDriver::OpenStorageImpl(std::shared_ptr<fs::IStorage> *out, NcaFsHeaderReader *out_header_reader, s32 fs_index, StorageContext *ctx) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(out_header_reader != nullptr);
        AMS_ASSERT(0 <= fs_index && fs_index < NcaHeader::FsCountMax);

        /* Validate the fs index. */
        R_UNLESS(m_reader->HasFsInfo(fs_index), fs::ResultPartitionNotFound());

        /* Initialize our header reader for the fs index. */
        R_TRY(out_header_reader->Initialize(*m_reader, fs_index));

        /* Declare the storage we're opening. */
        std::shared_ptr<fs::IStorage> storage;

        /* Process sparse layer. */
        s64 fs_data_offset = 0;
        if (out_header_reader->ExistsSparseLayer()) {
            /* Get the sparse info. */
            const auto &sparse_info = out_header_reader->GetSparseInfo();

            /* Create based on whether we have a meta hash layer. */
            if (out_header_reader->ExistsSparseMetaHashLayer()) {
                /* Create the sparse storage with verification. */
                R_TRY(this->CreateSparseStorageWithVerification(std::addressof(storage), std::addressof(fs_data_offset), ctx != nullptr ? std::addressof(ctx->current_sparse_storage) : nullptr, ctx != nullptr ? std::addressof(ctx->sparse_storage_meta_storage) : nullptr, ctx != nullptr ? std::addressof(ctx->sparse_layer_info_storage) : nullptr, fs_index, out_header_reader->GetAesCtrUpperIv(), sparse_info, out_header_reader->GetSparseMetaDataHashDataInfo(), out_header_reader->GetSparseMetaHashType()));
            } else {
                /* Create the sparse storage. */
                R_TRY(this->CreateSparseStorage(std::addressof(storage), std::addressof(fs_data_offset), ctx != nullptr ? std::addressof(ctx->current_sparse_storage) : nullptr, ctx != nullptr ? std::addressof(ctx->sparse_storage_meta_storage) : nullptr, fs_index, out_header_reader->GetAesCtrUpperIv(), sparse_info));
            }
        } else {
            /* Get the data offsets. */
            fs_data_offset           = GetFsOffset(*m_reader, fs_index);
            const auto fs_end_offset = GetFsEndOffset(*m_reader, fs_index);

            /* Validate that we're within range. */
            const auto data_size = fs_end_offset - fs_data_offset;
            R_UNLESS(data_size > 0, fs::ResultInvalidNcaHeader());

            /* Create the body substorage. */
            R_TRY(this->CreateBodySubStorage(std::addressof(storage), fs_data_offset, data_size));

            /* Potentially save the body substorage to our context. */
            if (ctx != nullptr) {
                ctx->body_substorage = storage;
            }
        }

        /* Process patch layer. */
        const auto &patch_info = out_header_reader->GetPatchInfo();
        std::shared_ptr<fs::IStorage> patch_meta_aes_ctr_ex_meta_storage;
        std::shared_ptr<fs::IStorage> patch_meta_indirect_meta_storage;
        if (out_header_reader->ExistsPatchMetaHashLayer()) {
            /* Check the meta hash type. */
            R_UNLESS(out_header_reader->GetPatchMetaHashType() == NcaFsHeader::MetaDataHashType::HierarchicalIntegrity, fs::ResultRomNcaInvalidPatchMetaDataHashType());

            /* Create the patch meta storage. */
            R_TRY(this->CreatePatchMetaStorage(std::addressof(patch_meta_aes_ctr_ex_meta_storage), std::addressof(patch_meta_indirect_meta_storage), ctx != nullptr ? std::addressof(ctx->patch_layer_info_storage) : nullptr, storage, fs_data_offset, out_header_reader->GetAesCtrUpperIv(), patch_info, out_header_reader->GetPatchMetaDataHashDataInfo(), m_hash_generator_factory_selector->GetFactory(fssystem::HashAlgorithmType_Sha2)));
        }

        if (patch_info.HasAesCtrExTable()) {
            /* Check the encryption type. */
            AMS_ASSERT(out_header_reader->GetEncryptionType() == NcaFsHeader::EncryptionType::None || out_header_reader->GetEncryptionType() == NcaFsHeader::EncryptionType::AesCtrEx || out_header_reader->GetEncryptionType() == NcaFsHeader::EncryptionType::AesCtrExSkipLayerHash);

            /* Create the ex meta storage. */
            std::shared_ptr<fs::IStorage> aes_ctr_ex_storage_meta_storage = patch_meta_aes_ctr_ex_meta_storage;
            if (aes_ctr_ex_storage_meta_storage == nullptr) {
                /* If we don't have a meta storage, we must not have a patch meta hash layer. */
                AMS_ASSERT(!out_header_reader->ExistsPatchMetaHashLayer());

                R_TRY(this->CreateAesCtrExStorageMetaStorage(std::addressof(aes_ctr_ex_storage_meta_storage), storage, fs_data_offset, out_header_reader->GetEncryptionType(), out_header_reader->GetAesCtrUpperIv(), patch_info));
            }

            /* Create the ex storage. */
            std::shared_ptr<fs::IStorage> aes_ctr_ex_storage;
            R_TRY(this->CreateAesCtrExStorage(std::addressof(aes_ctr_ex_storage), ctx != nullptr ? std::addressof(ctx->aes_ctr_ex_storage) : nullptr, std::move(storage), aes_ctr_ex_storage_meta_storage, fs_data_offset, out_header_reader->GetAesCtrUpperIv(), patch_info));

            /* Set the base storage as the ex storage. */
            storage = std::move(aes_ctr_ex_storage);

            /* Potentially save storages to our context. */
            if (ctx != nullptr) {
                ctx->aes_ctr_ex_storage_meta_storage = aes_ctr_ex_storage_meta_storage;
                ctx->aes_ctr_ex_storage_data_storage = storage;
                ctx->fs_data_storage                 = storage;
            }
        } else {
            /* Create the appropriate storage for the encryption type. */
            switch (out_header_reader->GetEncryptionType()) {
                case NcaFsHeader::EncryptionType::None:
                    /* If there's no encryption, use the base storage we made previously. */
                    break;
                case NcaFsHeader::EncryptionType::AesXts:
                    R_TRY(this->CreateAesXtsStorage(std::addressof(storage), std::move(storage), fs_data_offset));
                    break;
                case NcaFsHeader::EncryptionType::AesCtr:
                    R_TRY(this->CreateAesCtrStorage(std::addressof(storage), std::move(storage), fs_data_offset, out_header_reader->GetAesCtrUpperIv(), AlignmentStorageRequirement_None));
                    break;
                case NcaFsHeader::EncryptionType::AesCtrSkipLayerHash:
                    {
                        /* Create the aes ctr storage. */
                        std::shared_ptr<fs::IStorage> aes_ctr_storage;
                        R_TRY(this->CreateAesCtrStorage(std::addressof(aes_ctr_storage), storage, fs_data_offset, out_header_reader->GetAesCtrUpperIv(), AlignmentStorageRequirement_None));

                        /* Create region switch storage. */
                        R_TRY(this->CreateRegionSwitchStorage(std::addressof(storage), out_header_reader, std::move(storage), std::move(aes_ctr_storage)));
                    }
                    break;
                default:
                    R_THROW(fs::ResultInvalidNcaFsHeaderEncryptionType());
            }

            /* Potentially save storages to our context. */
            if (ctx != nullptr) {
                ctx->fs_data_storage = storage;
            }
        }

        /* Process indirect layer. */
        if (patch_info.HasIndirectTable()) {
            /* Create the indirect meta storage. */
            std::shared_ptr<fs::IStorage> indirect_storage_meta_storage = patch_meta_indirect_meta_storage;
            if (indirect_storage_meta_storage == nullptr) {
                /* If we don't have a meta storage, we must not have a patch meta hash layer. */
                AMS_ASSERT(!out_header_reader->ExistsPatchMetaHashLayer());

                R_TRY(this->CreateIndirectStorageMetaStorage(std::addressof(indirect_storage_meta_storage), storage, patch_info));
            }

            /* Potentially save the indirect meta storage to our context. */
            if (ctx != nullptr) {
                ctx->indirect_storage_meta_storage = indirect_storage_meta_storage;
            }

            /* Get the original indirectable storage. */
            std::shared_ptr<fs::IStorage> original_indirectable_storage;
            if (m_original_reader != nullptr && m_original_reader->HasFsInfo(fs_index)) {
                /* Create a driver for the original. */
                NcaFileSystemDriver original_driver(m_original_reader, m_allocator, m_buffer_manager, m_hash_generator_factory_selector);

                /* Create a header reader for the original. */
                NcaFsHeaderReader original_header_reader;
                R_TRY(original_header_reader.Initialize(*m_original_reader, fs_index));

                /* Open original indirectable storage. */
                R_TRY(original_driver.OpenIndirectableStorageAsOriginal(std::addressof(original_indirectable_storage), std::addressof(original_header_reader), ctx));
            } else if (ctx != nullptr && ctx->external_original_storage != nullptr) {
                /* Use the external original storage. */
                original_indirectable_storage = ctx->external_original_storage;
            } else {
                /* Allocate a dummy memory storage as original storage. */
                original_indirectable_storage = fssystem::AllocateShared<fs::MemoryStorage>(nullptr, 0);
                R_UNLESS(original_indirectable_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());
            }

            /* Create the indirect storage. */
            std::shared_ptr<fs::IStorage> indirect_storage;
            R_TRY(this->CreateIndirectStorage(std::addressof(indirect_storage), ctx != nullptr ? std::addressof(ctx->indirect_storage) : nullptr, std::move(storage), std::move(original_indirectable_storage), std::move(indirect_storage_meta_storage), patch_info));

            /* Set storage as the indirect storage. */
            storage = std::move(indirect_storage);
        }

        /* Check if we're sparse or requested to skip the integrity layer. */
        if (out_header_reader->ExistsSparseLayer() || (ctx != nullptr && ctx->open_raw_storage)) {
            *out = std::move(storage);
            R_SUCCEED();
        }

        /* Create the non-raw storage. */
        R_RETURN(this->CreateStorageByRawStorage(out, out_header_reader, std::move(storage), ctx));
    }

    Result NcaFileSystemDriver::CreateStorageByRawStorage(std::shared_ptr<fs::IStorage> *out, const NcaFsHeaderReader *header_reader, std::shared_ptr<fs::IStorage> raw_storage, StorageContext *ctx) {
        /* Initialize storage as raw storage. */
        std::shared_ptr<fs::IStorage> storage = std::move(raw_storage);

        /* Process hash/integrity layer. */
        switch (header_reader->GetHashType()) {
            case NcaFsHeader::HashType::HierarchicalSha256Hash:
                R_TRY(this->CreateSha256Storage(std::addressof(storage), std::move(storage), header_reader->GetHashData().hierarchical_sha256_data, m_hash_generator_factory_selector->GetFactory(fssystem::HashAlgorithmType_Sha2)));
                break;
            case NcaFsHeader::HashType::HierarchicalIntegrityHash:
                R_TRY(this->CreateIntegrityVerificationStorage(std::addressof(storage), std::move(storage), header_reader->GetHashData().integrity_meta_info, m_hash_generator_factory_selector->GetFactory(fssystem::HashAlgorithmType_Sha2)));
                break;
            default:
                R_THROW(fs::ResultInvalidNcaFsHeaderHashType());
        }

        /* Process compression layer. */
        if (header_reader->ExistsCompressionLayer()) {
            R_TRY(this->CreateCompressedStorage(std::addressof(storage), ctx != nullptr ? std::addressof(ctx->compressed_storage) : nullptr, ctx != nullptr ? std::addressof(ctx->compressed_storage_meta_storage) : nullptr, std::move(storage), header_reader->GetCompressionInfo()));
        }

        /* Set output storage. */
        *out = std::move(storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::OpenIndirectableStorageAsOriginal(std::shared_ptr<fs::IStorage> *out, const NcaFsHeaderReader *header_reader, StorageContext *ctx) {
        /* Get the fs index. */
        const auto fs_index = header_reader->GetFsIndex();

        /* Declare the storage we're opening. */
        std::shared_ptr<fs::IStorage> storage;

        /* Process sparse layer. */
        s64 fs_data_offset = 0;
        if (header_reader->ExistsSparseLayer()) {
            /* Get the sparse info. */
            const auto &sparse_info = header_reader->GetSparseInfo();

            /* Create based on whether we have a meta hash layer. */
            if (header_reader->ExistsSparseMetaHashLayer()) {
                /* Create the sparse storage with verification. */
                R_TRY(this->CreateSparseStorageWithVerification(std::addressof(storage), std::addressof(fs_data_offset), ctx != nullptr ? std::addressof(ctx->original_sparse_storage) : nullptr, ctx != nullptr ? std::addressof(ctx->sparse_storage_meta_storage) : nullptr, ctx != nullptr ? std::addressof(ctx->sparse_layer_info_storage) : nullptr, fs_index, header_reader->GetAesCtrUpperIv(), sparse_info, header_reader->GetSparseMetaDataHashDataInfo(), header_reader->GetSparseMetaHashType()));
            } else {
                /* Create the sparse storage. */
                R_TRY(this->CreateSparseStorage(std::addressof(storage), std::addressof(fs_data_offset), ctx != nullptr ? std::addressof(ctx->original_sparse_storage) : nullptr, ctx != nullptr ? std::addressof(ctx->sparse_storage_meta_storage) : nullptr, fs_index, header_reader->GetAesCtrUpperIv(), sparse_info));
            }
        } else {
            /* Get the data offsets. */
            fs_data_offset           = GetFsOffset(*m_reader, fs_index);
            const auto fs_end_offset = GetFsEndOffset(*m_reader, fs_index);

            /* Validate that we're within range. */
            const auto data_size = fs_end_offset - fs_data_offset;
            R_UNLESS(data_size > 0, fs::ResultInvalidNcaHeader());

            /* Create the body substorage. */
            R_TRY(this->CreateBodySubStorage(std::addressof(storage), fs_data_offset, data_size));
        }

        /* Create the appropriate storage for the encryption type. */
        switch (header_reader->GetEncryptionType()) {
            case NcaFsHeader::EncryptionType::None:
                /* If there's no encryption, use the base storage we made previously. */
                break;
            case NcaFsHeader::EncryptionType::AesXts:
                R_TRY(this->CreateAesXtsStorage(std::addressof(storage), std::move(storage), fs_data_offset));
                break;
            case NcaFsHeader::EncryptionType::AesCtr:
                R_TRY(this->CreateAesCtrStorage(std::addressof(storage), std::move(storage), fs_data_offset, header_reader->GetAesCtrUpperIv(), AlignmentStorageRequirement_CacheBlockSize));
                break;
            default:
                R_THROW(fs::ResultInvalidNcaFsHeaderEncryptionType());
        }

        /* Set output storage. */
        *out = std::move(storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateBodySubStorage(std::shared_ptr<fs::IStorage> *out, s64 offset, s64 size) {
        /* Create the body storage. */
        auto body_storage = fssystem::AllocateShared<SharedNcaBodyStorage>(m_reader->GetSharedBodyStorage(), m_reader);
        R_UNLESS(body_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Get the body storage size. */
        s64 body_size = 0;
        R_TRY(body_storage->GetSize(std::addressof(body_size)));

        /* Check that we're within range. */
        R_UNLESS(offset + size <= body_size, fs::ResultNcaBaseStorageOutOfRangeB());

        /* Create substorage. */
        auto body_substorage = fssystem::AllocateShared<fs::SubStorage>(std::move(body_storage), offset, size);
        R_UNLESS(body_substorage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Set the output storage. */
        *out = std::move(body_substorage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateAesCtrStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, s64 offset, const NcaAesCtrUpperIv &upper_iv, AlignmentStorageRequirement alignment_storage_requirement) {
        /* Check pre-conditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);

        /* Enforce alignment of accesses to base storage. */
        switch (alignment_storage_requirement) {
            case AlignmentStorageRequirement_CacheBlockSize:
                {
                    /* Get the base storage's size. */
                    s64 base_size;
                    R_TRY(base_storage->GetSize(std::addressof(base_size)));

                    /* Create buffered storage. */
                    auto buffered_storage = fssystem::AllocateShared<BufferedStorage>();
                    R_UNLESS(buffered_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

                    /* Initialize the buffered storage. */
                    R_TRY(buffered_storage->Initialize(fs::SubStorage(std::move(base_storage), 0, base_size), m_buffer_manager, AesCtrStorageCacheBlockSize, AesCtrStorageCacheCount));

                    /* Enable bulk read in the buffered storage. */
                    buffered_storage->EnableBulkRead();

                    /* Use the buffered storage in place of our base storage. */
                    base_storage = std::move(buffered_storage);
                }
                break;
            case AlignmentStorageRequirement_None:
            default:
                /* No alignment enforcing is required. */
                break;
        }

        /* Create the iv. */
        u8 iv[AesCtrStorageBySharedPointer::IvSize] = {};
        AesCtrStorageBySharedPointer::MakeIv(iv, sizeof(iv), upper_iv.value, offset);

        /* Create the ctr storage. */
        std::shared_ptr<fs::IStorage> aes_ctr_storage;
        if (m_reader->HasExternalDecryptionKey()) {
            aes_ctr_storage = fssystem::AllocateShared<AesCtrStorageExternal>(std::move(base_storage), m_reader->GetExternalDecryptionKey(), AesCtrStorageExternal::KeySize, iv, AesCtrStorageExternal::IvSize, m_reader->GetExternalDecryptAesCtrFunctionForExternalKey(), -1, -1);
            R_UNLESS(aes_ctr_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());
        } else {
            /* Create software decryption storage. */
            auto sw_storage = fssystem::AllocateShared<AesCtrStorageBySharedPointer>(base_storage, m_reader->GetDecryptionKey(NcaHeader::DecryptionKey_AesCtr), AesCtrStorageBySharedPointer::KeySize, iv, AesCtrStorageBySharedPointer::IvSize);
            R_UNLESS(sw_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

            /* If we have a hardware key and should use it, make the hardware decryption storage. */
            if (m_reader->HasInternalDecryptionKeyForAesHw() && !m_reader->IsSoftwareAesPrioritized()) {
                auto hw_storage = fssystem::AllocateShared<AesCtrStorageExternal>(base_storage, m_reader->GetDecryptionKey(NcaHeader::DecryptionKey_AesCtrHw), AesCtrStorageExternal::KeySize, iv, AesCtrStorageExternal::IvSize, m_reader->GetExternalDecryptAesCtrFunction(), m_reader->GetKeyIndex(), m_reader->GetKeyGeneration());
                R_UNLESS(hw_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

                /* Create the selection storage. */
                auto switch_storage = fssystem::AllocateShared<SwitchStorage<bool (*)()>>(std::move(hw_storage), std::move(sw_storage), IsUsingHwAesCtrForSpeedEmulation);
                R_UNLESS(switch_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

                /* Use the selection storage. */
                aes_ctr_storage = std::move(switch_storage);
            } else {
                /* Otherwise, just use the software decryption storage. */
                aes_ctr_storage = std::move(sw_storage);
            }
        }

        /* Create alignment matching storage. */
        auto aligned_storage = fssystem::AllocateShared<AlignmentMatchingStorage<NcaHeader::CtrBlockSize, 1>>(std::move(aes_ctr_storage));
        R_UNLESS(aligned_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Set the out storage. */
        *out = std::move(aligned_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateAesXtsStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, s64 offset) {
        /* Check pre-conditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);

        /* Create the iv. */
        u8 iv[AesXtsStorageBySharedPointer::IvSize] = {};
        AesXtsStorageBySharedPointer::MakeAesXtsIv(iv, sizeof(iv), offset, NcaHeader::XtsBlockSize);

        /* Make the aes xts storage. */
        const auto * const key1 = m_reader->GetDecryptionKey(NcaHeader::DecryptionKey_AesXts1);
        const auto * const key2 = m_reader->GetDecryptionKey(NcaHeader::DecryptionKey_AesXts2);
        auto xts_storage = fssystem::AllocateShared<AesXtsStorageBySharedPointer>(std::move(base_storage), key1, key2, AesXtsStorageBySharedPointer::KeySize, iv, AesXtsStorageBySharedPointer::IvSize, NcaHeader::XtsBlockSize);
        R_UNLESS(xts_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Create alignment matching storage. */
        auto aligned_storage = fssystem::AllocateShared<AlignmentMatchingStorage<NcaHeader::XtsBlockSize, 1>>(std::move(xts_storage));
        R_UNLESS(aligned_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Set the out storage. */
        *out = std::move(aligned_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateSparseStorageMetaStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, s64 offset, const NcaAesCtrUpperIv &upper_iv, const NcaSparseInfo &sparse_info) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);

        /* Get the base storage size. */
        s64 base_size = 0;
        R_TRY(base_storage->GetSize(std::addressof(base_size)));

        /* Get the meta extents. */
        const auto meta_offset = sparse_info.bucket.offset;
        const auto meta_size   = sparse_info.bucket.size;
        R_UNLESS(meta_offset + meta_size - offset <= base_size, fs::ResultNcaBaseStorageOutOfRangeB());

        /* Create the encrypted storage. */
        auto enc_storage = fssystem::AllocateShared<fs::SubStorage>(std::move(base_storage), meta_offset, meta_size);
        R_UNLESS(enc_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Create the decrypted storage. */
        std::shared_ptr<fs::IStorage> decrypted_storage;
        R_TRY(this->CreateAesCtrStorage(std::addressof(decrypted_storage), std::move(enc_storage), offset + meta_offset, sparse_info.MakeAesCtrUpperIv(upper_iv), AlignmentStorageRequirement_None));

        /* Create meta storage. */
        auto meta_storage = fssystem::AllocateShared<BufferedStorage>();
        R_UNLESS(meta_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Initialize the meta storage. */
        R_TRY(meta_storage->Initialize(fs::SubStorage(std::move(decrypted_storage), 0, meta_size), m_buffer_manager, SparseTableCacheBlockSize, SparseTableCacheCount));

        /* Set the output. */
        *out = std::move(meta_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateSparseStorageCore(std::shared_ptr<fssystem::SparseStorage> *out, std::shared_ptr<fs::IStorage> base_storage, s64 base_size, std::shared_ptr<fs::IStorage> meta_storage, const NcaSparseInfo &sparse_info, bool external_info) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(meta_storage != nullptr);

        /* Read and verify the bucket tree header. */
        BucketTree::Header header;
        std::memcpy(std::addressof(header), sparse_info.bucket.header, sizeof(header));
        R_TRY(header.Verify());

        /* Determine storage extents. */
        const auto node_offset  = 0;
        const auto node_size    = SparseStorage::QueryNodeStorageSize(header.entry_count);
        const auto entry_offset = node_offset + node_size;
        const auto entry_size   = SparseStorage::QueryEntryStorageSize(header.entry_count);

        /* Create the sparse storage. */
        auto sparse_storage = fssystem::AllocateShared<fssystem::SparseStorage>();
        R_UNLESS(sparse_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Sanity check that we can be doing this. */
        AMS_ASSERT(header.entry_count != 0);

        /* Initialize the sparse storage. */
        R_TRY(sparse_storage->Initialize(m_allocator, fs::SubStorage(meta_storage, node_offset, node_size), fs::SubStorage(meta_storage, entry_offset, entry_size), header.entry_count));

        /* If not external, set the data storage. */
        if (!external_info) {
            sparse_storage->SetDataStorage(fs::SubStorage(std::move(base_storage), 0, base_size));
        }

        /* Set the output. */
        *out = std::move(sparse_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateSparseStorage(std::shared_ptr<fs::IStorage> *out, s64 *out_fs_data_offset, std::shared_ptr<fssystem::SparseStorage> *out_sparse_storage, std::shared_ptr<fs::IStorage> *out_meta_storage, s32 index, const NcaAesCtrUpperIv &upper_iv, const NcaSparseInfo &sparse_info) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(out_fs_data_offset != nullptr);

        /* Check the sparse info generation. */
        R_UNLESS(sparse_info.generation != 0, fs::ResultInvalidNcaHeader());

        /* Read and verify the bucket tree header. */
        BucketTree::Header header;
        std::memcpy(std::addressof(header), sparse_info.bucket.header, sizeof(header));
        R_TRY(header.Verify());

        /* Determine the storage extents. */
        const auto fs_offset     = GetFsOffset(*m_reader, index);
        const auto fs_end_offset = GetFsEndOffset(*m_reader, index);
        const auto fs_size       = fs_end_offset - fs_offset;

        /* Create the sparse storage. */
        std::shared_ptr<fssystem::SparseStorage> sparse_storage;
        if (header.entry_count != 0) {
            /* Create the body substorage. */
            std::shared_ptr<fs::IStorage> body_substorage;
            R_TRY(this->CreateBodySubStorage(std::addressof(body_substorage), sparse_info.physical_offset, sparse_info.GetPhysicalSize()));

            /* Create the meta storage. */
            std::shared_ptr<fs::IStorage> meta_storage;
            R_TRY(this->CreateSparseStorageMetaStorage(std::addressof(meta_storage), body_substorage, sparse_info.physical_offset, upper_iv, sparse_info));

            /* Potentially set the output meta storage. */
            if (out_meta_storage != nullptr) {
                *out_meta_storage = meta_storage;
            }

            /* Create the sparse storage. */
            R_TRY(this->CreateSparseStorageCore(std::addressof(sparse_storage), body_substorage, sparse_info.GetPhysicalSize(), std::move(meta_storage), sparse_info, false));
        } else {
            /* If there are no entries, there's nothing to actually do. */
            sparse_storage = fssystem::AllocateShared<fssystem::SparseStorage>();
            R_UNLESS(sparse_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

            sparse_storage->Initialize(fs_size);
        }

        /* Potentially set the output sparse storage. */
        if (out_sparse_storage != nullptr) {
            *out_sparse_storage = sparse_storage;
        }

        /* Set the output fs data offset. */
        *out_fs_data_offset = fs_offset;

        /* Set the output storage. */
        *out = std::move(sparse_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateSparseStorageMetaStorageWithVerification(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> *out_layer_info_storage, std::shared_ptr<fs::IStorage> base_storage, s64 offset, const NcaAesCtrUpperIv &upper_iv, const NcaSparseInfo &sparse_info, const NcaMetaDataHashDataInfo &meta_data_hash_data_info, IHash256GeneratorFactory *hgf) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(hgf != nullptr);

        /* Get the base storage size. */
        s64 base_size = 0;
        R_TRY(base_storage->GetSize(std::addressof(base_size)));

        /* Get the meta extents. */
        const auto meta_offset = sparse_info.bucket.offset;
        const auto meta_size   = sparse_info.bucket.size;
        R_UNLESS(meta_offset + meta_size - offset <= base_size, fs::ResultNcaBaseStorageOutOfRangeB());

        /* Get the meta data hash data extents. */
        const s64 meta_data_hash_data_offset = meta_data_hash_data_info.offset;
        const s64 meta_data_hash_data_size   = util::AlignUp<s64>(meta_data_hash_data_info.size, NcaHeader::CtrBlockSize);
        R_UNLESS(meta_data_hash_data_offset + meta_data_hash_data_size <= base_size, fs::ResultNcaBaseStorageOutOfRangeB());

        /* Check that the meta is before the hash data. */
        R_UNLESS(meta_offset + meta_size <= meta_data_hash_data_offset, fs::ResultRomNcaInvalidSparseMetaDataHashDataOffset());

        /* Check that offsets are appropriately aligned. */
        R_UNLESS(util::IsAligned<s64>(meta_data_hash_data_offset, NcaHeader::CtrBlockSize), fs::ResultRomNcaInvalidSparseMetaDataHashDataOffset());
        R_UNLESS(util::IsAligned<s64>(meta_offset, NcaHeader::CtrBlockSize),                fs::ResultInvalidNcaFsHeader());

        /* Create the meta storage. */
        auto enc_storage = fssystem::AllocateShared<fs::SubStorage>(std::move(base_storage), meta_offset, meta_data_hash_data_offset + meta_data_hash_data_size - meta_offset);
        R_UNLESS(enc_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Create the decrypted storage. */
        std::shared_ptr<fs::IStorage> decrypted_storage;
        R_TRY(this->CreateAesCtrStorage(std::addressof(decrypted_storage), std::move(enc_storage), offset + meta_offset, sparse_info.MakeAesCtrUpperIv(upper_iv), AlignmentStorageRequirement_None));

        /* Create the verification storage. */
        std::shared_ptr<fs::IStorage> integrity_storage;
        R_TRY_CATCH(this->CreateIntegrityVerificationStorageForMeta(std::addressof(integrity_storage), out_layer_info_storage, std::move(decrypted_storage), meta_offset, meta_data_hash_data_info, hgf)) {
            R_CONVERT(fs::ResultInvalidNcaMetaDataHashDataSize, fs::ResultRomNcaInvalidSparseMetaDataHashDataSize())
            R_CONVERT(fs::ResultInvalidNcaMetaDataHashDataHash, fs::ResultRomNcaInvalidSparseMetaDataHashDataHash())
        } R_END_TRY_CATCH;

        /* Create the meta storage. */
        auto meta_storage = fssystem::AllocateShared<fs::SubStorage>(std::move(integrity_storage), 0, meta_size);
        R_UNLESS(meta_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Set the output. */
        *out = std::move(meta_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateSparseStorageWithVerification(std::shared_ptr<fs::IStorage> *out, s64 *out_fs_data_offset, std::shared_ptr<fssystem::SparseStorage> *out_sparse_storage, std::shared_ptr<fs::IStorage> *out_meta_storage, std::shared_ptr<fs::IStorage> *out_layer_info_storage, s32 index, const NcaAesCtrUpperIv &upper_iv, const NcaSparseInfo &sparse_info, const NcaMetaDataHashDataInfo &meta_data_hash_data_info, NcaFsHeader::MetaDataHashType meta_data_hash_type) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(out_fs_data_offset != nullptr);

        /* Check the sparse info generation. */
        R_UNLESS(sparse_info.generation != 0, fs::ResultInvalidNcaHeader());

        /* Read and verify the bucket tree header. */
        BucketTree::Header header;
        std::memcpy(std::addressof(header), sparse_info.bucket.header, sizeof(header));
        R_TRY(header.Verify());

        /* Determine the storage extents. */
        const auto fs_offset     = GetFsOffset(*m_reader, index);
        const auto fs_end_offset = GetFsEndOffset(*m_reader, index);
        const auto fs_size       = fs_end_offset - fs_offset;

        /* Create the sparse storage. */
        std::shared_ptr<fssystem::SparseStorage> sparse_storage;
        if (header.entry_count != 0) {
            /* Create the body substorage. */
            std::shared_ptr<fs::IStorage> body_substorage;
            R_TRY(this->CreateBodySubStorage(std::addressof(body_substorage), sparse_info.physical_offset, util::AlignUp<s64>(static_cast<s64>(meta_data_hash_data_info.offset) + static_cast<s64>(meta_data_hash_data_info.size), NcaHeader::CtrBlockSize)));

            /* Check the meta data hash type. */
            R_UNLESS(meta_data_hash_type == NcaFsHeader::MetaDataHashType::HierarchicalIntegrity, fs::ResultRomNcaInvalidSparseMetaDataHashType());

            /* Create the meta storage. */
            std::shared_ptr<fs::IStorage> meta_storage;
            R_TRY(this->CreateSparseStorageMetaStorageWithVerification(std::addressof(meta_storage), out_layer_info_storage, body_substorage, sparse_info.physical_offset, upper_iv, sparse_info, meta_data_hash_data_info, m_hash_generator_factory_selector->GetFactory(fssystem::HashAlgorithmType_Sha2)));

            /* Potentially set the output meta storage. */
            if (out_meta_storage != nullptr) {
                *out_meta_storage = meta_storage;
            }

            /* Create the sparse storage. */
            R_TRY(this->CreateSparseStorageCore(std::addressof(sparse_storage), body_substorage, sparse_info.GetPhysicalSize(), std::move(meta_storage), sparse_info, false));
        } else {
            /* If there are no entries, there's nothing to actually do. */
            sparse_storage = fssystem::AllocateShared<fssystem::SparseStorage>();
            R_UNLESS(sparse_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

            sparse_storage->Initialize(fs_size);
        }

        /* Potentially set the output sparse storage. */
        if (out_sparse_storage != nullptr) {
            *out_sparse_storage = sparse_storage;
        }

        /* Set the output fs data offset. */
        *out_fs_data_offset = fs_offset;

        /* Set the output storage. */
        *out = std::move(sparse_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateAesCtrExStorageMetaStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, s64 offset, NcaFsHeader::EncryptionType encryption_type, const NcaAesCtrUpperIv &upper_iv, const NcaPatchInfo &patch_info) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(encryption_type == NcaFsHeader::EncryptionType::None || encryption_type == NcaFsHeader::EncryptionType::AesCtrEx || encryption_type == NcaFsHeader::EncryptionType::AesCtrExSkipLayerHash);
        AMS_ASSERT(patch_info.HasAesCtrExTable());

        /* Validate patch info extents. */
        R_UNLESS(patch_info.indirect_size > 0,                                                          fs::ResultInvalidNcaPatchInfoIndirectSize());
        R_UNLESS(patch_info.aes_ctr_ex_size > 0,                                                        fs::ResultInvalidNcaPatchInfoAesCtrExSize());
        R_UNLESS(patch_info.indirect_size + patch_info.indirect_offset <= patch_info.aes_ctr_ex_offset, fs::ResultInvalidNcaPatchInfoAesCtrExOffset());

        /* Get the base storage size. */
        s64 base_size;
        R_TRY(base_storage->GetSize(std::addressof(base_size)));

        /* Get and validate the meta extents. */
        const s64 meta_offset = patch_info.aes_ctr_ex_offset;
        const s64 meta_size   = util::AlignUp(static_cast<s64>(patch_info.aes_ctr_ex_size), NcaHeader::XtsBlockSize);
        R_UNLESS(meta_offset + meta_size <= base_size, fs::ResultNcaBaseStorageOutOfRangeB());

        /* Create the encrypted storage. */
        auto enc_storage = fssystem::AllocateShared<fs::SubStorage>(std::move(base_storage), meta_offset, meta_size);
        R_UNLESS(enc_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Create the decrypted storage. */
        std::shared_ptr<fs::IStorage> decrypted_storage;
        if (encryption_type != NcaFsHeader::EncryptionType::None) {
            R_TRY(this->CreateAesCtrStorage(std::addressof(decrypted_storage), std::move(enc_storage), offset + meta_offset, upper_iv, AlignmentStorageRequirement_None));
        } else {
            /* If encryption type is none, don't do any decryption. */
            decrypted_storage = std::move(enc_storage);
        }

        /* Create meta storage. */
        auto meta_storage = fssystem::AllocateShared<BufferedStorage>();
        R_UNLESS(meta_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Initialize the meta storage. */
        R_TRY(meta_storage->Initialize(fs::SubStorage(std::move(decrypted_storage), 0, meta_size), m_buffer_manager, AesCtrExTableCacheBlockSize, AesCtrExTableCacheCount));

        /* Create an alignment-matching storage. */
        using AlignedStorage = AlignmentMatchingStorage<NcaHeader::CtrBlockSize, 1>;
        auto aligned_storage = fssystem::AllocateShared<AlignedStorage>(std::move(meta_storage));
        R_UNLESS(aligned_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Set the output. */
        *out = std::move(aligned_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateAesCtrExStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::AesCtrCounterExtendedStorage> *out_ext, std::shared_ptr<fs::IStorage> base_storage, std::shared_ptr<fs::IStorage> meta_storage, s64 counter_offset, const NcaAesCtrUpperIv &upper_iv, const NcaPatchInfo &patch_info) {
        /* Validate pre-conditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(meta_storage != nullptr);
        AMS_ASSERT(patch_info.HasAesCtrExTable());

        /* Read the bucket tree header. */
        BucketTree::Header header;
        std::memcpy(std::addressof(header), patch_info.aes_ctr_ex_header, sizeof(header));
        R_TRY(header.Verify());

        /* Determine the bucket extents. */
        const auto entry_count = header.entry_count;
        const s64 data_offset  = 0;
        const s64 data_size    = patch_info.aes_ctr_ex_offset;
        const s64 node_offset  = 0;
        const s64 node_size    = AesCtrCounterExtendedStorage::QueryNodeStorageSize(entry_count);
        const s64 entry_offset = node_offset + node_size;
        const s64 entry_size   = AesCtrCounterExtendedStorage::QueryEntryStorageSize(entry_count);

        /* Create bucket storages. */
        fs::SubStorage data_storage(std::move(base_storage), data_offset, data_size);
        fs::SubStorage node_storage(meta_storage, node_offset, node_size);
        fs::SubStorage entry_storage(meta_storage, entry_offset, entry_size);

        /* Get the secure value. */
        const auto secure_value = upper_iv.part.secure_value;

        /* Create the aes ctr ex storage. */
        std::shared_ptr<fs::IStorage> aes_ctr_ex_storage;
        if (m_reader->HasExternalDecryptionKey()) {
            /* Create the decryptor. */
            std::unique_ptr<AesCtrCounterExtendedStorage::IDecryptor> decryptor;
            R_TRY(AesCtrCounterExtendedStorage::CreateExternalDecryptor(std::addressof(decryptor), m_reader->GetExternalDecryptAesCtrFunctionForExternalKey(), -1, -1));

            /* Create the aes ctr ex storage. */
            auto impl_storage = fssystem::AllocateShared<AesCtrCounterExtendedStorage>();
            R_UNLESS(impl_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

            /* Initialize the aes ctr ex storage. */
            R_TRY(impl_storage->Initialize(m_allocator, m_reader->GetExternalDecryptionKey(), AesCtrStorageBySharedPointer::KeySize, secure_value, counter_offset, data_storage, node_storage, entry_storage, entry_count, std::move(decryptor)));

            /* Potentially set the output implementation storage. */
            if (out_ext != nullptr) {
                *out_ext = impl_storage;
            }

            /* Set the implementation storage. */
            aes_ctr_ex_storage = std::move(impl_storage);
        } else {
            /* Create the software decryptor. */
            std::unique_ptr<AesCtrCounterExtendedStorage::IDecryptor> sw_decryptor;
            R_TRY(AesCtrCounterExtendedStorage::CreateSoftwareDecryptor(std::addressof(sw_decryptor)));

            /* Make the software storage. */
            auto sw_storage = fssystem::AllocateShared<AesCtrCounterExtendedStorage>();
            R_UNLESS(sw_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

            /* Initialize the software storage. */
            R_TRY(sw_storage->Initialize(m_allocator, m_reader->GetDecryptionKey(NcaHeader::DecryptionKey_AesCtr), AesCtrStorageBySharedPointer::KeySize, secure_value, counter_offset, data_storage, node_storage, entry_storage, entry_count, std::move(sw_decryptor)));

            /* Potentially set the output implementation storage. */
            if (out_ext != nullptr) {
                *out_ext = sw_storage;
            }

            /* If we have a hardware key and should use it, make the hardware decryption storage. */
            if (m_reader->HasInternalDecryptionKeyForAesHw() && !m_reader->IsSoftwareAesPrioritized()) {
                /* Create the hardware decryptor. */
                std::unique_ptr<AesCtrCounterExtendedStorage::IDecryptor> hw_decryptor;
                R_TRY(AesCtrCounterExtendedStorage::CreateExternalDecryptor(std::addressof(hw_decryptor), m_reader->GetExternalDecryptAesCtrFunction(), m_reader->GetKeyIndex(), m_reader->GetKeyGeneration()));

                /* Create the hardware storage. */
                auto hw_storage = fssystem::AllocateShared<AesCtrCounterExtendedStorage>();
                R_UNLESS(hw_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

                /* Initialize the hardware storage. */
                R_TRY(hw_storage->Initialize(m_allocator, m_reader->GetDecryptionKey(NcaHeader::DecryptionKey_AesCtrHw), AesCtrStorageBySharedPointer::KeySize, secure_value, counter_offset, data_storage, node_storage, entry_storage, entry_count, std::move(hw_decryptor)));

                /* Create the selection storage. */
                auto switch_storage = fssystem::AllocateShared<SwitchStorage<bool (*)()>>(std::move(hw_storage), std::move(sw_storage), IsUsingHwAesCtrForSpeedEmulation);
                R_UNLESS(switch_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

                /* Set the implementation storage. */
                aes_ctr_ex_storage = std::move(switch_storage);
            } else {
                /* Set the implementation storage. */
                aes_ctr_ex_storage = std::move(sw_storage);
            }
        }

        /* Create an alignment-matching storage. */
        using AlignedStorage = AlignmentMatchingStorage<NcaHeader::CtrBlockSize, 1>;
        auto aligned_storage = fssystem::AllocateShared<AlignedStorage>(std::move(aes_ctr_ex_storage));
        R_UNLESS(aligned_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Set the output. */
        *out = std::move(aligned_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateIndirectStorageMetaStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, const NcaPatchInfo &patch_info) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(patch_info.HasIndirectTable());

        /* Get the base storage size. */
        s64 base_size = 0;
        R_TRY(base_storage->GetSize(std::addressof(base_size)));

        /* Check that we're within range. */
        R_UNLESS(patch_info.indirect_offset + patch_info.indirect_size <= base_size, fs::ResultNcaBaseStorageOutOfRangeE());

        /* Allocate the meta storage. */
        auto meta_storage = fssystem::AllocateShared<BufferedStorage>();
        R_UNLESS(meta_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Initialize the meta storage. */
        R_TRY(meta_storage->Initialize(fs::SubStorage(base_storage, patch_info.indirect_offset, patch_info.indirect_size), m_buffer_manager, IndirectTableCacheBlockSize, IndirectTableCacheCount));

        /* Set the output. */
        *out = std::move(meta_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateIndirectStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::IndirectStorage> *out_ind, std::shared_ptr<fs::IStorage> base_storage, std::shared_ptr<fs::IStorage> original_data_storage, std::shared_ptr<fs::IStorage> meta_storage, const NcaPatchInfo &patch_info) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(meta_storage != nullptr);
        AMS_ASSERT(patch_info.HasIndirectTable());

        /* Read the bucket tree header. */
        BucketTree::Header header;
        std::memcpy(std::addressof(header), patch_info.indirect_header, sizeof(header));
        R_TRY(header.Verify());

        /* Determine the storage sizes. */
        const auto node_size  = IndirectStorage::QueryNodeStorageSize(header.entry_count);
        const auto entry_size = IndirectStorage::QueryEntryStorageSize(header.entry_count);
        R_UNLESS(node_size + entry_size <= patch_info.indirect_size, fs::ResultInvalidNcaIndirectStorageOutOfRange());

        /* Get the indirect data size. */
        const s64 indirect_data_size = patch_info.indirect_offset;
        AMS_ASSERT(util::IsAligned(indirect_data_size, NcaHeader::XtsBlockSize));

        /* Create the indirect data storage. */
        auto indirect_data_storage = fssystem::AllocateShared<BufferedStorage>();
        R_UNLESS(indirect_data_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Initialize the indirect data storage. */
        R_TRY(indirect_data_storage->Initialize(fs::SubStorage(base_storage, 0, indirect_data_size), m_buffer_manager, IndirectDataCacheBlockSize, IndirectDataCacheCount));

        /* Enable bulk read on the data storage. */
        indirect_data_storage->EnableBulkRead();

        /* Create the indirect storage. */
        auto indirect_storage = fssystem::AllocateShared<IndirectStorage>();
        R_UNLESS(indirect_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Initialize the indirect storage. */
        R_TRY(indirect_storage->Initialize(m_allocator, fs::SubStorage(meta_storage, 0, node_size), fs::SubStorage(meta_storage, node_size, entry_size), header.entry_count));

        /* Get the original data size. */
        s64 original_data_size;
        R_TRY(original_data_storage->GetSize(std::addressof(original_data_size)));

        /* Set the indirect storages. */
        indirect_storage->SetStorage(0, fs::SubStorage(original_data_storage, 0, original_data_size));
        indirect_storage->SetStorage(1, fs::SubStorage(indirect_data_storage, 0, indirect_data_size));

        /* If necessary, set the output indirect storage. */
        if (out_ind != nullptr) {
            *out_ind = indirect_storage;
        }

        /* Set the output. */
        *out = std::move(indirect_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreatePatchMetaStorage(std::shared_ptr<fs::IStorage> *out_aes_ctr_ex_meta, std::shared_ptr<fs::IStorage> *out_indirect_meta, std::shared_ptr<fs::IStorage> *out_layer_info_storage, std::shared_ptr<fs::IStorage> base_storage, s64 offset, const NcaAesCtrUpperIv &upper_iv, const NcaPatchInfo &patch_info, const NcaMetaDataHashDataInfo &meta_data_hash_data_info, IHash256GeneratorFactory *hgf) {
        /* Validate preconditions. */
        AMS_ASSERT(out_aes_ctr_ex_meta != nullptr);
        AMS_ASSERT(out_indirect_meta != nullptr);
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(patch_info.HasAesCtrExTable());
        AMS_ASSERT(patch_info.HasIndirectTable());
        AMS_ASSERT(util::IsAligned<s64>(patch_info.aes_ctr_ex_size, NcaHeader::XtsBlockSize));

        /* Validate patch info extents. */
        R_UNLESS(patch_info.indirect_size > 0,                                                                 fs::ResultInvalidNcaPatchInfoIndirectSize());
        R_UNLESS(patch_info.aes_ctr_ex_size >= 0,                                                              fs::ResultInvalidNcaPatchInfoAesCtrExSize());
        R_UNLESS(patch_info.indirect_size + patch_info.indirect_offset <= patch_info.aes_ctr_ex_offset,        fs::ResultInvalidNcaPatchInfoAesCtrExOffset());
        R_UNLESS(patch_info.aes_ctr_ex_offset + patch_info.aes_ctr_ex_size <= meta_data_hash_data_info.offset, fs::ResultRomNcaInvalidPatchMetaDataHashDataOffset());

        /* Get the base storage size. */
        s64 base_size;
        R_TRY(base_storage->GetSize(std::addressof(base_size)));

        /* Check that extents remain within range. */
        R_UNLESS(patch_info.indirect_offset + patch_info.indirect_size <= base_size,     fs::ResultNcaBaseStorageOutOfRangeE());
        R_UNLESS(patch_info.aes_ctr_ex_offset + patch_info.aes_ctr_ex_size <= base_size, fs::ResultNcaBaseStorageOutOfRangeB());

        /* Check that metadata hash data extents remain within range. */
        const s64 meta_data_hash_data_offset = meta_data_hash_data_info.offset;
        const s64 meta_data_hash_data_size   = util::AlignUp<s64>(meta_data_hash_data_info.size, NcaHeader::CtrBlockSize);
        R_UNLESS(meta_data_hash_data_offset + meta_data_hash_data_size <= base_size, fs::ResultNcaBaseStorageOutOfRangeB());

        /* Create the encrypted storage. */
        auto enc_storage = fssystem::AllocateShared<fs::SubStorage>(std::move(base_storage), patch_info.indirect_offset, meta_data_hash_data_offset + meta_data_hash_data_size - patch_info.indirect_offset);
        R_UNLESS(enc_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Create the decrypted storage. */
        std::shared_ptr<fs::IStorage> decrypted_storage;
        R_TRY(this->CreateAesCtrStorage(std::addressof(decrypted_storage), std::move(enc_storage), offset + patch_info.indirect_offset, upper_iv, AlignmentStorageRequirement_None));

        /* Create the verification storage. */
        std::shared_ptr<fs::IStorage> integrity_storage;
        R_TRY_CATCH(this->CreateIntegrityVerificationStorageForMeta(std::addressof(integrity_storage), out_layer_info_storage, std::move(decrypted_storage), patch_info.indirect_offset, meta_data_hash_data_info, hgf)) {
            R_CONVERT(fs::ResultInvalidNcaMetaDataHashDataSize, fs::ResultRomNcaInvalidPatchMetaDataHashDataSize())
            R_CONVERT(fs::ResultInvalidNcaMetaDataHashDataHash, fs::ResultRomNcaInvalidPatchMetaDataHashDataHash())
        } R_END_TRY_CATCH;

        /* Create the indirect meta storage. */
        auto indirect_meta_storage = fssystem::AllocateShared<fs::SubStorage>(integrity_storage, patch_info.indirect_offset - patch_info.indirect_offset, patch_info.indirect_size);
        R_UNLESS(indirect_meta_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Create the aes ctr ex meta storage. */
        auto aes_ctr_ex_meta_storage = fssystem::AllocateShared<fs::SubStorage>(integrity_storage, patch_info.aes_ctr_ex_offset - patch_info.indirect_offset, patch_info.aes_ctr_ex_size);
        R_UNLESS(aes_ctr_ex_meta_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Set the output. */
        *out_aes_ctr_ex_meta = std::move(aes_ctr_ex_meta_storage);
        *out_indirect_meta   = std::move(indirect_meta_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateSha256Storage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, const NcaFsHeader::HashData::HierarchicalSha256Data &hash_data, IHash256GeneratorFactory *hgf) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);

        /* Define storage types. */
        using VerificationStorage = HierarchicalSha256Storage<fs::SubStorage>;
        using CacheStorage        = ReadOnlyBlockCacheStorage;
        using AlignedStorage      = AlignmentMatchingStoragePooledBuffer<std::shared_ptr<fs::IStorage>, 1>;

        /* Validate the hash data. */
        R_UNLESS(util::IsPowerOfTwo(hash_data.hash_block_size),                     fs::ResultInvalidHierarchicalSha256BlockSize());
        R_UNLESS(hash_data.hash_layer_count == VerificationStorage::LayerCount - 1, fs::ResultInvalidHierarchicalSha256LayerCount());

        /* Get the regions. */
        const auto &hash_region = hash_data.hash_layer_region[0];
        const auto &data_region = hash_data.hash_layer_region[1];

        /* Determine buffer sizes. */
        constexpr s32 CacheBlockCount = 2;
        const auto hash_buffer_size  = static_cast<size_t>(hash_region.size);
        const auto cache_buffer_size = CacheBlockCount * hash_data.hash_block_size;
        const auto total_buffer_size = hash_buffer_size + cache_buffer_size;

        /* Make a buffer holder storage. */
        auto buffer_hold_storage = fssystem::AllocateShared<MemoryResourceBufferHoldStorage>(std::move(base_storage), m_allocator, total_buffer_size);
        R_UNLESS(buffer_hold_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());
        R_UNLESS(buffer_hold_storage->IsValid(), fs::ResultAllocationMemoryFailedInNcaFileSystemDriverI());

        /* Get storage size. */
        s64 base_size;
        R_TRY(buffer_hold_storage->GetSize(std::addressof(base_size)));

        /* Check that we're within range. */
        R_UNLESS(hash_region.offset + hash_region.size <= base_size, fs::ResultNcaBaseStorageOutOfRangeC());
        R_UNLESS(data_region.offset + data_region.size <= base_size, fs::ResultNcaBaseStorageOutOfRangeC());

        /* Create the master hash storage. */
        fs::MemoryStorage master_hash_storage(const_cast<Hash *>(std::addressof(hash_data.fs_data_master_hash)), sizeof(Hash));

        /* Make the verification storage. */
        auto verification_storage = fssystem::AllocateShared<VerificationStorage>();
        R_UNLESS(verification_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Make layer storages. */
        fs::SubStorage layer_storages[VerificationStorage::LayerCount] = {
            fs::SubStorage(std::addressof(master_hash_storage), 0, sizeof(Hash)),
            fs::SubStorage(buffer_hold_storage.get(), hash_region.offset, hash_region.size),
            fs::SubStorage(buffer_hold_storage, data_region.offset, data_region.size)
        };

        /* Initialize the verification storage. */
        R_TRY(verification_storage->Initialize(layer_storages, util::size(layer_storages), hash_data.hash_block_size, buffer_hold_storage->GetBuffer(), hash_buffer_size, hgf));

        /* Make the cache storage. */
        auto cache_storage = fssystem::AllocateShared<CacheStorage>(std::move(verification_storage), hash_data.hash_block_size, static_cast<char *>(buffer_hold_storage->GetBuffer()) + hash_buffer_size, cache_buffer_size, CacheBlockCount);
        R_UNLESS(cache_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Make the aligned storage. */
        auto aligned_storage = fssystem::AllocateShared<AlignedStorage>(std::move(cache_storage), hash_data.hash_block_size);
        R_UNLESS(aligned_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Set the output. */
        *out = std::move(aligned_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateIntegrityVerificationStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, const NcaFsHeader::HashData::IntegrityMetaInfo &meta_info, IHash256GeneratorFactory *hgf) {
        R_RETURN(this->CreateIntegrityVerificationStorageImpl(out, base_storage, meta_info, 0, IntegrityDataCacheCount, IntegrityHashCacheCount, fssystem::HierarchicalIntegrityVerificationStorage::GetDefaultDataCacheBufferLevel(meta_info.level_hash_info.max_layers), hgf));
    }

    Result NcaFileSystemDriver::CreateIntegrityVerificationStorageForMeta(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> *out_layer_info_storage, std::shared_ptr<fs::IStorage> base_storage, s64 offset, const NcaMetaDataHashDataInfo &meta_data_hash_data_info, IHash256GeneratorFactory *hgf) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);

        /* Check the meta data hash data size. */
        R_UNLESS(meta_data_hash_data_info.size == sizeof(NcaMetaDataHashData), fs::ResultInvalidNcaMetaDataHashDataSize());

        /* Read the meta data hash data. */
        NcaMetaDataHashData meta_data_hash_data;
        R_TRY(base_storage->Read(meta_data_hash_data_info.offset - offset, std::addressof(meta_data_hash_data), sizeof(meta_data_hash_data)));

        /* Check the meta data hash data hash. */
        u8 meta_data_hash_data_hash[IHash256Generator::HashSize];
        m_hash_generator_factory_selector->GetFactory(fssystem::HashAlgorithmType_Sha2)->GenerateHash(meta_data_hash_data_hash, sizeof(meta_data_hash_data_hash), std::addressof(meta_data_hash_data), sizeof(meta_data_hash_data));
        R_UNLESS(crypto::IsSameBytes(meta_data_hash_data_hash, std::addressof(meta_data_hash_data_info.hash), sizeof(meta_data_hash_data_hash)), fs::ResultInvalidNcaMetaDataHashDataHash());

        /* Set the out layer info storage, if necessary. */
        if (out_layer_info_storage != nullptr) {
            auto layer_info_storage = fssystem::AllocateShared<fs::SubStorage>(base_storage, meta_data_hash_data.layer_info_offset - offset, meta_data_hash_data_info.offset + meta_data_hash_data_info.size - meta_data_hash_data.layer_info_offset);
            R_UNLESS(layer_info_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

            *out_layer_info_storage = std::move(layer_info_storage);
        }

        /* Create the meta storage. */
        auto meta_storage = fssystem::AllocateShared<fs::SubStorage>(std::move(base_storage), 0, meta_data_hash_data_info.offset - offset);
        R_UNLESS(meta_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Create the integrity verification storage. */
        R_RETURN(this->CreateIntegrityVerificationStorageImpl(out, std::move(meta_storage), meta_data_hash_data.integrity_meta_info, meta_data_hash_data.layer_info_offset - offset, IntegrityDataCacheCountForMeta, IntegrityHashCacheCountForMeta, 0, hgf));
    }

    Result NcaFileSystemDriver::CreateIntegrityVerificationStorageImpl(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fs::IStorage> base_storage, const NcaFsHeader::HashData::IntegrityMetaInfo &meta_info, s64 layer_info_offset, int max_data_cache_entries, int max_hash_cache_entries, s8 buffer_level, IHash256GeneratorFactory *hgf) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(layer_info_offset >= 0);

        /* Define storage types. */
        using VerificationStorage = HierarchicalIntegrityVerificationStorage;
        using StorageInfo         = VerificationStorage::HierarchicalStorageInformation;

        /* Validate the meta info. */
        HierarchicalIntegrityVerificationInformation level_hash_info;
        std::memcpy(std::addressof(level_hash_info), std::addressof(meta_info.level_hash_info), sizeof(level_hash_info));

        R_UNLESS(IntegrityMinLayerCount <= level_hash_info.max_layers,   fs::ResultInvalidNcaHierarchicalIntegrityVerificationLayerCount());
        R_UNLESS(level_hash_info.max_layers   <= IntegrityMaxLayerCount, fs::ResultInvalidNcaHierarchicalIntegrityVerificationLayerCount());

        /* Get the base storage size. */
        s64 base_storage_size;
        R_TRY(base_storage->GetSize(std::addressof(base_storage_size)));

        /* Create storage info. */
        StorageInfo storage_info;
        for (s32 i = 0; i < static_cast<s32>(level_hash_info.max_layers - 2); ++i) {
            const auto &layer_info = level_hash_info.info[i];
            R_UNLESS(layer_info_offset + layer_info.offset + layer_info.size <= base_storage_size, fs::ResultNcaBaseStorageOutOfRangeD());

            storage_info[i + 1] = fs::SubStorage(base_storage, layer_info_offset + layer_info.offset, layer_info.size);
        }

        /* Set the last layer info. */
        const auto &layer_info = level_hash_info.info[level_hash_info.max_layers - 2];
        const s64 last_layer_info_offset = layer_info_offset > 0 ? 0 : layer_info.offset;
        R_UNLESS(last_layer_info_offset + layer_info.size <= base_storage_size, fs::ResultNcaBaseStorageOutOfRangeD());
        if (layer_info_offset > 0) {
            R_UNLESS(last_layer_info_offset + layer_info.size <= layer_info_offset, fs::ResultRomNcaInvalidIntegrityLayerInfoOffset());
        }
        storage_info.SetDataStorage(fs::SubStorage(std::move(base_storage), last_layer_info_offset, layer_info.size));

        /* Make the integrity romfs storage. */
        auto integrity_storage = fssystem::AllocateShared<fssystem::IntegrityRomFsStorage>();
        R_UNLESS(integrity_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Initialize the integrity storage. */
        R_TRY(integrity_storage->Initialize(level_hash_info, meta_info.master_hash, storage_info, m_buffer_manager, max_data_cache_entries, max_hash_cache_entries, buffer_level, hgf));

        /* Set the output. */
        *out = std::move(integrity_storage);
        R_SUCCEED();
    }


    Result NcaFileSystemDriver::CreateRegionSwitchStorage(std::shared_ptr<fs::IStorage> *out, const NcaFsHeaderReader *header_reader, std::shared_ptr<fs::IStorage> inside_storage, std::shared_ptr<fs::IStorage> outside_storage) {
        /* Check pre-conditions. */
        AMS_ASSERT(header_reader->GetHashType() == NcaFsHeader::HashType::HierarchicalIntegrityHash);

        /* Create the region. */
        fssystem::RegionSwitchStorage::Region region = {};
        R_TRY(header_reader->GetHashTargetOffset(std::addressof(region.size)));

        /* Create the region switch storage. */
        auto region_switch_storage = fssystem::AllocateShared<fssystem::RegionSwitchStorage>(std::move(inside_storage), std::move(outside_storage), region);
        R_UNLESS(region_switch_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Set the output. */
        *out = std::move(region_switch_storage);
        R_SUCCEED();
    }

    Result NcaFileSystemDriver::CreateCompressedStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::CompressedStorage> *out_cmp, std::shared_ptr<fs::IStorage> *out_meta, std::shared_ptr<fs::IStorage> base_storage, const NcaCompressionInfo &compression_info) {
        R_RETURN(this->CreateCompressedStorage(out, out_cmp, out_meta, std::move(base_storage), compression_info, m_reader->GetDecompressor(), m_allocator, m_buffer_manager));
    }

    Result NcaFileSystemDriver::CreateCompressedStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::CompressedStorage> *out_cmp, std::shared_ptr<fs::IStorage> *out_meta, std::shared_ptr<fs::IStorage> base_storage, const NcaCompressionInfo &compression_info, GetDecompressorFunction get_decompressor, MemoryResource *allocator, fs::IBufferManager *buffer_manager) {
        /* Check pre-conditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(get_decompressor != nullptr);

        /* Read and verify the bucket tree header. */
        BucketTree::Header header;
        std::memcpy(std::addressof(header), compression_info.bucket.header, sizeof(header));
        R_TRY(header.Verify());

        /* Determine the storage extents. */
        const auto table_offset = compression_info.bucket.offset;
        const auto table_size   = compression_info.bucket.size;
        const auto node_size    = CompressedStorage::QueryNodeStorageSize(header.entry_count);
        const auto entry_size   = CompressedStorage::QueryEntryStorageSize(header.entry_count);
        R_UNLESS(node_size + entry_size <= table_size, fs::ResultInvalidCompressedStorageSize());

        /* If we should, set the output meta storage. */
        if (out_meta != nullptr) {
            auto meta_storage = fssystem::AllocateShared<fs::SubStorage>(base_storage, table_offset, table_size);
            R_UNLESS(meta_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

            *out_meta = std::move(meta_storage);
        }

        /* Allocate the compressed storage. */
        auto compressed_storage = fssystem::AllocateShared<fssystem::CompressedStorage>();
        R_UNLESS(compressed_storage != nullptr, fs::ResultAllocationMemoryFailedAllocateShared());

        /* Initialize the compressed storage. */
        R_TRY(compressed_storage->Initialize(allocator, buffer_manager, fs::SubStorage(base_storage, 0, table_offset), fs::SubStorage(base_storage, table_offset, node_size), fs::SubStorage(base_storage, table_offset + node_size, entry_size), header.entry_count, 64_KB, 640_KB, get_decompressor, 16_KB, 16_KB, 32));

        /* Potentially set the output compressed storage. */
        if (out_cmp) {
            *out_cmp = compressed_storage;
        }

        /* Set the output. */
        *out = std::move(compressed_storage);
        R_SUCCEED();
    }

}
