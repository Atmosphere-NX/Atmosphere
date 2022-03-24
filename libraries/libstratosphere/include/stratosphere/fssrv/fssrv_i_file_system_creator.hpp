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
#include <stratosphere/fs/impl/fs_newable.hpp>

namespace ams::fs {

    class IStorage;
    enum class BisPartitionId;

    class Path;

    namespace fsa {

        class IFileSystem;

    }

}

namespace ams::fssystem {

    class NcaReader;
    class NcaFsHeaderReader;

    class IAsynchronousAccessSplitter;

    namespace save {

        /* TODO */

    }

}

namespace ams::fssrv::fscreator {

    /* ACCURATE_TO_VERSION: Unknown */
    class IRomFileSystemCreator {
        public:
            virtual ~IRomFileSystemCreator() { /* ... */ }
            virtual Result Create(std::shared_ptr<fs::fsa::IFileSystem> *out, std::shared_ptr<fs::IStorage> storage) = 0;
    };

    /* ACCURATE_TO_VERSION: Unknown */
    class IPartitionFileSystemCreator {
        public:
            virtual ~IPartitionFileSystemCreator() { /* ... */ }
            virtual Result Create(std::shared_ptr<fs::fsa::IFileSystem> *out, std::shared_ptr<fs::IStorage> storage) = 0;
    };

    /* ACCURATE_TO_VERSION: Unknown */
    class IStorageOnNcaCreator {
        public:
            virtual ~IStorageOnNcaCreator() { /* ... */ }
            virtual Result Create(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::IAsynchronousAccessSplitter> *out_splitter, fssystem::NcaFsHeaderReader *out_header_reader, std::shared_ptr<fssystem::NcaReader> nca_reader, s32 index) = 0;
            virtual Result CreateWithPatch(std::shared_ptr<fs::IStorage> *out, std::shared_ptr<fssystem::IAsynchronousAccessSplitter> *out_splitter, fssystem::NcaFsHeaderReader *out_header_reader, std::shared_ptr<fssystem::NcaReader> original_nca_reader, std::shared_ptr<fssystem::NcaReader> current_nca_reader, s32 index) = 0;
            virtual Result CreateNcaReader(std::shared_ptr<fssystem::NcaReader> *out, std::shared_ptr<fs::IStorage> storage) = 0;
    };

    /* ACCURATE_TO_VERSION: Unknown */
    class ILocalFileSystemCreator {
        public:
            virtual Result Create(std::shared_ptr<fs::fsa::IFileSystem> *out, const fs::Path &path, bool case_sensitive, bool ensure_root, Result on_path_not_found) = 0;
        public:
            Result Create(std::shared_ptr<fs::fsa::IFileSystem> *out, const fs::Path &path, bool case_sensitive) {
                R_RETURN(this->Create(out, path, case_sensitive, false, ResultSuccess()));
            }
    };

    /* ACCURATE_TO_VERSION: Unknown */
    class ISubDirectoryFileSystemCreator {
        public:
            virtual Result Create(std::shared_ptr<fs::fsa::IFileSystem> *out, std::shared_ptr<fs::fsa::IFileSystem> base_fs, const fs::Path &path) = 0;
    };

    /* ACCURATE_TO_VERSION: Unknown */
    struct FileSystemCreatorInterfaces {
        ILocalFileSystemCreator *local_fs_creator;
        ISubDirectoryFileSystemCreator *subdir_fs_creator;
        /* TODO: These don't exist any more, and should be refactored out. */
        IRomFileSystemCreator *rom_fs_creator;
        IPartitionFileSystemCreator *partition_fs_creator;
        IStorageOnNcaCreator *storage_on_nca_creator;
        /* TODO: More creators. */
    };
    static_assert(util::is_pod<FileSystemCreatorInterfaces>::value);

}
