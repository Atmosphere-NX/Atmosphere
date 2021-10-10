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
#include <stratosphere/fssystem/impl/fssystem_path_resolution_filesystem.hpp>

namespace ams::fssystem {

    class SubDirectoryFileSystem : public impl::IPathResolutionFileSystem<SubDirectoryFileSystem> {
        NON_COPYABLE(SubDirectoryFileSystem);
        private:
            using PathResolutionFileSystem = impl::IPathResolutionFileSystem<SubDirectoryFileSystem>;
            friend class impl::IPathResolutionFileSystem<SubDirectoryFileSystem>;
        private:
            char *m_base_path;
            size_t m_base_path_len;
        public:
            SubDirectoryFileSystem(std::shared_ptr<fs::fsa::IFileSystem> fs, const char *bp, bool unc = false);
            SubDirectoryFileSystem(std::unique_ptr<fs::fsa::IFileSystem> fs, const char *bp, bool unc = false);

            virtual ~SubDirectoryFileSystem();
        protected:
            inline util::optional<std::scoped_lock<os::SdkMutex>> GetAccessorLock() const {
                /* No accessor lock is needed. */
                return util::nullopt;
            }
        private:
            Result Initialize(const char *bp);
            Result ResolveFullPath(char *out, size_t out_size, const char *relative_path);
    };

}
