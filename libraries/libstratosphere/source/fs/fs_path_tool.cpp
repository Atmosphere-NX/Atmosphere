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


    Result PathTool::Normalize(char *out, size_t *out_len, const char *src, size_t max_out_size, bool unc_preserved) {
        /* Paths must start with / */
        R_UNLESS(IsSeparator(src[0]), fs::ResultInvalidPathFormat());

        bool skip_next_sep = false;
        size_t i = 0;
        size_t len = 0;

        while (!IsNullTerminator(src[i])) {
            if (IsSeparator(src[i])) {
                /* Swallow separators. */
                while (IsSeparator(src[++i])) { }
                if (IsNullTerminator(src[i])) {
                    break;
                }

                /* Handle skip if needed */
                if (!skip_next_sep) {
                    if (len + 1 == max_out_size) {
                        out[len] = StringTraits::NullTerminator;
                        if (out_len != nullptr) {
                            *out_len = len;
                        }
                        return fs::ResultTooLongPath();
                    }

                    out[len++] = StringTraits::DirectorySeparator;

                    if (unc_preserved && len == 1) {
                        while (len < i) {
                            if (len + 1 == max_out_size) {
                                out[len] = StringTraits::NullTerminator;
                                if (out_len != nullptr) {
                                    *out_len = len;
                                }
                                return fs::ResultTooLongPath();
                            }
                            out[len++] = StringTraits::DirectorySeparator;
                        }
                    }
                }
                skip_next_sep = false;
            }

            /* See length of current dir. */
            size_t dir_len = 0;
            while (!IsSeparator(src[i+dir_len]) && !IsNullTerminator(src[i+dir_len])) {
                dir_len++;
            }

            if (IsCurrentDirectory(&src[i])) {
                skip_next_sep = true;
            } else if (IsParentDirectory(&src[i])) {
                AMS_ABORT_UNLESS(IsSeparator(out[0]));
                AMS_ABORT_UNLESS(IsSeparator(out[len - 1]));
                R_UNLESS(len != 1, fs::ResultDirectoryUnobtainable());

                /* Walk up a directory. */
                len -= 2;
                while (!IsSeparator(out[len])) {
                    len--;
                }
            } else {
                /* Copy, possibly truncating. */
                if (len + dir_len + 1 <= max_out_size) {
                    for (size_t j = 0; j < dir_len; j++) {
                        out[len++] = src[i+j];
                    }
                } else {
                    const size_t copy_len = max_out_size - 1 - len;
                    for (size_t j = 0; j < copy_len; j++) {
                        out[len++] = src[i+j];
                    }
                    out[len] = StringTraits::NullTerminator;
                    if (out_len != nullptr) {
                        *out_len = len;
                    }
                    return fs::ResultTooLongPath();
                }
            }

            i += dir_len;
        }

        if (skip_next_sep) {
            len--;
        }

        if (len == 0 && max_out_size) {
            out[len++] = StringTraits::DirectorySeparator;
        }

        R_UNLESS(max_out_size >= len - 1, fs::ResultTooLongPath());

        /* Null terminate. */
        out[len] = StringTraits::NullTerminator;
        if (out_len != nullptr) {
            *out_len = len;
        }

        /* Assert normalized. */
        bool normalized = false;
        AMS_ABORT_UNLESS(unc_preserved || (R_SUCCEEDED(IsNormalized(&normalized, out)) && normalized));

        return ResultSuccess();
    }

    Result PathTool::IsNormalized(bool *out, const char *path) {
        /* Nintendo uses a state machine here. */
        enum class PathState {
            Start,
            Normal,
            FirstSeparator,
            Separator,
            CurrentDir,
            ParentDir,
            WindowsDriveLetter,
        };

        PathState state = PathState::Start;

        for (const char *cur = path; *cur != StringTraits::NullTerminator; cur++) {
            const char c = *cur;
            switch (state) {
                case PathState::Start:
                    if (IsWindowsDriveCharacter(c)) {
                        state = PathState::WindowsDriveLetter;
                    } else if (IsSeparator(c)) {
                        state = PathState::FirstSeparator;
                    } else {
                        return fs::ResultInvalidPathFormat();
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
                    } else if (IsDot(c)) {
                        state = PathState::CurrentDir;
                    } else {
                        state = PathState::Normal;
                    }
                    break;
                case PathState::CurrentDir:
                    if (IsSeparator(c)) {
                        *out = false;
                        return ResultSuccess();
                    } else if (IsDot(c)) {
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
                case PathState::WindowsDriveLetter:
                    if (IsDriveSeparator(c)) {
                        *out = true;
                        return ResultSuccess();
                    } else {
                        return fs::ResultInvalidPathFormat();
                    }
                    break;
            }
        }

        switch (state) {
            case PathState::Start:
            case PathState::WindowsDriveLetter:
                return fs::ResultInvalidPathFormat();
            case PathState::FirstSeparator:
            case PathState::Normal:
                *out = true;
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

    bool PathTool::IsSubPath(const char *lhs, const char *rhs) {
        AMS_ABORT_UNLESS(lhs != nullptr);
        AMS_ABORT_UNLESS(rhs != nullptr);

        /* Special case certain paths. */
        if (IsSeparator(lhs[0]) && !IsSeparator(lhs[1]) && IsSeparator(rhs[0]) && IsSeparator(rhs[1])) {
            return false;
        }
        if (IsSeparator(rhs[0]) && !IsSeparator(rhs[1]) && IsSeparator(lhs[0]) && IsSeparator(lhs[1])) {
            return false;
        }
        if (IsSeparator(lhs[0]) && IsNullTerminator(lhs[1]) && IsSeparator(rhs[0]) && !IsNullTerminator(rhs[1])) {
            return true;
        }
        if (IsSeparator(rhs[0]) && IsNullTerminator(rhs[1]) && IsSeparator(lhs[0]) && !IsNullTerminator(lhs[1])) {
            return true;
        }

        /* Check subpath. */
        for (size_t i = 0; /* No terminating condition */; i++) {
            if (IsNullTerminator(lhs[i])) {
                return IsSeparator(rhs[i]);
            } else if (IsNullTerminator(rhs[i])) {
                return IsSeparator(lhs[i]);
            } else if (lhs[i] != rhs[i]) {
                return false;
            }
        }
    }

}
