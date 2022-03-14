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
#include <stratosphere/fssrv/fssrv_i_file_system_creator.hpp>
#include <stratosphere/fs/fs_i_buffer_manager.hpp>
#include <stratosphere/fssystem/fssystem_i_hash_256_generator.hpp>

namespace ams::fssystem {

    struct NcaCryptoConfiguration;
    struct NcaCompressionConfiguration;

}

namespace ams::fssrv::fscreator {

    class StorageOnNcaCreator : public IStorageOnNcaCreator {
        NON_COPYABLE(StorageOnNcaCreator);
        NON_MOVEABLE(StorageOnNcaCreator);
        private:
            MemoryResource *m_allocator;
            const fssystem::NcaCryptoConfiguration &m_nca_crypto_cfg;
            const fssystem::NcaCompressionConfiguration &m_nca_compression_cfg;
            fs::IBufferManager * const m_buffer_manager;
            fssystem::IHash256GeneratorFactorySelector * const m_hash_generator_factory_selector;
        public:
            explicit StorageOnNcaCreator(MemoryResource *mr, const fssystem::NcaCryptoConfiguration &cfg, const fssystem::NcaCompressionConfiguration &c_cfg, fs::IBufferManager *bm, fssystem::IHash256GeneratorFactorySelector *hgfs)
                : m_allocator(mr), m_nca_crypto_cfg(cfg), m_nca_compression_cfg(c_cfg), m_buffer_manager(bm), m_hash_generator_factory_selector(hgfs)
            {
                /* ... */
            }

            virtual Result Create(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::IAsynchronousAccessSplitter> *out_splitter, fssystem::NcaFsHeaderReader *out_header_reader, std::shared_ptr<fssystem::NcaReader> nca_reader, s32 index) override;
            virtual Result CreateWithPatch(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::IAsynchronousAccessSplitter> *out_splitter, fssystem::NcaFsHeaderReader *out_header_reader, std::shared_ptr<fssystem::NcaReader> original_nca_reader, std::shared_ptr<fssystem::NcaReader> current_nca_reader, s32 index) override;
            virtual Result CreateNcaReader(std::shared_ptr<fssystem::NcaReader> *out, std::shared_ptr<fs::IStorage> storage) override;

            #if !defined(ATMOSPHERE_BOARD_NINTENDO_NX)
            Result CreateWithContext(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::IAsynchronousAccessSplitter> *out_splitter, fssystem::NcaFsHeaderReader *out_header_reader, void *ctx, std::shared_ptr<fssystem::NcaReader> nca_reader, s32 index);
            Result CreateWithPatchWithContext(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::IAsynchronousAccessSplitter> *out_splitter, fssystem::NcaFsHeaderReader *out_header_reader, void *ctx, std::shared_ptr<fssystem::NcaReader> original_nca_reader, std::shared_ptr<fssystem::NcaReader> current_nca_reader, s32 index);

            Result CreateByRawStorage(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::IAsynchronousAccessSplitter> *out_splitter, const fssystem::NcaFsHeaderReader *header_reader, std::shared_ptr<fs::IStorage> raw_storage, void *ctx, std::shared_ptr<fssystem::NcaReader> nca_reader);
            #endif
    };

}
