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
#include <stratosphere/fssrv/fssrv_i_file_system_creator.hpp>
#include <stratosphere/fssystem/buffers/fssystem_i_buffer_manager.hpp>

namespace ams::fssystem {

    struct NcaCryptoConfiguration;

}

namespace ams::fssrv::fscreator {

    class StorageOnNcaCreator : public IStorageOnNcaCreator {
        NON_COPYABLE(StorageOnNcaCreator);
        NON_MOVEABLE(StorageOnNcaCreator);
        private:
            MemoryResource *allocator;
            fssystem::IBufferManager * const buffer_manager;
            const fssystem::NcaCryptoConfiguration &nca_crypto_cfg;
            bool is_prod;
            bool is_enabled_program_verification;
        private:
            Result VerifyNcaHeaderSign2(fssystem::NcaReader *nca_reader, fs::IStorage *storage);
        public:
            explicit StorageOnNcaCreator(MemoryResource *mr, const fssystem::NcaCryptoConfiguration &cfg, bool prod, fssystem::IBufferManager *bm) : allocator(mr), buffer_manager(bm), nca_crypto_cfg(cfg), is_prod(prod), is_enabled_program_verification(true) {
                /* ... */
            }

            virtual Result Create(std::shared_ptr<fs::IStorage> *out, fssystem::NcaFsHeaderReader *out_header_reader, std::shared_ptr<fssystem::NcaReader> nca_reader, s32 index, bool verify_header_sign_2) override;
            virtual Result CreateWithPatch(std::shared_ptr<fs::IStorage> *out, fssystem::NcaFsHeaderReader *out_header_reader, std::shared_ptr<fssystem::NcaReader> original_nca_reader, std::shared_ptr<fssystem::NcaReader> current_nca_reader, s32 index, bool verify_header_sign_2) override;
            virtual Result CreateNcaReader(std::shared_ptr<fssystem::NcaReader> *out, std::shared_ptr<fs::IStorage> storage) override;
            virtual Result VerifyAcid(fs::fsa::IFileSystem *fs, fssystem::NcaReader *nca_reader) override;
            virtual void   SetEnabledProgramVerification(bool en) override;
    };

}
