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
#include <stratosphere/fs/fs_substorage.hpp>
#include <stratosphere/fssystem/fssystem_nca_file_system_driver.hpp>

namespace ams::fssystem {

    class NcaFileSystemDriver::StorageOption {
        private:
            friend class NcaFileSystemDriver;
        private:
            const s32 fs_index;
            NcaFsHeaderReader * const header_reader;
            fs::IStorage *data_storage;
            s64 data_storage_size;
            fs::IStorage *aes_ctr_ex_table_storage;
            AesCtrCounterExtendedStorage *aes_ctr_ex_storage_raw;
            fs::IStorage *aes_ctr_ex_storage;
            IndirectStorage *indirect_storage;
            SparseStorage *sparse_storage;
        public:
            explicit StorageOption(NcaFsHeaderReader *reader) : fs_index(reader->GetFsIndex()), header_reader(reader), data_storage(), data_storage_size(), aes_ctr_ex_table_storage(), aes_ctr_ex_storage_raw(), aes_ctr_ex_storage(), indirect_storage(), sparse_storage() {
                AMS_ASSERT(this->header_reader != nullptr);
            }

            StorageOption(NcaFsHeaderReader *reader, s32 index) : fs_index(index), header_reader(reader), data_storage(), data_storage_size(), aes_ctr_ex_table_storage(), aes_ctr_ex_storage_raw(), aes_ctr_ex_storage(), indirect_storage(), sparse_storage() {
                AMS_ASSERT(this->header_reader != nullptr);
                AMS_ASSERT(0 <= index && index < NcaHeader::FsCountMax);
            }

            s32 GetFsIndex() const { return this->fs_index; }
            NcaFsHeaderReader &GetHeaderReader() { return *this->header_reader; }
            const NcaFsHeaderReader &GetHeaderReader() const { return *this->header_reader; }
            fs::SubStorage GetDataStorage() const { return fs::SubStorage(this->data_storage, 0, this->data_storage_size); }
            fs::IStorage *GetAesCtrExTableStorage() const { return this->aes_ctr_ex_table_storage; }
            fs::IStorage *GetAesCtrExStorage() const { return this->aes_ctr_ex_storage; }
            AesCtrCounterExtendedStorage *GetAesCtrExStorageRaw() const { return this->aes_ctr_ex_storage_raw; }
            IndirectStorage *GetIndirectStorage() const { return this->indirect_storage; }
            SparseStorage *GetSparseStorage() const { return this->sparse_storage; }
        private:
            void SetDataStorage(fs::IStorage *storage, s64 size) {
                AMS_ASSERT(storage != nullptr);
                AMS_ASSERT(size >= 0);
                this->data_storage = storage;
                this->data_storage_size = size;
            }

            void SetAesCtrExTableStorage(fs::IStorage *storage) { AMS_ASSERT(storage != nullptr); this->aes_ctr_ex_table_storage = storage; }
            void SetAesCtrExStorage(fs::IStorage *storage) { AMS_ASSERT(storage != nullptr); this->aes_ctr_ex_storage = storage; }
            void SetAesCtrExStorageRaw(AesCtrCounterExtendedStorage *storage) { AMS_ASSERT(storage != nullptr); this->aes_ctr_ex_storage_raw = storage; }
            void SetIndirectStorage(IndirectStorage *storage) { AMS_ASSERT(storage != nullptr); this->indirect_storage = storage; }
            void SetSparseStorage(SparseStorage *storage) { AMS_ASSERT(storage != nullptr); this->sparse_storage = storage; }
    };

    class NcaFileSystemDriver::StorageOptionWithHeaderReader : public NcaFileSystemDriver::StorageOption {
        private:
            NcaFsHeaderReader header_reader_data;
        public:
            explicit StorageOptionWithHeaderReader(s32 index) : StorageOption(std::addressof(header_reader_data), index) { /* ... */ }
    };

    class NcaFileSystemDriver::BaseStorage {
        private:
            std::unique_ptr<fs::IStorage> storage;
            fs::SubStorage sub_storage;
            s64 storage_offset;
            NcaAesCtrUpperIv aes_ctr_upper_iv;
        public:
            BaseStorage() : storage(), sub_storage(), storage_offset(0) {
                this->aes_ctr_upper_iv.value = 0;
            }

            explicit BaseStorage(const fs::SubStorage &ss) : storage(), sub_storage(ss), storage_offset(0) {
                this->aes_ctr_upper_iv.value = 0;
            }

            template<typename T>
            BaseStorage(T s, s64 offset, s64 size) : storage(), sub_storage(s, offset, size), storage_offset(0) {
                this->aes_ctr_upper_iv.value = 0;
            }

            void SetStorage(std::unique_ptr<fs::IStorage> &&storage) {
                this->storage = std::move(storage);
            }

            template<typename T>
            void SetStorage(T storage, s64 offset, s64 size) {
                this->sub_storage = fs::SubStorage(storage, offset, size);
            }

            std::unique_ptr<fs::IStorage> MakeStorage() {
                if (this->storage != nullptr) {
                    return std::move(this->storage);
                }
                return std::make_unique<fs::SubStorage>(this->sub_storage);
            }

            std::unique_ptr<fs::IStorage> GetStorage() {
                return std::move(this->storage);
            }

            Result GetSubStorage(fs::SubStorage *out, s64 offset, s64 size) {
                s64 storage_size = 0;

                if (this->storage != nullptr) {
                    R_TRY(this->storage->GetSize(std::addressof(storage_size)));
                    R_UNLESS(offset + size <= storage_size, fs::ResultNcaBaseStorageOutOfRangeA());
                    *out = fs::SubStorage(this->storage.get(), offset, size);
                } else {
                    R_TRY(this->sub_storage.GetSize(std::addressof(storage_size)));
                    R_UNLESS(offset + size <= storage_size, fs::ResultNcaBaseStorageOutOfRangeA());
                    *out = fs::SubStorage(std::addressof(this->sub_storage), offset, size);
                }

                return ResultSuccess();
            }

            void SetStorageOffset(s64 offset) {
                this->storage_offset = offset;
            }

            s64 GetStorageOffset() const {
                return this->storage_offset;
            }

            void SetAesCtrUpperIv(NcaAesCtrUpperIv v) {
                this->aes_ctr_upper_iv = v;
            }

            const NcaAesCtrUpperIv GetAesCtrUpperIv() const {
                return this->aes_ctr_upper_iv;
            }
    };

}
