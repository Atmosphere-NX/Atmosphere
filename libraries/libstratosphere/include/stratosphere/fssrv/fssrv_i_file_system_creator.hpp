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

namespace ams::fs {

    class IStorage;
    enum class BisPartitionId;

    namespace fsa {

        class IFileSystem;

    }

}

namespace ams::fssystem {

    class NcaReader;
    class NcaFsHeaderReader;

    namespace save {

        /* TODO */

    }

}

namespace ams::fssrv::fscreator {

    class IRomFileSystemCreator {
        public:
            virtual ~IRomFileSystemCreator() { /* ... */ }
            virtual Result Create(std::shared_ptr<fs::fsa::IFileSystem> *out, std::shared_ptr<fs::IStorage> storage) = 0;
    };

    class IPartitionFileSystemCreator {
        public:
            virtual ~IPartitionFileSystemCreator() { /* ... */ }
            virtual Result Create(std::shared_ptr<fs::fsa::IFileSystem> *out, std::shared_ptr<fs::IStorage> storage) = 0;
    };

    class IStorageOnNcaCreator {
        public:
            virtual ~IStorageOnNcaCreator() { /* ... */ }
            virtual Result Create(std::shared_ptr<fs::IStorage> *out, fssystem::NcaFsHeaderReader *out_header_reader, std::shared_ptr<fssystem::NcaReader> nca_reader, s32 index, bool verify_header_sign_2) = 0;
            virtual Result CreateWithPatch(std::shared_ptr<fs::IStorage> *out, fssystem::NcaFsHeaderReader *out_header_reader, std::shared_ptr<fssystem::NcaReader> original_nca_reader, std::shared_ptr<fssystem::NcaReader> current_nca_reader, s32 index, bool verify_header_sign_2) = 0;
            virtual Result CreateNcaReader(std::shared_ptr<fssystem::NcaReader> *out, std::shared_ptr<fs::IStorage> storage) = 0;
            virtual Result VerifyAcid(fs::fsa::IFileSystem *fs, fssystem::NcaReader *nca_reader) = 0;
            virtual void   SetEnabledProgramVerification(bool en) = 0;
    };

    struct FileSystemCreatorInterfaces {
        IRomFileSystemCreator *rom_fs_creator;
        IPartitionFileSystemCreator *partition_fs_creator;
        IStorageOnNcaCreator *storage_on_nca_creator;
        /* TODO: More creators. */
    };
    static_assert(util::is_pod<FileSystemCreatorInterfaces>::value);

}
