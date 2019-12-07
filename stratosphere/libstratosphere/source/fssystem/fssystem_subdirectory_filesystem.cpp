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
#include <stratosphere.hpp>

namespace ams::fssystem {

    SubDirectoryFileSystem::SubDirectoryFileSystem(std::shared_ptr<fs::fsa::IFileSystem> fs, const char *bp, bool unc)
        : PathResolutionFileSystem(fs, unc)
    {
        this->base_path = nullptr;
        R_ASSERT(this->Initialize(bp));
    }

    SubDirectoryFileSystem::SubDirectoryFileSystem(std::unique_ptr<fs::fsa::IFileSystem> fs, const char *bp, bool unc)
        : PathResolutionFileSystem(std::move(fs), unc)
    {
        this->base_path = nullptr;
        R_ASSERT(this->Initialize(bp));
    }

    SubDirectoryFileSystem::~SubDirectoryFileSystem() {
        if (this->base_path != nullptr) {
            std::free(this->base_path);
        }
    }

    Result SubDirectoryFileSystem::Initialize(const char *bp) {
        /* Make sure the path isn't too long. */
        R_UNLESS(strnlen(bp, fs::EntryNameLengthMax + 1) <= fs::EntryNameLengthMax, fs::ResultTooLongPath());

        /* Normalize the path. */
        char normalized_path[fs::EntryNameLengthMax + 2];
        size_t normalized_path_len;
        R_TRY(PathTool::Normalize(normalized_path, &normalized_path_len, bp, sizeof(normalized_path), this->IsUncPreserved()));

        /* Ensure terminating '/' */
        if (!PathTool::IsSeparator(normalized_path[normalized_path_len - 1])) {
            AMS_ASSERT(normalized_path_len + 2 <= sizeof(normalized_path));
            normalized_path[normalized_path_len]     = StringTraits::DirectorySeparator;
            normalized_path[normalized_path_len + 1] = StringTraits::NullTerminator;

            normalized_path_len++;
        }

        /* Allocate new path. */
        this->base_path_len = normalized_path_len + 1;
        this->base_path = static_cast<char *>(std::malloc(this->base_path_len));
        R_UNLESS(this->base_path != nullptr, fs::ResultAllocationFailureInSubDirectoryFileSystem());

        /* Copy path in. */
        std::memcpy(this->base_path, normalized_path, normalized_path_len);
        this->base_path[normalized_path_len] = StringTraits::NullTerminator;
        return ResultSuccess();
    }

    Result SubDirectoryFileSystem::ResolveFullPath(char *out, size_t out_size, const char *relative_path) {
        /* Ensure path will fit. */
        R_UNLESS(this->base_path_len + strnlen(relative_path, fs::EntryNameLengthMax + 1) <= out_size, fs::ResultTooLongPath());

        /* Copy base path. */
        std::memcpy(out, this->base_path, this->base_path_len);

        /* Normalize it. */
        const size_t prefix_size = this->base_path_len - 2;
        size_t normalized_len;
        return PathTool::Normalize(out + prefix_size, &normalized_len, relative_path, out_size - prefix_size, this->IsUncPreserved());
    }

}
