/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "fssystem_path_resolution_filesystem.hpp"

namespace ams::fssystem {

    class DirectoryRedirectionFileSystem : public IPathResolutionFileSystem<DirectoryRedirectionFileSystem> {
        NON_COPYABLE(DirectoryRedirectionFileSystem);
        private:
            using PathResolutionFileSystem = IPathResolutionFileSystem<DirectoryRedirectionFileSystem>;
        private:
            char *before_dir;
            size_t before_dir_len;
            char *after_dir;
            size_t after_dir_len;
            bool unc_preserved;
        public:
            DirectoryRedirectionFileSystem(std::shared_ptr<fs::fsa::IFileSystem> fs, const char *before, const char *after);
            DirectoryRedirectionFileSystem(std::shared_ptr<fs::fsa::IFileSystem> fs, const char *before, const char *after, bool unc);

            virtual ~DirectoryRedirectionFileSystem();
        private:
            Result GetNormalizedDirectoryPath(char **out, size_t *out_size, const char *dir);
            Result Initialize(const char *before, const char *after);
        public:
            Result ResolveFullPath(char *out, size_t out_size, const char *relative_path);
    };

}
