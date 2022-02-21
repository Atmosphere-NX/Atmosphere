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

namespace ams::fssrv::fscreator {

    Result StorageOnNcaCreator::Create(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::IAsynchronousAccessSplitter> *out_splitter, fssystem::NcaFsHeaderReader *out_header_reader, std::shared_ptr<fssystem::NcaReader> nca_reader, s32 index) {
        /* Create a fs driver. */
        fssystem::NcaFileSystemDriver nca_fs_driver(nca_reader, m_allocator, m_buffer_manager, m_hash_generator_factory_selector);

        /* Open the storage. */
        std::shared_ptr<fs::IStorage> storage;
        std::shared_ptr<fssystem::IAsynchronousAccessSplitter> splitter;
        R_TRY(nca_fs_driver.OpenStorage(std::addressof(storage), std::addressof(splitter), out_header_reader, index));

        /* Set the out storage. */
        *out = std::move(storage);
        *out_splitter = std::move(splitter);
        return ResultSuccess();
    }

    Result StorageOnNcaCreator::CreateWithPatch(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::IAsynchronousAccessSplitter> *out_splitter, fssystem::NcaFsHeaderReader *out_header_reader, std::shared_ptr<fssystem::NcaReader> original_nca_reader, std::shared_ptr<fssystem::NcaReader> current_nca_reader, s32 index) {
        /* Create a fs driver. */
        fssystem::NcaFileSystemDriver nca_fs_driver(original_nca_reader, current_nca_reader, m_allocator, m_buffer_manager, m_hash_generator_factory_selector);

        /* Open the storage. */
        std::shared_ptr<fs::IStorage> storage;
        std::shared_ptr<fssystem::IAsynchronousAccessSplitter> splitter;
        R_TRY(nca_fs_driver.OpenStorage(std::addressof(storage), std::addressof(splitter), out_header_reader, index));

        /* Set the out storage. */
        *out = std::move(storage);
        *out_splitter = std::move(splitter);
        return ResultSuccess();
    }

    Result StorageOnNcaCreator::CreateNcaReader(std::shared_ptr<fssystem::NcaReader> *out, std::shared_ptr<fs::IStorage> storage) {
        /* Create a reader. */
        std::shared_ptr reader = fssystem::AllocateShared<fssystem::NcaReader>();
        R_UNLESS(reader != nullptr, fs::ResultAllocationFailureInStorageOnNcaCreatorB());

        /* Initialize the reader. */
        R_TRY(reader->Initialize(std::move(storage), m_nca_crypto_cfg, m_nca_compression_cfg, m_hash_generator_factory_selector));

        /* Set the output. */
        *out = std::move(reader);
        return ResultSuccess();
    }

}
