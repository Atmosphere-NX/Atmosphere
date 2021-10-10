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
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/fs_memory_storage.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fssystem/fssystem_nca_header.hpp>
#include <stratosphere/fssystem/save/fssystem_hierarchical_integrity_verification_storage.hpp>

namespace ams::fssystem {

    constexpr inline size_t IntegrityLayerCountRomFs    = 7;
    constexpr inline size_t IntegrityHashLayerBlockSize = 16_KB;

    class IntegrityRomFsStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        private:
            save::HierarchicalIntegrityVerificationStorage m_integrity_storage;
            save::FileSystemBufferManagerSet m_buffers;
            os::SdkRecursiveMutex m_mutex;
            Hash m_master_hash;
            std::unique_ptr<fs::MemoryStorage> m_master_hash_storage;
        public:
            IntegrityRomFsStorage() : m_mutex() { /* ... */ }
            virtual ~IntegrityRomFsStorage() override { this->Finalize(); }

            Result Initialize(save::HierarchicalIntegrityVerificationInformation level_hash_info, Hash master_hash, save::HierarchicalIntegrityVerificationStorage::HierarchicalStorageInformation storage_info, IBufferManager *bm);
            void Finalize();

            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                return m_integrity_storage.Read(offset, buffer, size);
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                return m_integrity_storage.Write(offset, buffer, size);
            }

            virtual Result SetSize(s64 size) override { AMS_UNUSED(size); return fs::ResultUnsupportedOperationInIntegrityRomFsStorageA(); }

            virtual Result GetSize(s64 *out) override {
                return m_integrity_storage.GetSize(out);
            }

            virtual Result Flush() override {
                return m_integrity_storage.Flush();
            }

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                return m_integrity_storage.OperateRange(dst, dst_size, op_id, offset, size, src, src_size);
            }

            Result Commit() {
                return m_integrity_storage.Commit();
            }

            save::FileSystemBufferManagerSet *GetBuffers() {
                return m_integrity_storage.GetBuffers();
            }
    };

}
