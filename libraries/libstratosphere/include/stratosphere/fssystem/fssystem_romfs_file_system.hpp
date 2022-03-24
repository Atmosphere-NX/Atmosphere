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
#include <stratosphere/fs/fsa/fs_ifilesystem.hpp>
#include <stratosphere/fs/impl/fs_newable.hpp>
#include <stratosphere/fs/common/fs_dbm_hierarchical_rom_file_table.hpp>
#include <stratosphere/fs/fs_istorage.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: Unknown */

    class RomFsFileSystem : public fs::fsa::IFileSystem, public fs::impl::Newable {
        NON_COPYABLE(RomFsFileSystem);
        public:
            using RomFileTable = fs::HierarchicalRomFileTable;
        private:
            RomFileTable m_rom_file_table;
            fs::IStorage *m_base_storage;
            std::shared_ptr<fs::IStorage> m_shared_storage;
            std::unique_ptr<fs::IStorage> m_dir_bucket_storage;
            std::unique_ptr<fs::IStorage> m_dir_entry_storage;
            std::unique_ptr<fs::IStorage> m_file_bucket_storage;
            std::unique_ptr<fs::IStorage> m_file_entry_storage;
            s64 m_entry_size;
        private:
            Result GetFileInfo(RomFileTable::FileInfo *out, const char *path);
            Result GetFileInfo(RomFileTable::FileInfo *out, const fs::Path &path) {
                R_RETURN(this->GetFileInfo(out, path.GetString()));
            }

            Result CheckPathFormat(const fs::Path &path) const {
                R_RETURN(fs::PathFormatter::CheckPathFormat(path.GetString(), fs::PathFlags{}));
            }
        public:
            static Result GetRequiredWorkingMemorySize(size_t *out, fs::IStorage *storage);
        public:
            RomFsFileSystem();
            virtual ~RomFsFileSystem() override;

            Result Initialize(fs::IStorage *base, void *work, size_t work_size, bool use_cache);
            Result Initialize(std::shared_ptr<fs::IStorage> base, void *work, size_t work_size, bool use_cache);

            fs::IStorage *GetBaseStorage();
            RomFileTable *GetRomFileTable();
            Result GetFileBaseOffset(s64 *out, const fs::Path &path);
        public:
            virtual Result DoCreateFile(const fs::Path &path, s64 size, int flags) override;
            virtual Result DoDeleteFile(const fs::Path &path) override;
            virtual Result DoCreateDirectory(const fs::Path &path) override;
            virtual Result DoDeleteDirectory(const fs::Path &path) override;
            virtual Result DoDeleteDirectoryRecursively(const fs::Path &path) override;
            virtual Result DoRenameFile(const fs::Path &old_path, const fs::Path &new_path) override;
            virtual Result DoRenameDirectory(const fs::Path &old_path, const fs::Path &new_path) override;
            virtual Result DoGetEntryType(fs::DirectoryEntryType *out, const fs::Path &path) override;
            virtual Result DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const fs::Path &path, fs::OpenMode mode) override;
            virtual Result DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const fs::Path &path, fs::OpenDirectoryMode mode) override;
            virtual Result DoCommit() override;
            virtual Result DoGetFreeSpaceSize(s64 *out, const fs::Path &path) override;
            virtual Result DoCleanDirectoryRecursively(const fs::Path &path) override;

            /* These aren't accessible as commands. */
            virtual Result DoCommitProvisionally(s64 counter) override;
    };

}
