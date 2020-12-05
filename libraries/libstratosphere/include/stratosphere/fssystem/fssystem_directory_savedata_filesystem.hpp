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
#include "impl/fssystem_path_resolution_filesystem.hpp"

namespace ams::fssystem {

    class DirectorySaveDataFileSystem : public impl::IPathResolutionFileSystem<DirectorySaveDataFileSystem> {
        NON_COPYABLE(DirectorySaveDataFileSystem);
        private:
            using PathResolutionFileSystem = impl::IPathResolutionFileSystem<DirectorySaveDataFileSystem>;
            friend class impl::IPathResolutionFileSystem<DirectorySaveDataFileSystem>;
        private:
            os::Mutex accessor_mutex;
            s32 open_writable_files;
        public:
            DirectorySaveDataFileSystem(std::shared_ptr<fs::fsa::IFileSystem> fs);
            DirectorySaveDataFileSystem(std::unique_ptr<fs::fsa::IFileSystem> fs);
            Result Initialize();

            virtual ~DirectorySaveDataFileSystem();
        protected:
            inline std::optional<std::scoped_lock<os::Mutex>> GetAccessorLock() {
                /* We have a real accessor lock that we want to use. */
                return std::make_optional<std::scoped_lock<os::Mutex>>(this->accessor_mutex);
            }
        private:
            Result AllocateWorkBuffer(std::unique_ptr<u8[]> *out, size_t *out_size, size_t ideal_size);
            Result SynchronizeDirectory(const char *dst, const char *src);
            Result ResolveFullPath(char *out, size_t out_size, const char *relative_path);
        public:
            void OnWritableFileClose();
            Result CopySaveFromFileSystem(fs::fsa::IFileSystem *save_fs);
        public:
            /* Overridden from IPathResolutionFileSystem */
            virtual Result DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const char *path, fs::OpenMode mode) override;
            virtual Result DoCommit() override;

            /* Overridden from IPathResolutionFileSystem but not commands. */
            virtual Result DoCommitProvisionally(s64 counter) override;
            virtual Result DoRollback() override;

            /* Explicitly overridden to be not implemented. */
            virtual Result DoGetFreeSpaceSize(s64 *out, const char *path) override;
            virtual Result DoGetTotalSpaceSize(s64 *out, const char *path) override;
            virtual Result DoGetFileTimeStampRaw(fs::FileTimeStampRaw *out, const char *path) override;
            virtual Result DoQueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fs::fsa::QueryId query, const char *path) override;
            virtual Result DoFlush() override;
    };

}
