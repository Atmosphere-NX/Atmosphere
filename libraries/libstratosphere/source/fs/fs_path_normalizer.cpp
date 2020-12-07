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
#include <stratosphere.hpp>

namespace ams::fs {

    namespace {

        Result CheckSharedName(const char *path, int len) {
            if (len == 1) {
                R_UNLESS(path[0] != StringTraits::Dot, fs::ResultInvalidPathFormat());
            } else if (len == 2) {
                R_UNLESS(path[0] != StringTraits::Dot || path[1] != StringTraits::Dot, fs::ResultInvalidPathFormat());
            }

            return ResultSuccess();
        }

        Result ParseWindowsPath(const char **out_path, char *out, size_t *out_windows_path_len, bool *out_normalized, const char *path, size_t max_out_size, bool has_mount_name) {
            /* Prepare to parse. */
            const char * const path_start = path;
            if (out_normalized != nullptr) {
                *out_normalized = true;
            }

            /* Handle start of path. */
            bool skipped_mount = false;
            auto prefix_len = 0;
            if (has_mount_name) {
                if (PathNormalizer::IsSeparator(path[0]) && path[1] == StringTraits::AlternateDirectorySeparator && path[2] == StringTraits::AlternateDirectorySeparator) {
                    path += 1;
                    prefix_len = 1;
                } else {
                    /* Advance past separators. */
                    while (PathNormalizer::IsSeparator(path[0])) {
                        ++path;
                    }
                    if (path != path_start) {
                        if (path - path_start == 1 || IsWindowsDrive(path)) {
                            prefix_len = 1;
                        } else {
                            if (path - path_start > 2 && out_normalized != nullptr) {
                                *out_normalized = false;
                                return ResultSuccess();
                            }
                            path -= 2;
                            skipped_mount = true;
                        }
                    }
                }
            } else if (PathNormalizer::IsSeparator(path[0]) && !IsUnc(path)) {
                path += 1;
                prefix_len = 1;
            }


            /* Parse the path. */
            const char *trimmed_path = path_start;
            if (IsWindowsDrive(path)) {
                /* Find the first separator. */
                int i;
                for (i = 2; !PathNormalizer::IsNullTerminator(path[i]); ++i) {
                    if (PathNormalizer::IsAnySeparator(path[i])) {
                        break;
                    }
                }
                trimmed_path = path + i;

                const size_t win_path_len = trimmed_path - path_start;
                if (out != nullptr) {
                    R_UNLESS(win_path_len <= max_out_size, fs::ResultTooLongPath());
                    std::memcpy(out, path_start, win_path_len);
                }
                *out_path             = trimmed_path;
                *out_windows_path_len = win_path_len;
            } else if (IsUnc(path)) {
                if (PathNormalizer::IsAnySeparator(path[2])) {
                    AMS_ASSERT(!has_mount_name);
                    return fs::ResultInvalidPathFormat();
                }

                int cur_part_ofs = 0;
                bool needs_sep_fix = false;
                for (auto i = 2; !PathNormalizer::IsNullTerminator(path[i]); ++i) {
                    if (cur_part_ofs == 0 && path[i] == StringTraits::AlternateDirectorySeparator) {
                        needs_sep_fix = true;
                        if (out_normalized != nullptr) {
                            *out_normalized = false;
                            return ResultSuccess();
                        }
                    }
                    if (PathNormalizer::IsAnySeparator(path[i])) {
                        if (path[i] == StringTraits::AlternateDirectorySeparator) {
                            needs_sep_fix = true;
                        }

                        if (cur_part_ofs != 0) {
                            break;
                        }

                        R_UNLESS(!PathNormalizer::IsSeparator(path[i + 1]), fs::ResultInvalidPathFormat());

                        R_TRY(CheckSharedName(path + 2, i - 2));

                        cur_part_ofs = i + 1;
                    }
                    if (path[i] == '$' || path[i] == StringTraits::DriveSeparator) {
                        R_UNLESS(cur_part_ofs != 0, fs::ResultInvalidCharacter());
                        R_UNLESS(PathNormalizer::IsAnySeparator(path[i + 1]) || PathNormalizer::IsNullTerminator(path[i + 1]), fs::ResultInvalidPathFormat());
                        trimmed_path = path + i + 1;
                        break;
                    }
                }

                if (trimmed_path == path_start) {
                    int tr_part_ofs = 0;
                    int i;
                    for (i = 2; !PathNormalizer::IsNullTerminator(path[i]); ++i) {
                        if (PathNormalizer::IsAnySeparator(path[i])) {
                            if (tr_part_ofs != 0) {
                                R_TRY(CheckSharedName(path + tr_part_ofs, i - tr_part_ofs));
                                trimmed_path = path + i;
                                break;
                            }

                            R_UNLESS(!PathNormalizer::IsSeparator(path[i + 1]), fs::ResultInvalidPathFormat());

                            R_TRY(CheckSharedName(path + 2, i - 2));

                            cur_part_ofs = i + 1;
                        }
                    }

                    if (tr_part_ofs != 0 && trimmed_path == path_start) {
                        R_TRY(CheckSharedName(path + tr_part_ofs, i - tr_part_ofs));
                        trimmed_path = path + i;
                    }
                }

                const size_t win_path_len = trimmed_path - path;
                const bool prepend_sep = prefix_len != 0 || skipped_mount;
                if (out != nullptr) {
                    R_UNLESS(win_path_len <= max_out_size, fs::ResultTooLongPath());
                    if (prepend_sep) {
                        *(out++) = StringTraits::DirectorySeparator;
                    }
                    std::memcpy(out, path, win_path_len);
                    out[0] = StringTraits::AlternateDirectorySeparator;
                    out[1] = StringTraits::AlternateDirectorySeparator;
                    if (needs_sep_fix) {
                        for (size_t i = 2; i < win_path_len; ++i) {
                            if (PathNormalizer::IsSeparator(out[i])) {
                                out[i] = StringTraits::AlternateDirectorySeparator;
                            }
                        }
                    }
                }
                *out_path             = trimmed_path;
                *out_windows_path_len = win_path_len + (prepend_sep ? 1 : 0);
            } else {
                *out_path = trimmed_path;
            }

            return ResultSuccess();
        }

        Result SkipWindowsPath(const char **out_path, bool *out_normalized, const char *path, bool has_mount_name) {
            size_t windows_path_len;
            return ParseWindowsPath(out_path, nullptr, std::addressof(windows_path_len), out_normalized, path, 0, has_mount_name);
        }

        Result ParseMountName(const char **out_path, char *out, size_t *out_mount_name_len, const char *path, size_t max_out_size) {
            /* Decide on a start. */
            const char *start = PathNormalizer::IsSeparator(path[0]) ? path + 1 : path;

            /* Find the end of the mount name. */
            const char *cur;
            for (cur = start; cur < start + MountNameLengthMax + 1; ++cur) {
                if (*cur == StringTraits::DriveSeparator) {
                    ++cur;
                    break;
                }
                if (PathNormalizer::IsSeparator(*cur)) {
                    *out_path           = path;
                    *out_mount_name_len = 0;
                    return ResultSuccess();
                }
            }
            R_UNLESS(start < cur - 1,                         fs::ResultInvalidPathFormat());
            R_UNLESS(cur[-1] == StringTraits::DriveSeparator, fs::ResultInvalidPathFormat());

            /* Check the mount name doesn't contain a dot. */
            if (cur != start) {
                for (const char *p = start; p < cur; ++p) {
                    R_UNLESS(*p != StringTraits::Dot, fs::ResultInvalidCharacter());
                }
            }

            const size_t mount_name_len = cur - path;
            if (out != nullptr) {
                R_UNLESS(mount_name_len <= max_out_size, fs::ResultTooLongPath());
                std::memcpy(out, path, mount_name_len);
            }
            *out_path = cur;
            *out_mount_name_len = mount_name_len;

            return ResultSuccess();
        }

        Result SkipMountName(const char **out_path, const char *path) {
            size_t mount_name_len;
            return ParseMountName(out_path, nullptr, std::addressof(mount_name_len), path, 0);
        }

        bool IsParentDirectoryPathReplacementNeeded(const char *path) {
            if (!PathNormalizer::IsAnySeparator(path[0])) {
                return false;
            }

            for (auto i = 0; !PathNormalizer::IsNullTerminator(path[i]); ++i) {
                if (path[i + 0] == StringTraits::AlternateDirectorySeparator &&
                    path[i + 1] == StringTraits::Dot &&
                    path[i + 2] == StringTraits::Dot &&
                    (PathNormalizer::IsAnySeparator(path[i + 3]) || PathNormalizer::IsNullTerminator(path[i + 3])))
                {
                    return true;
                }

                if (PathNormalizer::IsAnySeparator(path[i + 0]) &&
                    path[i + 1] == StringTraits::Dot &&
                    path[i + 2] == StringTraits::Dot &&
                    path[i + 3] == StringTraits::AlternateDirectorySeparator)
                {
                    return true;
                }
            }

            return false;
        }

        void ReplaceParentDirectoryPath(char *dst, const char *src) {
            dst[0] = StringTraits::DirectorySeparator;

            int i = 1;
            while (!PathNormalizer::IsNullTerminator(src[i])) {
                if (PathNormalizer::IsAnySeparator(src[i - 1]) &&
                    src[i + 0] == StringTraits::Dot &&
                    src[i + 1] == StringTraits::Dot &&
                    PathNormalizer::IsAnySeparator(src[i + 2]))
                {
                    dst[i - 1] = StringTraits::DirectorySeparator;
                    dst[i + 0] = StringTraits::Dot;
                    dst[i + 1] = StringTraits::Dot;
                    dst[i - 2] = StringTraits::DirectorySeparator;
                    i += 3;
                } else {
                    if (src[i - 1] == StringTraits::AlternateDirectorySeparator &&
                        src[i + 0] == StringTraits::Dot &&
                        src[i + 1] == StringTraits::Dot &&
                        PathNormalizer::IsNullTerminator(src[i + 2]))
                    {
                        dst[i - 1] = StringTraits::DirectorySeparator;
                        dst[i + 0] = StringTraits::Dot;
                        dst[i + 1] = StringTraits::Dot;
                        i += 2;
                        break;
                    }

                    dst[i] = src[i];
                    ++i;
                }
            }

            dst[i] = StringTraits::NullTerminator;
        }

    }

    Result PathNormalizer::Normalize(char *out, size_t *out_len, const char *path, size_t max_out_size, bool unc_preserved, bool has_mount_name) {
        /* Check pre-conditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(out_len != nullptr);
        AMS_ASSERT(path != nullptr);

        /* If we should, handle the mount name. */
        size_t prefix_len = 0;
        if (has_mount_name) {
            size_t mount_name_len = 0;
            R_TRY(ParseMountName(std::addressof(path), out, std::addressof(mount_name_len), path, max_out_size));
            prefix_len += mount_name_len;
        }

        /* Deal with unc. */
        bool is_unc_path = false;
        if (unc_preserved) {
            const char * const path_start = path;
            size_t windows_path_len = 0;
            R_TRY(ParseWindowsPath(std::addressof(path), out + prefix_len, std::addressof(windows_path_len), nullptr, path, max_out_size, has_mount_name));
            prefix_len += windows_path_len;
            is_unc_path = path != path_start;
        }

        /* Paths must start with / */
        R_UNLESS(prefix_len != 0 || IsSeparator(path[0]), fs::ResultInvalidPathFormat());

        /* Check if parent directory path replacement is needed. */
        std::unique_ptr<char[], fs::impl::Deleter> replacement_path;
        if (IsParentDirectoryPathReplacementNeeded(path)) {
            /* Allocate a buffer to hold the replacement path. */
            replacement_path = fs::impl::MakeUnique<char[]>(EntryNameLengthMax + 1);
            R_UNLESS(replacement_path != nullptr, fs::ResultAllocationFailureInNew());

            /* Replace the path. */
            ReplaceParentDirectoryPath(replacement_path.get(), path);

            /* Set path to be the replacement path. */
            path = replacement_path.get();
        }

        bool skip_next_sep = false;
        size_t i = 0;
        size_t len = prefix_len;

        while (!IsNullTerminator(path[i])) {
            if (IsSeparator(path[i])) {
                /* Swallow separators. */
                while (IsSeparator(path[++i])) { }
                if (IsNullTerminator(path[i])) {
                    break;
                }

                /* Handle skip if needed */
                if (!skip_next_sep) {
                    if (len + 1 == max_out_size) {
                        out[len] = StringTraits::NullTerminator;
                        *out_len = len;
                        return fs::ResultTooLongPath();
                    }

                    out[len++] = StringTraits::DirectorySeparator;
                }
                skip_next_sep = false;
            }

            /* See length of current dir. */
            size_t dir_len = 0;
            while (!IsSeparator(path[i + dir_len]) && !IsNullTerminator(path[i + dir_len])) {
                ++dir_len;
            }

            if (IsCurrentDirectory(path + i)) {
                skip_next_sep = true;
            } else if (IsParentDirectory(path + i)) {
                AMS_ASSERT(IsSeparator(out[len - 1]));
                if (!is_unc_path) {
                    AMS_ASSERT(IsSeparator(out[prefix_len]));
                }

                /* Walk up a directory. */
                if (len == prefix_len + 1) {
                    R_UNLESS(is_unc_path, fs::ResultDirectoryUnobtainable());
                    --len;
                } else {
                    len -= 2;
                    do {
                        if (IsSeparator(out[len])) {
                            break;
                        }
                        --len;
                    } while (len != prefix_len);
                }

                if (!is_unc_path) {
                    AMS_ASSERT(IsSeparator(out[prefix_len]));
                }

                AMS_ASSERT(len < max_out_size);
            } else {
                /* Copy, possibly truncating. */
                if (len + dir_len + 1 <= max_out_size) {
                    for (size_t j = 0; j < dir_len; ++j) {
                        out[len++] = path[i+j];
                    }
                } else {
                    const size_t copy_len = max_out_size - 1 - len;
                    for (size_t j = 0; j < copy_len; ++j) {
                        out[len++] = path[i+j];
                    }
                    out[len] = StringTraits::NullTerminator;
                    *out_len = len;
                    return fs::ResultTooLongPath();
                }
            }

            i += dir_len;
        }

        if (skip_next_sep) {
            --len;
        }

        if (!is_unc_path && len == prefix_len && max_out_size > len) {
            out[len++] = StringTraits::DirectorySeparator;
        }

        R_UNLESS(max_out_size >= len - 1, fs::ResultTooLongPath());

        /* Null terminate. */
        out[len] = StringTraits::NullTerminator;
        *out_len = len;

        /* Assert normalized. */
        {
            bool normalized = false;
            const auto is_norm_result = IsNormalized(std::addressof(normalized), out, unc_preserved, has_mount_name);
            AMS_ASSERT(R_SUCCEEDED(is_norm_result));
            AMS_ASSERT(normalized);
        }

        return ResultSuccess();
    }

    Result PathNormalizer::IsNormalized(bool *out, const char *path, bool unc_preserved, bool has_mount_name) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(path != nullptr);

        /* Save the start of the path. */
        const char *path_start = path;

        /* If we should, skip the mount name. */
        if (has_mount_name) {
            R_TRY(SkipMountName(std::addressof(path), path));
            R_UNLESS(IsSeparator(*path), fs::ResultInvalidPathFormat());
        }

        /* If we should, handle unc. */
        bool is_unc_path = false;
        if (unc_preserved) {
            path_start = path;

            /* Skip the windows path. */
            bool normalized_windows = false;
            R_TRY(SkipWindowsPath(std::addressof(path), std::addressof(normalized_windows), path, has_mount_name));

            /* If we're not windows-normalized, we're not normalized. */
            if (!normalized_windows) {
                *out = false;
                return ResultSuccess();
            }

            /* Handle the case where we're dealing with a unc path. */
            if (path != path_start) {
                is_unc_path = true;
                if (IsSeparator(path_start[0]) && IsSeparator(path_start[1])) {
                    *out = false;
                    return ResultSuccess();
                }

                if (IsNullTerminator(path[0])) {
                    *out = true;
                    return ResultSuccess();
                }
            }
        }

        /* Check if parent directory path replacement is needed. */
        if (IsParentDirectoryPathReplacementNeeded(path)) {
            *out = false;
            return ResultSuccess();
        }

        /* Nintendo uses a state machine here. */
        enum class PathState {
            Start,
            Normal,
            FirstSeparator,
            Separator,
            CurrentDir,
            ParentDir,
        };

        PathState state = PathState::Start;
        for (const char *cur = path; *cur != StringTraits::NullTerminator; ++cur) {
            const char c = *cur;
            switch (state) {
                case PathState::Start:
                    if (IsSeparator(c)) {
                        state = PathState::FirstSeparator;
                    } else {
                        R_UNLESS(path != path_start, fs::ResultInvalidPathFormat());
                        if (c == StringTraits::Dot) {
                            state = PathState::CurrentDir;
                        } else {
                            state = PathState::Normal;
                        }
                    }
                    break;
                case PathState::Normal:
                    if (IsSeparator(c)) {
                        state = PathState::Separator;
                    }
                    break;
                case PathState::FirstSeparator:
                case PathState::Separator:
                    if (IsSeparator(c)) {
                        *out = false;
                        return ResultSuccess();
                    } else if (c == StringTraits::Dot) {
                        state = PathState::CurrentDir;
                    } else {
                        state = PathState::Normal;
                    }
                    break;
                case PathState::CurrentDir:
                    if (IsSeparator(c)) {
                        *out = false;
                        return ResultSuccess();
                    } else if (c == StringTraits::Dot) {
                        state = PathState::ParentDir;
                    } else {
                        state = PathState::Normal;
                    }
                    break;
                case PathState::ParentDir:
                    if (IsSeparator(c)) {
                        *out = false;
                        return ResultSuccess();
                    } else {
                        state = PathState::Normal;
                    }
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        switch (state) {
            case PathState::Start:
                return fs::ResultInvalidPathFormat();
            case PathState::Normal:
                *out = true;
                break;
            case PathState::FirstSeparator:
                *out = !is_unc_path;
                break;
            case PathState::CurrentDir:
            case PathState::ParentDir:
            case PathState::Separator:
                *out = false;
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        return ResultSuccess();
    }

}
