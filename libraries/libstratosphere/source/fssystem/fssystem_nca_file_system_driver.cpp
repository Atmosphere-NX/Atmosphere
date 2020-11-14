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
#include "fssystem_read_only_block_cache_storage.hpp"
#include "fssystem_hierarchical_sha256_storage.hpp"

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

        class BufferHolder {
            NON_COPYABLE(BufferHolder);
            private:
                MemoryResource *allocator;
                char *buffer;
                size_t buffer_size;
            public:
                BufferHolder() : allocator(), buffer(), buffer_size() { /* ... */ }
                BufferHolder(MemoryResource *a, size_t sz) : allocator(a), buffer(static_cast<char *>(a->Allocate(sz))), buffer_size(sz) { /* ... */ }
                ~BufferHolder() {
                    if (this->buffer != nullptr) {
                        this->allocator->Deallocate(this->buffer, this->buffer_size);
                        this->buffer = nullptr;
                    }
                }

                BufferHolder(BufferHolder &&rhs) : allocator(rhs.allocator), buffer(rhs.buffer), buffer_size(rhs.buffer_size) {
                    rhs.buffer = nullptr;
                }

                BufferHolder &operator=(BufferHolder &&rhs) {
                    if (this != std::addressof(rhs)) {
                        AMS_ASSERT(this->buffer == nullptr);
                        this->allocator   = rhs.allocator;
                        this->buffer      = rhs.buffer;
                        this->buffer_size = rhs.buffer_size;

                        rhs.buffer        = nullptr;
                    }
                    return *this;
                }

                bool IsValid() const { return this->buffer != nullptr; }
                char *Get() const { return this->buffer; }
                size_t GetSize() const { return this->buffer_size; }
        };

        template<typename Base, typename Sequence>
        class DerivedStorageHolderImpl;

        template<typename Base, std::size_t... Is>
        class DerivedStorageHolderImpl<Base, std::index_sequence<Is...>> : public Base {
            NON_COPYABLE(DerivedStorageHolderImpl);
            public:
                using StoragePointer = std::unique_ptr<fs::IStorage>;

                template<size_t N>
                using IndexedStoragePointer = StoragePointer;
            private:
                std::shared_ptr<NcaReader> nca_reader;
                std::array<StoragePointer, sizeof...(Is)> storages;
            private:

                template<size_t N>
                void SetImpl(IndexedStoragePointer<N> &&ptr) {
                    static_assert(N < sizeof...(Is));
                    this->storages[N] = std::move(ptr);
                }
            public:
                DerivedStorageHolderImpl() : Base(), nca_reader(), storages() { /* ... */ }
                explicit DerivedStorageHolderImpl(std::shared_ptr<NcaReader> nr) : Base(), nca_reader(nr), storages() { /* ... */ }

                #define DEFINE_CONSTRUCTORS(n)                                                                                                                                                                                   \
                template<AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS##n (T)>                                                                                                                                                           \
                explicit DerivedStorageHolderImpl(AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS##n (T, t)) : Base(AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS##n (T, t)), nca_reader(), storages() { /* ... */ }                                  \
                template<AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS##n (T)>                                                                                                                                                           \
                explicit DerivedStorageHolderImpl(AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS##n (T, t), std::shared_ptr<NcaReader> nr) : Base(AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS##n (T, t)), nca_reader(nr), storages() { /* ... */ }

                AMS_UTIL_VARIADIC_INVOKE_MACRO(DEFINE_CONSTRUCTORS)

                #undef  DEFINE_CONSTRUCTORS

                void Set(IndexedStoragePointer<Is> &&... ptrs) {
                    (this->SetImpl<Is>(std::forward<IndexedStoragePointer<Is>>(ptrs)), ...);
                }
        };

        template<typename Base, size_t N>
        using DerivedStorageHolder = DerivedStorageHolderImpl<Base, std::make_index_sequence<N>>;

        template<typename Base, size_t N>
        class DerivedStorageHolderWithBuffer : public DerivedStorageHolder<Base, N> {
            NON_COPYABLE(DerivedStorageHolderWithBuffer);
            private:
                using BaseHolder = DerivedStorageHolder<Base, N>;
            private:
                BufferHolder buffer;
            public:
                DerivedStorageHolderWithBuffer() : BaseHolder(), buffer() { /* ... */ }

                template<typename... Args>
                DerivedStorageHolderWithBuffer(Args &&... args) : BaseHolder(std::forward<Args>(args)...), buffer() { /* ... */ }

                using BaseHolder::Set;

                void Set(BufferHolder &&buf) {
                    this->buffer = std::move(buf);
                }
        };

        class AesCtrStorageExternal : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
            NON_COPYABLE(AesCtrStorageExternal);
            NON_MOVEABLE(AesCtrStorageExternal);
            public:
                static constexpr size_t BlockSize = crypto::Aes128CtrEncryptor::BlockSize;
                static constexpr size_t KeySize   = crypto::Aes128CtrEncryptor::KeySize;
                static constexpr size_t IvSize    = crypto::Aes128CtrEncryptor::IvSize;
            private:
                IStorage * const base_storage;
                u8 iv[IvSize];
                DecryptAesCtrFunction decrypt_function;
                s32 key_index;
                u8 encrypted_key[KeySize];
            public:
                AesCtrStorageExternal(fs::IStorage *bs, const void *enc_key, size_t enc_key_size, const void *iv, size_t iv_size, DecryptAesCtrFunction df, s32 kidx) : base_storage(bs), decrypt_function(df), key_index(kidx) {
                    AMS_ASSERT(bs != nullptr);
                    AMS_ASSERT(enc_key_size == KeySize);
                    AMS_ASSERT(iv != nullptr);
                    AMS_ASSERT(iv_size == IvSize);

                    std::memcpy(this->iv, iv, IvSize);
                    std::memcpy(this->encrypted_key, enc_key, enc_key_size);
                }

                virtual Result Read(s64 offset, void *buffer, size_t size) override {
                    /* Allow zero size. */
                    R_SUCCEED_IF(size == 0);

                    /* Validate arguments. */
                    /* NOTE: For some reason, Nintendo uses InvalidArgument instead of InvalidOffset/InvalidSize here. */
                    R_UNLESS(buffer != nullptr,                  fs::ResultNullptrArgument());
                    R_UNLESS(util::IsAligned(offset, BlockSize), fs::ResultInvalidArgument());
                    R_UNLESS(util::IsAligned(size,   BlockSize), fs::ResultInvalidArgument());

                    /* Read the data. */
                    R_TRY(this->base_storage->Read(offset, buffer, size));

                    /* Temporarily increase our thread priority. */
                    ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

                    /* Allocate a pooled buffer for decryption. */
                    PooledBuffer pooled_buffer;
                    pooled_buffer.AllocateParticularlyLarge(size, BlockSize);
                    AMS_ASSERT(pooled_buffer.GetSize() >= BlockSize);

                    /* Setup the counter. */
                    u8 ctr[IvSize];
                    std::memcpy(ctr, this->iv, IvSize);
                    AddCounter(ctr, IvSize, offset / BlockSize);

                    /* Setup tracking. */
                    size_t remaining_size = size;
                    s64 cur_offset = 0;

                    while (remaining_size > 0) {
                        /* Get the current size to process. */
                        size_t cur_size = std::min(pooled_buffer.GetSize(), remaining_size);
                        char *dst = static_cast<char *>(buffer) + cur_offset;

                        /* Decrypt into the temporary buffer */
                        this->decrypt_function(pooled_buffer.GetBuffer(), cur_size, this->key_index, this->encrypted_key, KeySize, ctr, IvSize, dst, cur_size);

                        /* Copy to the destination. */
                        std::memcpy(dst, pooled_buffer.GetBuffer(), cur_size);

                        /* Update tracking. */
                        cur_offset     += cur_size;
                        remaining_size -= cur_size;

                        if (remaining_size > 0) {
                            AddCounter(ctr, IvSize, cur_size / BlockSize);
                        }
                    }

                    return ResultSuccess();
                }
                virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                    switch (op_id) {
                        case fs::OperationId::QueryRange:
                            {
                                /* Validate that we have an output range info. */
                                R_UNLESS(dst != nullptr,                         fs::ResultNullptrArgument());
                                R_UNLESS(dst_size == sizeof(fs::QueryRangeInfo), fs::ResultInvalidSize());

                                /* Operate on our base storage. */
                                R_TRY(this->base_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));

                                /* Add in new flags. */
                                fs::QueryRangeInfo new_info;
                                new_info.Clear();
                                new_info.aes_ctr_key_type = static_cast<s32>(this->key_index >= 0 ? fs::AesCtrKeyTypeFlag::InternalKeyForHardwareAes : fs::AesCtrKeyTypeFlag::ExternalKeyForHardwareAes);
                            }
                        default:
                            {
                                /* Operate on our base storage. */
                                R_TRY(this->base_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                                return ResultSuccess();
                            }
                    }
                }

                virtual Result GetSize(s64 *out) override {
                    return this->base_storage->GetSize(out);
                }

                virtual Result Flush() override {
                    return ResultSuccess();
                }

                virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                    return fs::ResultUnsupportedOperationInAesCtrStorageExternalA();
                }

                virtual Result SetSize(s64 size) override {
                    return fs::ResultUnsupportedOperationInAesCtrStorageExternalB();
                }
        };

        template<typename F>
        class SwitchStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
            NON_COPYABLE(SwitchStorage);
            NON_MOVEABLE(SwitchStorage);
            private:
                std::unique_ptr<fs::IStorage> true_storage;
                std::unique_ptr<fs::IStorage> false_storage;
                F truth_function;
            private:
                ALWAYS_INLINE std::unique_ptr<fs::IStorage> &SelectStorage() {
                    return this->truth_function() ? this->true_storage : this->false_storage;
                }
            public:
                SwitchStorage(std::unique_ptr<fs::IStorage> &&t, std::unique_ptr<fs::IStorage> &&f, F func) : true_storage(std::move(t)), false_storage(std::move(f)), truth_function(func) { /* ... */ }

                virtual Result Read(s64 offset, void *buffer, size_t size) override {
                    return this->SelectStorage()->Read(offset, buffer, size);
                }

                virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                    switch (op_id) {
                        case fs::OperationId::InvalidateCache:
                            {
                                R_TRY(this->true_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                                R_TRY(this->false_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                                return ResultSuccess();
                            }
                        case fs::OperationId::QueryRange:
                            {
                                R_TRY(this->SelectStorage()->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                                return ResultSuccess();
                            }
                        default:
                            return fs::ResultUnsupportedOperationInSwitchStorageA();
                    }
                }

                virtual Result GetSize(s64 *out) override {
                    return this->SelectStorage()->GetSize(out);
                }

                virtual Result Flush() override {
                    return this->SelectStorage()->Flush();
                }

                virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                    return this->SelectStorage()->Write(offset, buffer, size);
                }

                virtual Result SetSize(s64 size) override {
                    return this->SelectStorage()->SetSize(size);
                }
        };

        inline s64 GetFsOffset(const NcaReader &reader, s32 fs_index) {
            return static_cast<s64>(reader.GetFsOffset(fs_index));
        }

        inline s64 GetFsEndOffset(const NcaReader &reader, s32 fs_index) {
            return static_cast<s64>(reader.GetFsEndOffset(fs_index));
        }

        inline void MakeAesXtsIv(void *ctr, s64 base_offset) {
            util::StoreBigEndian<s64>(static_cast<s64 *>(ctr) + 1, base_offset / NcaHeader::XtsBlockSize);
        }

        inline bool IsUsingHardwareAesCtrForSpeedEmulation() {
            auto mode = fssystem::SpeedEmulationConfiguration::GetSpeedEmulationMode();
            return mode == fs::SpeedEmulationMode::None || mode == fs::SpeedEmulationMode::Slower;
        }

        using Sha256DataRegion   = NcaFsHeader::Region;
        using IntegrityLevelInfo = NcaFsHeader::HashData::IntegrityMetaInfo::LevelHashInfo;
        using IntegrityDataInfo  = IntegrityLevelInfo::HierarchicalIntegrityVerificationLevelInformation;

        inline const Sha256DataRegion &GetSha256DataRegion(const NcaFsHeader::HashData &hash_data) {
            return hash_data.hierarchical_sha256_data.hash_layer_region[1];
        }

        inline const IntegrityDataInfo &GetIntegrityDataInfo(const NcaFsHeader::HashData &hash_data) {
            return hash_data.integrity_meta_info.level_hash_info.info[hash_data.integrity_meta_info.level_hash_info.max_layers - 2];
        }

    }

    Result NcaFileSystemDriver::OpenRawStorage(std::shared_ptr<fs::IStorage> *out, s32 fs_index) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(0 <= fs_index && fs_index < NcaHeader::FsCountMax);
        AMS_ASSERT(this->reader != nullptr);

        /* Get storage extents. */
        const auto storage_offset = GetFsOffset(*this->reader, fs_index);
        const auto storage_size   = GetFsEndOffset(*this->reader, fs_index) - storage_offset;
        R_UNLESS(storage_size > 0, fs::ResultInvalidNcaHeader());

        /* Allocate a substorage. */
        *out = AllocateShared<DerivedStorageHolder<fs::SubStorage, 0>>(this->reader->GetBodyStorage(), storage_offset, storage_size, this->reader);
        R_UNLESS(*out != nullptr, fs::ResultAllocationFailureInAllocateShared());

        return ResultSuccess();
    }

    Result NcaFileSystemDriver::OpenStorage(std::shared_ptr<fs::IStorage> *out, NcaFsHeaderReader *out_header_reader, s32 fs_index) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(out_header_reader != nullptr);
        AMS_ASSERT(0 <= fs_index && fs_index < NcaHeader::FsCountMax);

        /* Open a reader with the appropriate option. */
        StorageOption option(out_header_reader, fs_index);
        R_TRY(this->OpenStorage(out, std::addressof(option)));

        return ResultSuccess();
    }

    Result NcaFileSystemDriver::OpenStorage(std::shared_ptr<fs::IStorage> *out, StorageOption *option) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(option != nullptr);
        AMS_ASSERT(this->reader != nullptr);

        /* Get and validate fs index. */
        const auto fs_index = option->GetFsIndex();
        R_UNLESS(this->reader->HasFsInfo(fs_index), fs::ResultPartitionNotFound());

        /* Initialize a reader for the fs header. */
        auto &header_reader = option->GetHeaderReader();
        R_TRY(header_reader.Initialize(*this->reader, fs_index));

        /* Create the storage. */
        std::unique_ptr<fs::IStorage> storage;
        {
            BaseStorage base_storage;
            R_TRY(this->CreateBaseStorage(std::addressof(base_storage), option));
            R_TRY(this->CreateDecryptableStorage(std::addressof(storage), option, std::addressof(base_storage)));
        }
        R_TRY(this->CreateIndirectStorage(std::addressof(storage), option, std::move(storage)));
        R_TRY(this->CreateVerificationStorage(std::addressof(storage), std::move(storage), std::addressof(header_reader)));

        /* Set the output. */
        *out = std::move(storage);
        return ResultSuccess();
    }

    Result NcaFileSystemDriver::OpenDecryptableStorage(std::shared_ptr<fs::IStorage> *out, StorageOption *option, bool indirect_needed) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(option != nullptr);
        AMS_ASSERT(this->reader != nullptr);

        /* Get and validate fs index. */
        const auto fs_index = option->GetFsIndex();
        R_UNLESS(this->reader->HasFsInfo(fs_index), fs::ResultPartitionNotFound());

        /* Initialize a reader for the fs header. */
        auto &header_reader = option->GetHeaderReader();
        if (!header_reader.IsInitialized()) {
            R_TRY(header_reader.Initialize(*this->reader, fs_index));
        }

        /* Create the storage. */
        std::unique_ptr<fs::IStorage> storage;
        {
            BaseStorage base_storage;
            R_TRY(this->CreateBaseStorage(std::addressof(base_storage), option));
            R_TRY(this->CreateDecryptableStorage(std::addressof(storage), option, std::addressof(base_storage)));
        }

        /* Set the data storage. */
        {
            const auto &patch_info = header_reader.GetPatchInfo();
            s64 data_storage_size = 0;

            if (header_reader.GetEncryptionType() == NcaFsHeader::EncryptionType::AesCtrEx) {
                data_storage_size = patch_info.aes_ctr_ex_offset;
            } else {
                switch (header_reader.GetHashType()) {
                    case NcaFsHeader::HashType::HierarchicalSha256Hash:
                    {
                        const auto &region = GetSha256DataRegion(header_reader.GetHashData());
                        data_storage_size = region.offset + region.size;
                    }
                    break;
                    case NcaFsHeader::HashType::HierarchicalIntegrityHash:
                    {
                        const auto &info = GetIntegrityDataInfo(header_reader.GetHashData());
                        data_storage_size = info.offset + info.size;
                    }
                    break;
                    default:
                        return fs::ResultInvalidNcaFsHeaderHashType();
                }

                data_storage_size = util::AlignUp(data_storage_size, NcaHeader::XtsBlockSize);
            }

            /* Set the data storage in option. */
            option->SetDataStorage(storage.get(), data_storage_size);
        }

        /* Create the indirect storage if needed. */
        if (indirect_needed) {
            R_TRY(this->CreateIndirectStorage(std::addressof(storage), option, std::move(storage)));
        }

        /* Set the output. */
        *out = std::move(storage);
        return ResultSuccess();
    }

    Result NcaFileSystemDriver::CreateBaseStorage(BaseStorage *out, StorageOption *option) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(option != nullptr);

        /* Get the header reader. */
        const auto fs_index       = option->GetFsIndex();
        const auto &header_reader = option->GetHeaderReader();

        /* Get storage extents. */
        const auto storage_offset = GetFsOffset(*this->reader, fs_index);
        const auto storage_size   = GetFsEndOffset(*this->reader, fs_index) - storage_offset;
        R_UNLESS(storage_size > 0, fs::ResultInvalidNcaHeader());

        /* Set up the sparse storage if we need to, otherwise use body storage directly. */
        if (header_reader.ExistsSparseLayer()) {
            const auto &sparse_info = header_reader.GetSparseInfo();

            /* Read and verify the bucket tree header. */
            BucketTree::Header header;
            std::memcpy(std::addressof(header), sparse_info.bucket.header, sizeof(header));
            R_TRY(header.Verify());

            /* Create a new holder for the storages. */
            std::unique_ptr storage = std::make_unique<DerivedStorageHolder<SparseStorage, 2>>(this->reader);
            R_UNLESS(storage != nullptr, fs::ResultAllocationFailureInNew());

            /* If there are no entries, there's nothing to actually do. */
            if (header.entry_count == 0) {
                storage->Initialize(storage_size);
            } else {
                /* Prepare to create the decryptable storage. */
                const auto raw_storage        = this->reader->GetBodyStorage();
                const auto raw_storage_offset = sparse_info.physical_offset;
                const auto raw_storage_size   = sparse_info.GetPhysicalSize();

                /* Validate that we're within range. */
                s64 body_storage_size = 0;
                R_TRY(raw_storage->GetSize(std::addressof(body_storage_size)));
                R_UNLESS(raw_storage_offset + raw_storage_size <= body_storage_size, fs::ResultNcaBaseStorageOutOfRangeB());

                /* Create the decryptable storage. */
                std::unique_ptr<fs::IStorage> decryptable_storage;
                {
                    BaseStorage base_storage(raw_storage, raw_storage_offset, raw_storage_size);
                    base_storage.SetStorageOffset(raw_storage_offset);
                    base_storage.SetAesCtrUpperIv(sparse_info.MakeAesCtrUpperIv(header_reader.GetAesCtrUpperIv()));
                    R_TRY(this->CreateAesCtrStorage(std::addressof(decryptable_storage), std::addressof(base_storage)));
                }

                /* Create the table storage. */
                std::unique_ptr table_storage = std::make_unique<save::BufferedStorage>();
                R_UNLESS(table_storage != nullptr, fs::ResultAllocationFailureInNew());

                /* Initialize the table storage. */
                R_TRY(table_storage->Initialize(fs::SubStorage(decryptable_storage.get(), 0, raw_storage_size), this->buffer_manager, SparseTableCacheBlockSize, SparseTableCacheCount));

                /* Determine storage extents. */
                const auto node_offset = sparse_info.bucket.offset;
                const auto node_size   = SparseStorage::QueryNodeStorageSize(header.entry_count);
                const auto entry_offset = node_offset + node_size;
                const auto entry_size   = SparseStorage::QueryEntryStorageSize(header.entry_count);

                /* Initialize the storage. */
                R_TRY(storage->Initialize(this->allocator, fs::SubStorage(table_storage.get(), node_offset, node_size), fs::SubStorage(table_storage.get(), entry_offset, entry_size), header.entry_count));

                /* Set the data/decryptable storage. */
                storage->SetDataStorage(raw_storage, raw_storage_offset, node_offset);
                storage->Set(std::move(decryptable_storage), std::move(table_storage));
            }

            /* Set the sparse storage. */
            option->SetSparseStorage(storage.get());

            /* Set the out storage. */
            out->SetStorage(std::move(storage));
        } else {
            /* Validate that we're within range. */
            s64 body_storage_size;
            R_TRY(this->reader->GetBodyStorage()->GetSize(std::addressof(body_storage_size)));
            R_UNLESS(storage_offset + storage_size <= body_storage_size, fs::ResultNcaBaseStorageOutOfRangeB());

            /* Set the out storage. */
            out->SetStorage(this->reader->GetBodyStorage(), storage_offset, storage_size);
        }

        /* Set the crypto variables. */
        out->SetStorageOffset(storage_offset);
        out->SetAesCtrUpperIv(header_reader.GetAesCtrUpperIv());

        return ResultSuccess();
    }

    Result NcaFileSystemDriver::CreateDecryptableStorage(std::unique_ptr<fs::IStorage> *out, StorageOption *option, BaseStorage *base_storage) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(option != nullptr);
        AMS_ASSERT(base_storage != nullptr);

        /* Get the header reader. */
        const auto &header_reader = option->GetHeaderReader();

        /* Create the appropriate storage for the encryption type. */
        switch (header_reader.GetEncryptionType()) {
            case NcaFsHeader::EncryptionType::None:
                *out = base_storage->MakeStorage();
                R_UNLESS(*out != nullptr, fs::ResultAllocationFailureInNew());
                break;
            case NcaFsHeader::EncryptionType::AesXts:
                R_TRY(this->CreateAesXtsStorage(out, base_storage));
                break;
            case NcaFsHeader::EncryptionType::AesCtr:
                R_TRY(this->CreateAesCtrStorage(out, base_storage));
                break;
            case NcaFsHeader::EncryptionType::AesCtrEx:
                R_TRY(this->CreateAesCtrExStorage(out, option, base_storage));
                break;
            default:
                return fs::ResultInvalidNcaFsHeaderEncryptionType();
        }

        return ResultSuccess();
    }

    Result NcaFileSystemDriver::CreateAesXtsStorage(std::unique_ptr<fs::IStorage> *out, BaseStorage *base_storage) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);

        /* Create the iv. */
        u8 iv[AesXtsStorage::IvSize] = {};
        MakeAesXtsIv(iv, base_storage->GetStorageOffset());

        /* Allocate a new raw storage. */
        std::unique_ptr<fs::IStorage> raw_storage = base_storage->MakeStorage();
        R_UNLESS(raw_storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Make the aes xts storage. */
        const auto *key1 = this->reader->GetDecryptionKey(NcaHeader::DecryptionKey_AesXts1);
        const auto *key2 = this->reader->GetDecryptionKey(NcaHeader::DecryptionKey_AesXts2);
        std::unique_ptr xts_storage = std::make_unique<AesXtsStorage>(raw_storage.get(), key1, key2, AesXtsStorage::KeySize, iv, AesXtsStorage::IvSize, NcaHeader::XtsBlockSize);
        R_UNLESS(xts_storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Make the out storage. */
        std::unique_ptr storage = std::make_unique<DerivedStorageHolder<AlignmentMatchingStorage<NcaHeader::XtsBlockSize, 1>, 2>>(xts_storage.get(), this->reader);
        R_UNLESS(storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Set the substorages. */
        storage->Set(std::move(raw_storage), std::move(xts_storage));

        /* Set the output. */
        *out = std::move(storage);
        return ResultSuccess();
    }

    Result NcaFileSystemDriver::CreateAesCtrStorage(std::unique_ptr<fs::IStorage> *out, BaseStorage *base_storage) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);

        /* Create the iv. */
        u8 iv[AesCtrStorage::IvSize] = {};
        AesCtrStorage::MakeIv(iv, sizeof(iv), base_storage->GetAesCtrUpperIv().value, base_storage->GetStorageOffset());

        /* Create the raw storage. */
        std::unique_ptr raw_storage = base_storage->MakeStorage();

        /* Create the decrypt storage. */
        const bool has_external_key = reader->HasExternalDecryptionKey();
        std::unique_ptr<fs::IStorage> decrypt_storage;
        if (has_external_key) {
            decrypt_storage = std::make_unique<AesCtrStorageExternal>(raw_storage.get(), this->reader->GetExternalDecryptionKey(), AesCtrStorageExternal::KeySize, iv, AesCtrStorageExternal::IvSize, this->reader->GetExternalDecryptAesCtrFunctionForExternalKey(), -1);
            R_UNLESS(decrypt_storage != nullptr, fs::ResultAllocationFailureInNew());
        } else {
            /* Check if we have a hardware key. */
            const bool has_hardware_key = this->reader->HasInternalDecryptionKeyForAesHardwareSpeedEmulation();

            /* Create the software decryption storage. */
            std::unique_ptr<fs::IStorage> aes_ctr_sw_storage = std::make_unique<AesCtrStorage>(raw_storage.get(), this->reader->GetDecryptionKey(NcaHeader::DecryptionKey_AesCtr), AesCtrStorage::KeySize, iv, AesCtrStorage::IvSize);
            R_UNLESS(aes_ctr_sw_storage != nullptr, fs::ResultAllocationFailureInNew());

            /* If we have a hardware key and should use it, make the hardware decryption storage. */
            if (has_hardware_key && !this->reader->IsSoftwareAesPrioritized()) {
                std::unique_ptr<fs::IStorage> aes_ctr_hw_storage = std::make_unique<AesCtrStorageExternal>(raw_storage.get(), this->reader->GetDecryptionKey(NcaHeader::DecryptionKey_AesCtrHw), AesCtrStorageExternal::KeySize, iv, AesCtrStorageExternal::IvSize, this->reader->GetExternalDecryptAesCtrFunction(), GetKeyTypeValue(this->reader->GetKeyIndex(), this->reader->GetKeyGeneration()));
                R_UNLESS(aes_ctr_hw_storage != nullptr, fs::ResultAllocationFailureInNew());

                /* Create the selection storage. */
                decrypt_storage = std::make_unique<SwitchStorage<bool (*)()>>(std::move(aes_ctr_hw_storage), std::move(aes_ctr_sw_storage), IsUsingHardwareAesCtrForSpeedEmulation);
                R_UNLESS(decrypt_storage != nullptr, fs::ResultAllocationFailureInNew());
            } else {
                /* Otherwise, just use the software decryption storage. */
                decrypt_storage = std::move(aes_ctr_sw_storage);
            }
        }

        /* Create the storage holder. */
        std::unique_ptr storage = std::make_unique<DerivedStorageHolder<AlignmentMatchingStorage<NcaHeader::CtrBlockSize, 1>, 2>>(decrypt_storage.get(), this->reader);
        R_UNLESS(storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Set the storage holder's storages. */
        storage->Set(std::move(raw_storage), std::move(decrypt_storage));

        /* Set the out storage. */
        *out = std::move(storage);
        return ResultSuccess();
    }

    Result NcaFileSystemDriver::CreateAesCtrExStorage(std::unique_ptr<fs::IStorage> *out, StorageOption *option, BaseStorage *base_storage) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(option != nullptr);
        AMS_ASSERT(base_storage != nullptr);

        /* Check if indirection is needed. */
        const auto &header_reader = option->GetHeaderReader();
        const auto &patch_info    = header_reader.GetPatchInfo();

        /* Read the bucket tree header. */
        BucketTree::Header header;
        std::memcpy(std::addressof(header), patch_info.aes_ctr_ex_header, sizeof(header));
        R_TRY(header.Verify());

        /* Validate patch info extents. */
        R_UNLESS(patch_info.indirect_size   > 0, fs::ResultInvalidNcaPatchInfoIndirectSize());
        R_UNLESS(patch_info.aes_ctr_ex_size > 0, fs::ResultInvalidNcaPatchInfoAesCtrExSize());

        /* Make new base storage. */
        const auto base_storage_offset  = base_storage->GetStorageOffset();
        const auto base_storage_size    = util::AlignUp(patch_info.aes_ctr_ex_offset + patch_info.aes_ctr_ex_size, NcaHeader::XtsBlockSize);
        fs::SubStorage new_base_storage;
        R_TRY(base_storage->GetSubStorage(std::addressof(new_base_storage), 0, base_storage_size));

        /* Create the table storage. */
        std::unique_ptr<fs::IStorage> table_storage;
        {
            BaseStorage aes_ctr_base_storage(std::addressof(new_base_storage), patch_info.aes_ctr_ex_offset, patch_info.aes_ctr_ex_size);
            aes_ctr_base_storage.SetStorageOffset(base_storage_offset + patch_info.aes_ctr_ex_offset);
            aes_ctr_base_storage.SetAesCtrUpperIv(header_reader.GetAesCtrUpperIv());
            R_TRY(this->CreateAesCtrStorage(std::addressof(table_storage), std::addressof(aes_ctr_base_storage)));
        }

        /* Get the table size. */
        s64 table_size = 0;
        R_TRY(table_storage->GetSize(std::addressof(table_size)));

        /* Create the buffered storage. */
        std::unique_ptr buffered_storage = std::make_unique<save::BufferedStorage>();
        R_UNLESS(buffered_storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Initialize the buffered storage. */
        R_TRY(buffered_storage->Initialize(fs::SubStorage(table_storage.get(), 0, table_size), this->buffer_manager, AesCtrExTableCacheBlockSize, AesCtrExTableCacheCount));

        /* Create an aligned storage for the buffered storage. */
        using AlignedStorage = AlignmentMatchingStorage<NcaHeader::CtrBlockSize, 1>;
        std::unique_ptr aligned_storage = std::make_unique<AlignedStorage>(buffered_storage.get());
        R_UNLESS(aligned_storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Determine the bucket extents. */
        const auto entry_count = header.entry_count;
        const s64 data_offset  = 0;
        const s64 data_size    = patch_info.aes_ctr_ex_offset;
        const s64 node_offset  = 0;
        const s64 node_size    = AesCtrCounterExtendedStorage::QueryNodeStorageSize(entry_count);
        const s64 entry_offset = node_offset + node_size;
        const s64 entry_size   = AesCtrCounterExtendedStorage::QueryEntryStorageSize(entry_count);

        /* Create bucket storages. */
        fs::SubStorage data_storage(std::addressof(new_base_storage), data_offset, data_size);
        fs::SubStorage node_storage(aligned_storage.get(), node_offset, node_size);
        fs::SubStorage entry_storage(aligned_storage.get(), entry_offset, entry_size);

        /* Get the secure value. */
        const auto secure_value = header_reader.GetAesCtrUpperIv().part.secure_value;

        /* Create the aes ctr ex storage. */
        std::unique_ptr<fs::IStorage> aes_ctr_ex_storage;
        const bool has_external_key = this->reader->HasExternalDecryptionKey();
        if (has_external_key) {
            /* Create the decryptor. */
            std::unique_ptr<AesCtrCounterExtendedStorage::IDecryptor> decryptor;
            R_TRY(AesCtrCounterExtendedStorage::CreateExternalDecryptor(std::addressof(decryptor), this->reader->GetExternalDecryptAesCtrFunctionForExternalKey(), -1));

            /* Create the aes ctr ex storage. */
            std::unique_ptr impl_storage = std::make_unique<AesCtrCounterExtendedStorage>();
            R_UNLESS(impl_storage != nullptr, fs::ResultAllocationFailureInNew());

            /* Initialize the aes ctr ex storage. */
            R_TRY(impl_storage->Initialize(this->allocator, this->reader->GetExternalDecryptionKey(), AesCtrStorage::KeySize, secure_value, base_storage_offset, data_storage, node_storage, entry_storage, entry_count, std::move(decryptor)));

            /* Set the option's aes ctr ex storage. */
            option->SetAesCtrExStorageRaw(impl_storage.get());

            aes_ctr_ex_storage = std::move(impl_storage);
        } else {
            /* Check if we have a hardware key. */
            const bool has_hardware_key = this->reader->HasInternalDecryptionKeyForAesHardwareSpeedEmulation();

            /* Create the software decryptor. */
            std::unique_ptr<AesCtrCounterExtendedStorage::IDecryptor> sw_decryptor;
            R_TRY(AesCtrCounterExtendedStorage::CreateSoftwareDecryptor(std::addressof(sw_decryptor)));

            /* Make the software storage. */
            std::unique_ptr sw_storage = std::make_unique<AesCtrCounterExtendedStorage>();
            R_UNLESS(sw_storage != nullptr, fs::ResultAllocationFailureInNew());

            /* Initialize the software storage. */
            R_TRY(sw_storage->Initialize(this->allocator, this->reader->GetDecryptionKey(NcaHeader::DecryptionKey_AesCtr), AesCtrStorage::KeySize, secure_value, base_storage_offset, data_storage, node_storage, entry_storage, entry_count, std::move(sw_decryptor)));

            /* Set the option's aes ctr ex storage. */
            option->SetAesCtrExStorageRaw(sw_storage.get());

            /* If we have a hardware key and should use it, make the hardware decryption storage. */
            if (has_hardware_key && !this->reader->IsSoftwareAesPrioritized()) {
                /* Create the hardware decryptor. */
                std::unique_ptr<AesCtrCounterExtendedStorage::IDecryptor> hw_decryptor;
                R_TRY(AesCtrCounterExtendedStorage::CreateExternalDecryptor(std::addressof(hw_decryptor), this->reader->GetExternalDecryptAesCtrFunction(), GetKeyTypeValue(this->reader->GetKeyIndex(), this->reader->GetKeyGeneration())));

                /* Create the hardware storage. */
                std::unique_ptr hw_storage = std::make_unique<AesCtrCounterExtendedStorage>();
                R_UNLESS(hw_storage != nullptr, fs::ResultAllocationFailureInNew());

                /* Initialize the hardware storage. */
                R_TRY(hw_storage->Initialize(this->allocator, this->reader->GetDecryptionKey(NcaHeader::DecryptionKey_AesCtrHw), AesCtrStorage::KeySize, secure_value, base_storage_offset, data_storage, node_storage, entry_storage, entry_count, std::move(hw_decryptor)));

                /* Create the selection storage. */
                std::unique_ptr switch_storage = std::make_unique<SwitchStorage<bool (*)()>>(std::move(hw_storage), std::move(sw_storage), IsUsingHardwareAesCtrForSpeedEmulation);
                R_UNLESS(switch_storage != nullptr, fs::ResultAllocationFailureInNew());

                /* Set the aes ctr ex storage. */
                aes_ctr_ex_storage = std::move(switch_storage);
            } else {
                /* Set the aes ctr ex storage. */
                aes_ctr_ex_storage = std::move(sw_storage);
            }
        }

        /* Create the storage holder. */
        std::unique_ptr storage = std::make_unique<DerivedStorageHolder<AlignedStorage, 5>>(aes_ctr_ex_storage.get(), this->reader);
        R_UNLESS(storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Set the aes ctr ex storages in the option. */
        option->SetAesCtrExTableStorage(table_storage.get());
        option->SetAesCtrExStorage(storage.get());

        /* Set the storage holder's storages. */
        storage->Set(std::move(base_storage->GetStorage()), std::move(table_storage), std::move(buffered_storage), std::move(aligned_storage), std::move(aes_ctr_ex_storage));

        /* Set the out storage. */
        *out = std::move(storage);
        return ResultSuccess();
    }

    Result NcaFileSystemDriver::CreateIndirectStorage(std::unique_ptr<fs::IStorage> *out, StorageOption *option, std::unique_ptr<fs::IStorage> base_storage) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(option != nullptr);
        AMS_ASSERT(base_storage != nullptr);

        /* Check if indirection is needed. */
        const auto &header_reader = option->GetHeaderReader();
        const auto &patch_info    = header_reader.GetPatchInfo();

        if (!patch_info.HasIndirectTable()) {
            *out = std::move(base_storage);
            return ResultSuccess();
        }

        /* Read the bucket tree header. */
        BucketTree::Header header;
        std::memcpy(std::addressof(header), patch_info.indirect_header, sizeof(header));
        R_TRY(header.Verify());

        /* Determine the storage sizes. */
        const auto node_size  = IndirectStorage::QueryNodeStorageSize(header.entry_count);
        const auto entry_size = IndirectStorage::QueryEntryStorageSize(header.entry_count);
        R_UNLESS(node_size + entry_size <= patch_info.indirect_size, fs::ResultInvalidIndirectStorageSize());

        /* Open the original storage. */
        std::unique_ptr<fs::IStorage> original_storage;
        {
            const s32 fs_index = header_reader.GetFsIndex();

            if (this->original_reader != nullptr && this->original_reader->HasFsInfo(fs_index)) {
                NcaFsHeaderReader original_header_reader;
                R_TRY(original_header_reader.Initialize(*this->original_reader, fs_index));

                NcaFileSystemDriver original_driver(this->original_reader, this->allocator, this->buffer_manager);
                StorageOption original_option(std::addressof(original_header_reader), fs_index);

                BaseStorage original_base_storage;
                R_TRY(original_driver.CreateBaseStorage(std::addressof(original_base_storage), std::addressof(original_option)));
                R_TRY(original_driver.CreateDecryptableStorage(std::addressof(original_storage), std::addressof(original_option), std::addressof(original_base_storage)));
            } else {
                original_storage = std::make_unique<fs::MemoryStorage>(nullptr, 0);
                R_UNLESS(original_storage != nullptr, fs::ResultAllocationFailureInNew());
            }
        }

        /* Get the original data size. */
        s64 original_data_size = 0;
        R_TRY(original_storage->GetSize(std::addressof(original_data_size)));

        /* Get the indirect data size. */
        s64 indirect_data_size = patch_info.indirect_offset;
        AMS_ASSERT(util::IsAligned(indirect_data_size, NcaHeader::XtsBlockSize));

        /* Create the indirect table storage. */
        std::unique_ptr indirect_table_storage = std::make_unique<save::BufferedStorage>();
        R_UNLESS(indirect_table_storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Initialize the indirect table storage. */
        R_TRY(indirect_table_storage->Initialize(fs::SubStorage(base_storage.get(), indirect_data_size, node_size + entry_size), this->buffer_manager, IndirectTableCacheBlockSize, IndirectTableCacheCount));

        /* Create the indirect data storage. */
        std::unique_ptr indirect_data_storage = std::make_unique<save::BufferedStorage>();
        R_UNLESS(indirect_data_storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Initialize the indirect data storage. */
        R_TRY(indirect_data_storage->Initialize(fs::SubStorage(base_storage.get(), 0, indirect_data_size), this->buffer_manager, IndirectDataCacheBlockSize, IndirectDataCacheCount));

        /* Create the storage holder. */
        std::unique_ptr storage = std::make_unique<DerivedStorageHolder<IndirectStorage, 4>>(this->reader);
        R_UNLESS(storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Initialize the storage holder. */
        R_TRY(storage->Initialize(this->allocator, fs::SubStorage(indirect_table_storage.get(), 0, node_size), fs::SubStorage(indirect_table_storage.get(), node_size, entry_size), header.entry_count));

        /* Set the storage holder's storages. */
        storage->SetStorage(0, original_storage.get(), 0, original_data_size);
        storage->SetStorage(1, indirect_data_storage.get(), 0, indirect_data_size);
        storage->Set(std::move(base_storage), std::move(original_storage), std::move(indirect_table_storage), std::move(indirect_data_storage));

        /* Set the indirect storage. */
        option->SetIndirectStorage(storage.get());

        /* Set the out storage. */
        *out = std::move(storage);
        return ResultSuccess();
    }

    Result NcaFileSystemDriver::CreateVerificationStorage(std::unique_ptr<fs::IStorage> *out, std::unique_ptr<fs::IStorage> base_storage, NcaFsHeaderReader *header_reader) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(header_reader != nullptr);

        /* Create the appropriate storage for the encryption type. */
        switch (header_reader->GetHashType()) {
            case NcaFsHeader::HashType::HierarchicalSha256Hash:
                R_TRY(this->CreateSha256Storage(out, std::move(base_storage), header_reader));
                break;
            case NcaFsHeader::HashType::HierarchicalIntegrityHash:
                R_TRY(this->CreateIntegrityVerificationStorage(out, std::move(base_storage), header_reader));
                break;
            default:
                return fs::ResultInvalidNcaFsHeaderHashType();
        }

        return ResultSuccess();
    }

    Result NcaFileSystemDriver::CreateSha256Storage(std::unique_ptr<fs::IStorage> *out, std::unique_ptr<fs::IStorage> base_storage, NcaFsHeaderReader *header_reader) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(header_reader != nullptr);

        /* Define storage types. */
        using VerificationStorage = HierarchicalSha256Storage;
        using CacheStorage        = ReadOnlyBlockCacheStorage;
        using AlignedStorage      = AlignmentMatchingStoragePooledBuffer<1>;
        using StorageHolder       = DerivedStorageHolderWithBuffer<AlignedStorage, 4>;

        /* Get and validate the hash data. */
        auto &hash_data = header_reader->GetHashData().hierarchical_sha256_data;
        R_UNLESS(util::IsPowerOfTwo(hash_data.hash_block_size),                           fs::ResultInvalidHierarchicalSha256BlockSize());
        R_UNLESS(hash_data.hash_layer_count == HierarchicalSha256Storage::LayerCount - 1, fs::ResultInvalidHierarchicalSha256LayerCount());

        /* Get the regions. */
        const auto &hash_region = hash_data.hash_layer_region[0];
        const auto &data_region = hash_data.hash_layer_region[1];

        /* Determine buffer sizes. */
        constexpr s32 CacheBlockCount = 2;
        const auto hash_buffer_size  = static_cast<size_t>(hash_region.size);
        const auto cache_buffer_size = CacheBlockCount * hash_data.hash_block_size;
        const auto total_buffer_size = hash_buffer_size + cache_buffer_size;

        /* Make a buffer holder. */
        BufferHolder buffer_holder(this->allocator, total_buffer_size);
        R_UNLESS(buffer_holder.IsValid(), fs::ResultAllocationFailureInNcaFileSystemDriverI());

        /* Make the data storage. */
        std::unique_ptr data_storage = std::make_unique<fs::SubStorage>(base_storage.get(), data_region.offset, data_region.size);
        R_UNLESS(data_storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Make the verification storage. */
        std::unique_ptr verification_storage = std::make_unique<VerificationStorage>();
        R_UNLESS(verification_storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Make layer storages. */
        fs::MemoryStorage master_hash_storage(std::addressof(hash_data.fs_data_master_hash), sizeof(Hash));
        fs::SubStorage layer_hash_storage(base_storage.get(), hash_region.offset, hash_region.size);
        fs::IStorage *storages[VerificationStorage::LayerCount] = {
            std::addressof(master_hash_storage),
            std::addressof(layer_hash_storage),
            data_storage.get()
        };

        /* Initialize the verification storage. */
        R_TRY(verification_storage->Initialize(storages, VerificationStorage::LayerCount, hash_data.hash_block_size, buffer_holder.Get(), hash_buffer_size));

        /* Make the cache storage. */
        std::unique_ptr cache_storage = std::make_unique<CacheStorage>(verification_storage.get(), hash_data.hash_block_size, buffer_holder.Get() + hash_buffer_size, cache_buffer_size, CacheBlockCount);
        R_UNLESS(cache_storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Make the storage holder. */
        std::unique_ptr storage = std::make_unique<StorageHolder>(cache_storage.get(), hash_data.hash_block_size, this->reader);
        R_UNLESS(storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Set the storage holder's data. */
        storage->Set(std::move(base_storage), std::move(data_storage), std::move(verification_storage), std::move(cache_storage));
        storage->Set(std::move(buffer_holder));

        /* Set the output. */
        *out = std::move(storage);
        return ResultSuccess();
    }

    Result NcaFileSystemDriver::CreateIntegrityVerificationStorage(std::unique_ptr<fs::IStorage> *out, std::unique_ptr<fs::IStorage> base_storage, NcaFsHeaderReader *header_reader) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(base_storage != nullptr);
        AMS_ASSERT(header_reader != nullptr);

        /* Define storage types. */
        using VerificationStorage = save::HierarchicalIntegrityVerificationStorage;
        using StorageInfo         = VerificationStorage::HierarchicalStorageInformation;
        using StorageHolder       = DerivedStorageHolder<IntegrityRomFsStorage, 1>;

        /* Get and validate the hash data. */
        auto &hash_data = header_reader->GetHashData().integrity_meta_info;
        save::HierarchicalIntegrityVerificationInformation level_hash_info;
        std::memcpy(std::addressof(level_hash_info), std::addressof(hash_data.level_hash_info), sizeof(level_hash_info));

        R_UNLESS(save::IntegrityMinLayerCount <= level_hash_info.max_layers,   fs::ResultInvalidHierarchicalIntegrityVerificationLayerCount());
        R_UNLESS(level_hash_info.max_layers   <= save::IntegrityMaxLayerCount, fs::ResultInvalidHierarchicalIntegrityVerificationLayerCount());

        /* Create storage info. */
        StorageInfo storage_info;
        for (s32 i = 0; i < static_cast<s32>(level_hash_info.max_layers - 2); ++i) {
            const auto &layer_info = level_hash_info.info[i];
            storage_info[i + 1] = fs::SubStorage(base_storage.get(), layer_info.offset, layer_info.size);
        }

        /* Set the last layer info. */
        const auto &layer_info = level_hash_info.info[level_hash_info.max_layers - 2];
        storage_info.SetDataStorage(fs::SubStorage(base_storage.get(), layer_info.offset, layer_info.size));

        /* Make the storage holder. */
        std::unique_ptr storage = std::make_unique<StorageHolder>(this->reader);
        R_UNLESS(storage != nullptr, fs::ResultAllocationFailureInNew());

        /* Initialize the integrity storage. */
        R_TRY(storage->Initialize(level_hash_info, hash_data.master_hash, storage_info, this->buffer_manager));

        /* Set the storage holder's data. */
        storage->Set(std::move(base_storage));

        /* Set the output. */
        *out = std::move(storage);
        return ResultSuccess();
    }

    Result NcaFileSystemDriver::SetupFsHeaderReader(NcaFsHeaderReader *out, const NcaReader &reader, s32 fs_index) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(0 <= fs_index && fs_index < NcaHeader::FsCountMax);

        /* Validate magic. */
        R_UNLESS(reader.GetMagic() == NcaHeader::Magic, fs::ResultUnsupportedVersion());

        /* Check that the fs header exists. */
        R_UNLESS(reader.HasFsInfo(fs_index), fs::ResultPartitionNotFound());

        /* Initialize the reader. */
        R_TRY(out->Initialize(reader, fs_index));

        return ResultSuccess();
    }

}
