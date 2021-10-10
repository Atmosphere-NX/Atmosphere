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

namespace ams::fssystem {

    DirectoryRedirectionFileSystem::DirectoryRedirectionFileSystem(std::shared_ptr<fs::fsa::IFileSystem> fs, const char *before, const char *after, bool unc)
        : PathResolutionFileSystem(fs, unc)
    {
        m_before_dir = nullptr;
        m_after_dir = nullptr;
        R_ABORT_UNLESS(this->Initialize(before, after));
    }

    DirectoryRedirectionFileSystem::DirectoryRedirectionFileSystem(std::unique_ptr<fs::fsa::IFileSystem> fs, const char *before, const char *after, bool unc)
        : PathResolutionFileSystem(std::move(fs), unc)
    {
        m_before_dir = nullptr;
        m_after_dir = nullptr;
        R_ABORT_UNLESS(this->Initialize(before, after));
    }

    DirectoryRedirectionFileSystem::~DirectoryRedirectionFileSystem() {
        if (m_before_dir != nullptr) {
            fs::impl::Deallocate(m_before_dir, m_before_dir_len);
        }
        if (m_after_dir != nullptr) {
            fs::impl::Deallocate(m_after_dir, m_after_dir_len);
        }
    }

    Result DirectoryRedirectionFileSystem::GetNormalizedDirectoryPath(char **out, size_t *out_size, const char *dir) {
        /* Clear output. */
        *out = nullptr;
        *out_size = 0;

        /* Make sure the path isn't too long. */
        R_UNLESS(strnlen(dir, fs::EntryNameLengthMax + 1) <= fs::EntryNameLengthMax, fs::ResultTooLongPath());

        /* Normalize the path. */
        char normalized_path[fs::EntryNameLengthMax + 2];
        size_t normalized_path_len;
        R_TRY(fs::PathNormalizer::Normalize(normalized_path, &normalized_path_len, dir, sizeof(normalized_path), this->IsUncPreserved(), false));

        /* Ensure terminating '/' */
        if (!fs::PathNormalizer::IsSeparator(normalized_path[normalized_path_len - 1])) {
            AMS_ASSERT(normalized_path_len + 2 < sizeof(normalized_path));
            normalized_path[normalized_path_len]     = fs::StringTraits::DirectorySeparator;
            normalized_path[normalized_path_len + 1] = fs::StringTraits::NullTerminator;

            ++normalized_path_len;
        }

        /* Allocate new path. */
        const size_t size = normalized_path_len + 1;
        char *new_dir = static_cast<char *>(fs::impl::Allocate(size));
        AMS_ABORT_UNLESS(new_dir != nullptr);
        /* TODO: custom fs::ResultAllocationFailure? */

        /* Copy path in. */
        std::memcpy(new_dir, normalized_path, normalized_path_len);
        new_dir[normalized_path_len] = fs::StringTraits::NullTerminator;

        /* Set output. */
        *out = new_dir;
        *out_size = size;
        return ResultSuccess();
    }

    Result DirectoryRedirectionFileSystem::Initialize(const char *before, const char *after) {
        /* Normalize both directories. */
        this->GetNormalizedDirectoryPath(std::addressof(m_before_dir), std::addressof(m_before_dir_len), before);
        this->GetNormalizedDirectoryPath(std::addressof(m_after_dir),  std::addressof(m_after_dir_len), after);

        return ResultSuccess();
    }

    Result DirectoryRedirectionFileSystem::ResolveFullPath(char *out, size_t out_size, const char *relative_path) {
        /* Check pre-conditions. */
        AMS_ASSERT(relative_path[0] == '/');
        AMS_ASSERT(m_before_dir_len >= 2);
        AMS_ASSERT(m_after_dir_len >= 2);
        AMS_ASSERT(m_after_dir_len <= out_size);
        AMS_ASSERT(fs::PathNormalizer::IsNullTerminator(m_before_dir[m_before_dir_len - 1]));
        AMS_ASSERT(fs::PathNormalizer::IsSeparator(m_before_dir[m_before_dir_len - 2]));
        AMS_ASSERT(fs::PathNormalizer::IsNullTerminator(m_after_dir[m_after_dir_len - 1]));
        AMS_ASSERT(fs::PathNormalizer::IsSeparator(m_after_dir[m_after_dir_len - 2]));

        /* Normalize the relative path. */
        char normalized_rel_path[fs::EntryNameLengthMax + 1];
        size_t normalized_rel_path_len;
        R_TRY(fs::PathNormalizer::Normalize(normalized_rel_path, std::addressof(normalized_rel_path_len), relative_path, sizeof(normalized_rel_path), this->IsUncPreserved(), false));

        const bool is_prefixed = std::memcmp(normalized_rel_path, m_before_dir, m_before_dir_len - 2) == 0 &&
                                 (fs::PathNormalizer::IsSeparator(normalized_rel_path[m_before_dir_len - 2]) || fs::PathNormalizer::IsNullTerminator(normalized_rel_path[m_before_dir_len - 2]));
        if (is_prefixed) {
            const size_t before_prefix_len = m_before_dir_len - 2;
            const size_t after_prefix_len  = m_after_dir_len - 2;
            const size_t final_str_len = after_prefix_len + normalized_rel_path_len - before_prefix_len;
            R_UNLESS(final_str_len < out_size, fs::ResultTooLongPath());

            /* Copy normalized path. */
            std::memcpy(out, m_after_dir, after_prefix_len);
            std::memcpy(out + after_prefix_len, normalized_rel_path + before_prefix_len, normalized_rel_path_len - before_prefix_len);
            out[final_str_len] = fs::StringTraits::NullTerminator;
        } else {
            /* Path is not prefixed. */
            R_UNLESS(normalized_rel_path_len + 1 <= out_size, fs::ResultTooLongPath());
            std::memcpy(out, normalized_rel_path, normalized_rel_path_len);
            out[normalized_rel_path_len] = fs::StringTraits::NullTerminator;
        }

        return ResultSuccess();
    }

}
