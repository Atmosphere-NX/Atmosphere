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
#include <stratosphere/fs/fs_substorage.hpp>
#include <stratosphere/fssystem/fssystem_nca_file_system_driver.hpp>

namespace ams::fssystem {

    class NcaFileSystemDriver::StorageOption {
        private:
            friend class NcaFileSystemDriver;
        private:
            const s32 m_fs_index;
            NcaFsHeaderReader * const m_header_reader;
            fs::IStorage *m_data_storage;
            s64 m_data_storage_size;
            fs::IStorage *m_aes_ctr_ex_table_storage;
            AesCtrCounterExtendedStorage *m_aes_ctr_ex_storage_raw;
            fs::IStorage *m_aes_ctr_ex_storage;
            IndirectStorage *m_indirect_storage;
            SparseStorage *m_sparse_storage;
        public:
            explicit StorageOption(NcaFsHeaderReader *reader) : m_fs_index(reader->GetFsIndex()), m_header_reader(reader), m_data_storage(), m_data_storage_size(), m_aes_ctr_ex_table_storage(), m_aes_ctr_ex_storage_raw(), m_aes_ctr_ex_storage(), m_indirect_storage(), m_sparse_storage() {
                AMS_ASSERT(m_header_reader != nullptr);
            }

            StorageOption(NcaFsHeaderReader *reader, s32 index) : m_fs_index(index), m_header_reader(reader), m_data_storage(), m_data_storage_size(), m_aes_ctr_ex_table_storage(), m_aes_ctr_ex_storage_raw(), m_aes_ctr_ex_storage(), m_indirect_storage(), m_sparse_storage() {
                AMS_ASSERT(m_header_reader != nullptr);
                AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
            }

            s32 GetFsIndex() const { return m_fs_index; }
            NcaFsHeaderReader &GetHeaderReader() { return *m_header_reader; }
            const NcaFsHeaderReader &GetHeaderReader() const { return *m_header_reader; }
            fs::SubStorage GetDataStorage() const { return fs::SubStorage(m_data_storage, 0, m_data_storage_size); }
            fs::IStorage *GetAesCtrExTableStorage() const { return m_aes_ctr_ex_table_storage; }
            fs::IStorage *GetAesCtrExStorage() const { return m_aes_ctr_ex_storage; }
            AesCtrCounterExtendedStorage *GetAesCtrExStorageRaw() const { return m_aes_ctr_ex_storage_raw; }
            IndirectStorage *GetIndirectStorage() const { return m_indirect_storage; }
            SparseStorage *GetSparseStorage() const { return m_sparse_storage; }
        private:
            void SetDataStorage(fs::IStorage *storage, s64 size) {
                AMS_ASSERT(storage != nullptr);
                AMS_ASSERT(size >= 0);
                m_data_storage = storage;
                m_data_storage_size = size;
            }

            void SetAesCtrExTableStorage(fs::IStorage *storage) { AMS_ASSERT(storage != nullptr); m_aes_ctr_ex_table_storage = storage; }
            void SetAesCtrExStorage(fs::IStorage *storage) { AMS_ASSERT(storage != nullptr); m_aes_ctr_ex_storage = storage; }
            void SetAesCtrExStorageRaw(AesCtrCounterExtendedStorage *storage) { AMS_ASSERT(storage != nullptr); m_aes_ctr_ex_storage_raw = storage; }
            void SetIndirectStorage(IndirectStorage *storage) { AMS_ASSERT(storage != nullptr); m_indirect_storage = storage; }
            void SetSparseStorage(SparseStorage *storage) { AMS_ASSERT(storage != nullptr); m_sparse_storage = storage; }
    };

    class NcaFileSystemDriver::StorageOptionWithHeaderReader : public NcaFileSystemDriver::StorageOption {
        private:
            NcaFsHeaderReader m_header_reader_data;
        public:
            explicit StorageOptionWithHeaderReader(s32 index) : StorageOption(std::addressof(m_header_reader_data), index) { /* ... */ }
    };

    class NcaFileSystemDriver::BaseStorage {
        private:
            std::unique_ptr<fs::IStorage> m_storage;
            fs::SubStorage m_sub_storage;
            s64 m_storage_offset;
            NcaAesCtrUpperIv m_aes_ctr_upper_iv;
        public:
            BaseStorage() : m_storage(), m_sub_storage(), m_storage_offset(0) {
                m_aes_ctr_upper_iv.value = 0;
            }

            explicit BaseStorage(const fs::SubStorage &ss) : m_storage(), m_sub_storage(ss), m_storage_offset(0) {
                m_aes_ctr_upper_iv.value = 0;
            }

            template<typename T>
            BaseStorage(T s, s64 offset, s64 size) : m_storage(), m_sub_storage(s, offset, size), m_storage_offset(0) {
                m_aes_ctr_upper_iv.value = 0;
            }

            void SetStorage(std::unique_ptr<fs::IStorage> &&storage) {
                m_storage = std::move(storage);
            }

            template<typename T>
            void SetStorage(T storage, s64 offset, s64 size) {
                m_sub_storage = fs::SubStorage(storage, offset, size);
            }

            std::unique_ptr<fs::IStorage> MakeStorage() {
                if (m_storage != nullptr) {
                    return std::move(m_storage);
                }
                return std::make_unique<fs::SubStorage>(m_sub_storage);
            }

            std::unique_ptr<fs::IStorage> GetStorage() {
                return std::move(m_storage);
            }

            Result GetSubStorage(fs::SubStorage *out, s64 offset, s64 size) {
                s64 storage_size = 0;

                if (m_storage != nullptr) {
                    R_TRY(m_storage->GetSize(std::addressof(storage_size)));
                    R_UNLESS(offset + size <= storage_size, fs::ResultNcaBaseStorageOutOfRangeA());
                    *out = fs::SubStorage(m_storage.get(), offset, size);
                } else {
                    R_TRY(m_sub_storage.GetSize(std::addressof(storage_size)));
                    R_UNLESS(offset + size <= storage_size, fs::ResultNcaBaseStorageOutOfRangeA());
                    *out = fs::SubStorage(std::addressof(m_sub_storage), offset, size);
                }

                return ResultSuccess();
            }

            void SetStorageOffset(s64 offset) {
                m_storage_offset = offset;
            }

            s64 GetStorageOffset() const {
                return m_storage_offset;
            }

            void SetAesCtrUpperIv(NcaAesCtrUpperIv v) {
                m_aes_ctr_upper_iv = v;
            }

            const NcaAesCtrUpperIv GetAesCtrUpperIv() const {
                return m_aes_ctr_upper_iv;
            }
    };

}
