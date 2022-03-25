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

        constexpr inline u32 IntegrityVerificationStorageMagic       = util::FourCC<'I','V','F','C'>::Code;
        constexpr inline u32 IntegrityVerificationStorageVersion     = 0x00020000;
        constexpr inline u32 IntegrityVerificationStorageVersionMask = 0xFFFF0000;

        constexpr inline auto AccessCountMax = 5;
        constexpr inline auto AccessTimeout  = TimeSpan::FromMilliSeconds(10);

        os::Semaphore g_read_semaphore(AccessCountMax, AccessCountMax);
        os::Semaphore g_write_semaphore(AccessCountMax, AccessCountMax);

        constexpr inline const char MasterKey[] = "HierarchicalIntegrityVerificationStorage::Master";
        constexpr inline const char L1Key[]     = "HierarchicalIntegrityVerificationStorage::L1";
        constexpr inline const char L2Key[]     = "HierarchicalIntegrityVerificationStorage::L2";
        constexpr inline const char L3Key[]     = "HierarchicalIntegrityVerificationStorage::L3";
        constexpr inline const char L4Key[]     = "HierarchicalIntegrityVerificationStorage::L4";
        constexpr inline const char L5Key[]     = "HierarchicalIntegrityVerificationStorage::L5";

        constexpr inline const struct {
            const char *key;
            size_t size;
        } KeyArray[] = {
            { MasterKey, sizeof(MasterKey) },
            { L1Key,     sizeof(L1Key)     },
            { L2Key,     sizeof(L2Key)     },
            { L3Key,     sizeof(L3Key)     },
            { L4Key,     sizeof(L4Key)     },
            { L5Key,     sizeof(L5Key)     },
        };

    }

    /* Instantiate the global random generation function. */
    constinit HierarchicalIntegrityVerificationStorage::GenerateRandomFunction HierarchicalIntegrityVerificationStorage::s_generate_random = nullptr;

    Result HierarchicalIntegrityVerificationStorageControlArea::QuerySize(HierarchicalIntegrityVerificationSizeSet *out, const InputParam &input_param, s32 layer_count, s64 data_size) {
        /* Validate preconditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT((static_cast<s32>(IntegrityMinLayerCount) <= layer_count) && (layer_count <= static_cast<s32>(IntegrityMaxLayerCount)));
        for (s32 level = 0; level < (layer_count - 1); ++level) {
            AMS_ASSERT(input_param.level_block_size[level] > 0);
            AMS_ASSERT(IsPowerOfTwo(static_cast<s32>(input_param.level_block_size[level])));
        }

        /* Set the control size. */
        out->control_size = sizeof(HierarchicalIntegrityVerificationMetaInformation);

        /* Determine the level sizes. */
        s64 level_size[IntegrityMaxLayerCount];
        s32 level = layer_count - 1;

        level_size[level] = util::AlignUp(data_size, input_param.level_block_size[level - 1]);
        --level;

        for (/* ... */; level > 0; --level) {
            level_size[level] = util::AlignUp(level_size[level + 1] / input_param.level_block_size[level] * HashSize, input_param.level_block_size[level - 1]);
        }

        /* Determine the master size. */
        level_size[0] = level_size[1] / input_param.level_block_size[0] * HashSize;

        /* Set the master size. */
        out->master_hash_size = level_size[0];

        /* Set the level sizes. */
        for (level = 1; level < layer_count - 1; ++level) {
            out->layered_hash_sizes[level - 1] = level_size[level];
        }

        R_SUCCEED();
    }

    Result HierarchicalIntegrityVerificationStorageControlArea::Expand(fs::SubStorage meta_storage, const HierarchicalIntegrityVerificationMetaInformation &meta) {
        /* Check the meta size. */
        {
            s64 meta_size = 0;
            R_TRY(meta_storage.GetSize(std::addressof(meta_size)));
            R_UNLESS(meta_size >= static_cast<s64>(sizeof(meta)), fs::ResultInvalidSize());
        }

        /* Validate both the previous and new metas. */
        {
            /* Read the previous meta. */
            HierarchicalIntegrityVerificationMetaInformation prev_meta = {};
            R_TRY(meta_storage.Read(0, std::addressof(prev_meta), sizeof(prev_meta)));

            /* Validate both magics. */
            R_UNLESS(prev_meta.magic == IntegrityVerificationStorageMagic, fs::ResultIncorrectIntegrityVerificationMagic());
            R_UNLESS(prev_meta.magic == meta.magic,                        fs::ResultIncorrectIntegrityVerificationMagic());

            /* Validate both versions. */
            R_UNLESS(prev_meta.version == IntegrityVerificationStorageVersion, fs::ResultUnsupportedVersion());
            R_UNLESS(prev_meta.version == meta.version,                        fs::ResultUnsupportedVersion());
        }

        /* Write the new meta. */
        R_TRY(meta_storage.Write(0, std::addressof(meta), sizeof(meta)));
        R_TRY(meta_storage.Flush());

        R_SUCCEED();
    }

    Result HierarchicalIntegrityVerificationStorageControlArea::Initialize(fs::SubStorage meta_storage) {
        /* Check the meta size. */
        {
            s64 meta_size = 0;
            R_TRY(meta_storage.GetSize(std::addressof(meta_size)));
            R_UNLESS(meta_size >= static_cast<s64>(sizeof(m_meta)), fs::ResultInvalidSize());
        }

        /* Set the storage and read the meta. */
        m_storage = meta_storage;
        R_TRY(m_storage.Read(0, std::addressof(m_meta), sizeof(m_meta)));

        /* Validate the meta magic. */
        R_UNLESS(m_meta.magic == IntegrityVerificationStorageMagic, fs::ResultIncorrectIntegrityVerificationMagic());

        /* Validate the meta version. */
        R_UNLESS((m_meta.version & IntegrityVerificationStorageVersionMask) == (IntegrityVerificationStorageVersion & IntegrityVerificationStorageVersionMask), fs::ResultUnsupportedVersion());

        R_SUCCEED();
    }

    void HierarchicalIntegrityVerificationStorageControlArea::Finalize() {
        m_storage = fs::SubStorage();
    }

    Result HierarchicalIntegrityVerificationStorage::Initialize(const HierarchicalIntegrityVerificationInformation &info, HierarchicalStorageInformation storage, FileSystemBufferManagerSet *bufs, IHash256GeneratorFactory *hgf, bool hash_salt_enabled, os::SdkRecursiveMutex *mtx, os::Semaphore *read_sema, os::Semaphore *write_sema, int max_data_cache_entries, int max_hash_cache_entries, s8 buffer_level, bool is_writable, bool allow_cleared_blocks) {
        /* Validate preconditions. */
        AMS_ASSERT(bufs != nullptr);
        AMS_ASSERT(IntegrityMinLayerCount <= info.max_layers && info.max_layers <= IntegrityMaxLayerCount);

        /* Set member variables. */
        m_max_layers      = info.max_layers;
        m_buffers         = bufs;
        m_mutex           = mtx;
        m_read_semaphore  = read_sema;
        m_write_semaphore = write_sema;

        /* If hash salt is enabled, generate it. */
        util::optional<fs::HashSalt> hash_salt = util::nullopt;
        if (hash_salt_enabled) {
            hash_salt.emplace();
            crypto::GenerateHmacSha256(hash_salt->value, sizeof(hash_salt->value), info.seed.value, sizeof(info.seed), KeyArray[0].key, KeyArray[0].size);
        }

        /* Initialize the top level verification storage. */
        m_verify_storages[0].Initialize(storage[HierarchicalStorageInformation::MasterStorage], storage[HierarchicalStorageInformation::Layer1Storage], static_cast<s64>(1) << info.info[0].block_order, HashSize, m_buffers->buffers[m_max_layers - 2], hgf, hash_salt, false, is_writable, allow_cleared_blocks);

        /* Ensure we don't leak state if further initialization goes wrong. */
        ON_RESULT_FAILURE {
            m_verify_storages[0].Finalize();

            m_data_size = -1;
            m_buffers   = nullptr;
            m_mutex     = nullptr;
        };

        /* Initialize the top level buffer storage. */
        R_TRY(m_buffer_storages[0].Initialize(m_buffers->buffers[0], m_mutex, std::addressof(m_verify_storages[0]), info.info[0].size, static_cast<s64>(1) << info.info[0].block_order, max_hash_cache_entries, false, 0x10, false, is_writable));
        ON_RESULT_FAILURE_2 { m_buffer_storages[0].Finalize(); };

        /* Prepare to initialize the level storages. */
        s32 level = 0;

        /* Ensure we don't leak state if further initialization goes wrong. */
        ON_RESULT_FAILURE_2 {
            m_verify_storages[level + 1].Finalize();
            for (/* ... */; level > 0; --level) {
                m_buffer_storages[level].Finalize();
                m_verify_storages[level].Finalize();
            }
        };

        /* Initialize the level storages. */
        for (/* ... */; level < m_max_layers - 3; ++level) {
            /* If hash salt is enabled, generate it. */
            util::optional<fs::HashSalt> hash_salt = util::nullopt;
            if (hash_salt_enabled) {
                hash_salt.emplace();
                crypto::GenerateHmacSha256(hash_salt->value, sizeof(hash_salt->value), info.seed.value, sizeof(info.seed), KeyArray[level + 1].key, KeyArray[level + 1].size);
            }

            /* Initialize the verification storage. */
            fs::SubStorage buffer_storage(std::addressof(m_buffer_storages[level]), 0, info.info[level].size);
            m_verify_storages[level + 1].Initialize(buffer_storage, storage[level + 2], static_cast<s64>(1) << info.info[level + 1].block_order, static_cast<s64>(1) << info.info[level].block_order, m_buffers->buffers[m_max_layers - 2], hgf, hash_salt, false, is_writable, allow_cleared_blocks);

            /* Initialize the buffer storage. */
            R_TRY(m_buffer_storages[level + 1].Initialize(m_buffers->buffers[level + 1], m_mutex, std::addressof(m_verify_storages[level + 1]), info.info[level + 1].size, static_cast<s64>(1) << info.info[level + 1].block_order, max_hash_cache_entries, false, 0x11 + static_cast<s8>(level), false, is_writable));
        }

        /* Initialize the final level storage. */
        {
            /* If hash salt is enabled, generate it. */
            util::optional<fs::HashSalt> hash_salt = util::nullopt;
            if (hash_salt_enabled) {
                hash_salt.emplace();
                crypto::GenerateHmacSha256(hash_salt->value, sizeof(hash_salt->value), info.seed.value, sizeof(info.seed), KeyArray[level + 1].key, KeyArray[level + 1].size);
            }

            /* Initialize the verification storage. */
            fs::SubStorage buffer_storage(std::addressof(m_buffer_storages[level]), 0, info.info[level].size);
            m_verify_storages[level + 1].Initialize(buffer_storage, storage[level + 2], static_cast<s64>(1) << info.info[level + 1].block_order, static_cast<s64>(1) << info.info[level].block_order, m_buffers->buffers[m_max_layers - 2], hgf, hash_salt, true, is_writable, allow_cleared_blocks);

            /* Initialize the buffer storage. */
            R_TRY(m_buffer_storages[level + 1].Initialize(m_buffers->buffers[level + 1], m_mutex, std::addressof(m_verify_storages[level + 1]), info.info[level + 1].size, static_cast<s64>(1) << info.info[level + 1].block_order, max_data_cache_entries, true, buffer_level, true, is_writable));
        }

        /* Set the data size. */
        m_data_size = info.info[level + 1].size;

        /* We succeeded. */
        R_SUCCEED();
    }

    void HierarchicalIntegrityVerificationStorage::Finalize() {
        if (m_data_size >= 0) {
            m_data_size = 0;

            m_buffers   = nullptr;
            m_mutex     = nullptr;

            for (s32 level = m_max_layers - 2; level >= 0; --level) {
                m_buffer_storages[level].Finalize();
                m_verify_storages[level].Finalize();
            }

            m_data_size = -1;
        }
    }

    Result HierarchicalIntegrityVerificationStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Validate preconditions. */
        AMS_ASSERT(m_data_size >= 0);

        /* Succeed if zero-size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* If we have a read semaphore, acquire it. */
        if (m_read_semaphore != nullptr) { m_read_semaphore->Acquire(); }
        ON_SCOPE_EXIT { if (m_read_semaphore != nullptr) { m_read_semaphore->Release(); } };

        /* Acquire access to the global read semaphore. */
        if (!g_read_semaphore.TimedAcquire(AccessTimeout)) {
            for (auto level = m_max_layers - 2; level >= 0; --level) {
                R_TRY(m_buffer_storages[level].Flush());
            }
            g_read_semaphore.Acquire();
        }

        /* Ensure that we release the semaphore when done. */
        ON_SCOPE_EXIT { g_read_semaphore.Release(); };

        /* Read the data. */
        R_RETURN(m_buffer_storages[m_max_layers - 2].Read(offset, buffer, size));
    }

    Result HierarchicalIntegrityVerificationStorage::Write(s64 offset, const void *buffer, size_t size) {
        /* Validate preconditions. */
        AMS_ASSERT(m_data_size >= 0);

        /* Succeed if zero-size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* If we have a write semaphore, acquire it. */
        if (m_write_semaphore != nullptr) { m_write_semaphore->Acquire(); }
        ON_SCOPE_EXIT { if (m_write_semaphore != nullptr) { m_write_semaphore->Release(); } };

        /* Acquire access to the write semaphore. */
        if (!g_write_semaphore.TimedAcquire(AccessTimeout)) {
            for (auto level = m_max_layers - 2; level >= 0; --level) {
                R_TRY(m_buffer_storages[level].Flush());
            }
            g_write_semaphore.Acquire();
        }

        /* Ensure that we release the semaphore when done. */
        ON_SCOPE_EXIT { g_write_semaphore.Release(); };

        /* Write the data. */
        R_RETURN(m_buffer_storages[m_max_layers - 2].Write(offset, buffer, size));
    }

    Result HierarchicalIntegrityVerificationStorage::GetSize(s64 *out) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(m_data_size >= 0);
        *out = m_data_size;
        R_SUCCEED();
    }

    Result HierarchicalIntegrityVerificationStorage::Flush() {
        R_SUCCEED();
    }

    Result HierarchicalIntegrityVerificationStorage::OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        switch (op_id) {
            case fs::OperationId::FillZero:
            case fs::OperationId::DestroySignature:
                {
                    R_TRY(m_buffer_storages[m_max_layers - 2].OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                    R_SUCCEED();
                }
            case fs::OperationId::Invalidate:
            case fs::OperationId::QueryRange:
                {
                    R_TRY(m_buffer_storages[m_max_layers - 2].OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                    R_SUCCEED();
                }
            default:
                R_THROW(fs::ResultUnsupportedOperateRangeForHierarchicalIntegrityVerificationStorage());
        }
    }

    Result HierarchicalIntegrityVerificationStorage::Commit() {
        for (s32 level = m_max_layers - 2; level >= 0; --level) {
            R_TRY(m_buffer_storages[level].Commit());
        }
        R_SUCCEED();
    }

    Result HierarchicalIntegrityVerificationStorage::OnRollback() {
        for (s32 level = m_max_layers - 2; level >= 0; --level) {
            R_TRY(m_buffer_storages[level].OnRollback());
        }
        R_SUCCEED();
    }

}
