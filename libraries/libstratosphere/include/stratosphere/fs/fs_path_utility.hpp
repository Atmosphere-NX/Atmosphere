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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_path.hpp>

namespace ams::fs {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    namespace StringTraits {

        constexpr inline char DirectorySeparator = '/';
        constexpr inline char DriveSeparator     = ':';
        constexpr inline char Dot                = '.';
        constexpr inline char NullTerminator     = '\x00';

        constexpr inline char AlternateDirectorySeparator = '\\';

        constexpr inline const char InvalidCharacters[6]             = { ':', '*', '?', '<', '>', '|' };
        constexpr inline const char InvalidCharactersForHostName[6]  = { ':', '*', '<', '>', '|', '$' };
        constexpr inline const char InvalidCharactersForMountName[5] = { '*', '?', '<', '>', '|' };

        namespace impl {

            template<const char *InvalidCharacterSet, size_t NumInvalidCharacters>
            consteval u64 MakeInvalidCharacterMask(size_t n) {
                u64 mask = 0;
                for (size_t i = 0; i < NumInvalidCharacters; ++i) {
                    if ((static_cast<u64>(InvalidCharacterSet[i]) >> 6) == n) {
                        mask |= static_cast<u64>(1) << (static_cast<u64>(InvalidCharacterSet[i]) & 0x3F);
                    }
                }
                return mask;
            }

            template<const char *InvalidCharacterSet, size_t NumInvalidCharacters>
            constexpr ALWAYS_INLINE bool IsInvalidCharacterImpl(char c) {
                constexpr u64 Masks[4] = {
                    MakeInvalidCharacterMask<InvalidCharacterSet, NumInvalidCharacters>(0),
                    MakeInvalidCharacterMask<InvalidCharacterSet, NumInvalidCharacters>(1),
                    MakeInvalidCharacterMask<InvalidCharacterSet, NumInvalidCharacters>(2),
                    MakeInvalidCharacterMask<InvalidCharacterSet, NumInvalidCharacters>(3)
                };

                return (Masks[static_cast<u64>(c) >> 6] & (static_cast<u64>(1) << (static_cast<u64>(c) & 0x3F))) != 0;
            }

        }

        constexpr ALWAYS_INLINE bool IsInvalidCharacter(char c)             { return impl::IsInvalidCharacterImpl<InvalidCharacters, util::size(InvalidCharacters)>(c); }
        constexpr ALWAYS_INLINE bool IsInvalidCharacterForHostName(char c)  { return impl::IsInvalidCharacterImpl<InvalidCharactersForHostName, util::size(InvalidCharactersForHostName)>(c); }
        constexpr ALWAYS_INLINE bool IsInvalidCharacterForMountName(char c) { return impl::IsInvalidCharacterImpl<InvalidCharactersForMountName, util::size(InvalidCharactersForMountName)>(c); }

    }

    constexpr inline size_t WindowsDriveLength        = 2;
    constexpr inline size_t UncPathPrefixLength       = 2;
    constexpr inline size_t DosDevicePathPrefixLength = 4;

    class PathFlags {
        private:
            static constexpr u32 WindowsPathFlag   = (1 << 0);
            static constexpr u32 RelativePathFlag  = (1 << 1);
            static constexpr u32 EmptyPathFlag     = (1 << 2);
            static constexpr u32 MountNameFlag     = (1 << 3);
            static constexpr u32 BackslashFlag     = (1 << 4);
            static constexpr u32 AllCharactersFlag = (1 << 5);
        private:
            u32 m_value;
        public:
            constexpr ALWAYS_INLINE PathFlags() : m_value(0) { /* ... */ }

            #define DECLARE_PATH_FLAG_HANDLER(__WHICH__)                                                                      \
                constexpr ALWAYS_INLINE bool Is ## __WHICH__ ##Allowed() const { return (m_value & __WHICH__ ## Flag) != 0; } \
                constexpr ALWAYS_INLINE void Allow ## __WHICH__ () { m_value |= __WHICH__ ## Flag; }

            DECLARE_PATH_FLAG_HANDLER(WindowsPath)
            DECLARE_PATH_FLAG_HANDLER(RelativePath)
            DECLARE_PATH_FLAG_HANDLER(EmptyPath)
            DECLARE_PATH_FLAG_HANDLER(MountName)
            DECLARE_PATH_FLAG_HANDLER(Backslash)
            DECLARE_PATH_FLAG_HANDLER(AllCharacters)

            #undef DECLARE_PATH_FLAG_HANDLER
    };

    template<typename T> requires (std::same_as<T, char> || std::same_as<T, wchar_t>)
    constexpr inline bool IsDosDevicePath(const T *path) {
        AMS_ASSERT(path != nullptr);

        using namespace StringTraits;

        return path[0] == AlternateDirectorySeparator && path[1] == AlternateDirectorySeparator && (path[2] == Dot || path[2] == '?') && (path[3] == DirectorySeparator || path[3] == AlternateDirectorySeparator);
    }

    template<typename T> requires (std::same_as<T, char> || std::same_as<T, wchar_t>)
    constexpr inline bool IsUncPath(const T *path, bool allow_forward_slash = true, bool allow_back_slash = true) {
        AMS_ASSERT(path != nullptr);

        using namespace StringTraits;

        return (allow_forward_slash && path[0] == DirectorySeparator && path[1] == DirectorySeparator) || (allow_back_slash && path[0] == AlternateDirectorySeparator && path[1] == AlternateDirectorySeparator);
    }

    constexpr inline bool IsWindowsDrive(const char *path) {
        AMS_ASSERT(path != nullptr);

        return (('a' <= path[0] && path[0] <= 'z') || ('A' <= path[0] && path[0] <= 'Z')) && path[1] == StringTraits::DriveSeparator;
    }

    constexpr inline bool IsWindowsPath(const char *path, bool allow_forward_slash_unc) {
        return IsWindowsDrive(path) || IsDosDevicePath(path) || IsUncPath(path, allow_forward_slash_unc, true);
    }

    constexpr inline int GetWindowsSkipLength(const char *path) {
        if (IsDosDevicePath(path)) {
            return DosDevicePathPrefixLength;
        } else if (IsWindowsDrive(path)) {
            return WindowsDriveLength;
        } else if (IsUncPath(path)) {
            return UncPathPrefixLength;
        } else {
            return 0;
        }
    }

    constexpr inline bool IsPathAbsolute(const char *path) {
        return IsWindowsPath(path, false) || path[0] == StringTraits::DirectorySeparator;
    }

    constexpr inline bool IsPathRelative(const char *path) {
        return path[0] && !IsPathAbsolute(path);
    }

    constexpr inline bool IsCurrentDirectory(const char *path) {
        return path[0] == StringTraits::Dot && (path[1] == StringTraits::NullTerminator || path[1] == StringTraits::DirectorySeparator);
    }

    constexpr inline bool IsParentDirectory(const char *path) {
        return path[0] == StringTraits::Dot && path[1] == StringTraits::Dot && (path[2] == StringTraits::NullTerminator || path[2] == StringTraits::DirectorySeparator);
    }

    constexpr inline bool IsPathStartWithCurrentDirectory(const char *path) {
        return IsCurrentDirectory(path) || IsParentDirectory(path);
    }

    constexpr inline bool IsSubPath(const char *lhs, const char *rhs) {
        /* Check pre-conditions. */
        AMS_ASSERT(lhs != nullptr);
        AMS_ASSERT(rhs != nullptr);

        /* Import StringTraits names for current scope. */
        using namespace StringTraits;

        /* Special case certain paths. */
        if (IsUncPath(lhs) && !IsUncPath(rhs)) {
            return false;
        }
        if (!IsUncPath(lhs) && IsUncPath(rhs)) {
            return false;
        }

        if (lhs[0] == DirectorySeparator && lhs[1] == NullTerminator && rhs[0] == DirectorySeparator && rhs[1] != NullTerminator) {
            return true;
        }
        if (rhs[0] == DirectorySeparator && rhs[1] == NullTerminator && lhs[0] == DirectorySeparator && lhs[1] != NullTerminator) {
            return true;
        }

        /* Check subpath. */
        for (size_t i = 0; /* ... */; ++i) {
            if (lhs[i] == NullTerminator) {
                return rhs[i] == DirectorySeparator;
            } else if (rhs[i] == NullTerminator) {
                return lhs[i] == DirectorySeparator;
            } else if (lhs[i] != rhs[i]) {
                return false;
            }
        }
    }

    /* Path utilities. */
    constexpr inline void Replace(char *dst, size_t dst_size, char old_char, char new_char) {
        AMS_ASSERT(dst != nullptr);
        for (char *cur = dst; cur < dst + dst_size && *cur; ++cur) {
            if (*cur == old_char) {
                *cur = new_char;
            }
        }
    }

    constexpr inline Result CheckUtf8(const char *s) {
        /* Check pre-conditions. */
        AMS_ASSERT(s != nullptr);

        /* Iterate, checking for utf8-validity. */
        while (*s) {
            char utf8_buf[4] = {};

            const auto pick_res = util::PickOutCharacterFromUtf8String(utf8_buf, std::addressof(s));
            R_UNLESS(pick_res == util::CharacterEncodingResult_Success, fs::ResultInvalidPathFormat());

            u32 dummy;
            const auto cvt_res = util::ConvertCharacterUtf8ToUtf32(std::addressof(dummy), utf8_buf);
            R_UNLESS(cvt_res == util::CharacterEncodingResult_Success, fs::ResultInvalidPathFormat());
        }

        R_SUCCEED();
    }

    /* Path formatting. */
    class PathNormalizer {
        private:
            enum class PathState {
                Start,
                Normal,
                FirstSeparator,
                Separator,
                CurrentDir,
                ParentDir,
            };
        private:
            static constexpr void ReplaceParentDirectoryPath(char *dst, const char *src) {
                /* Use StringTraits names for remainder of scope. */
                using namespace StringTraits;

                /* Start with a dir-separator. */
                dst[0] = DirectorySeparator;

                auto i = 1;
                while (src[i] != NullTerminator) {
                    if ((src[i - 1] == DirectorySeparator || src[i - 1] == AlternateDirectorySeparator) && src[i + 0] == Dot && src[i + 1] == Dot && (src[i + 2] == DirectorySeparator || src[i + 2] == AlternateDirectorySeparator)) {
                        dst[i - 1] = DirectorySeparator;
                        dst[i + 0] = Dot;
                        dst[i + 1] = Dot;
                        dst[i + 2] = DirectorySeparator;
                        i += 3;
                    } else {
                        if (src[i - 1] == AlternateDirectorySeparator && src[i + 0] == Dot && src[i + 1] == Dot && src[i + 2] == NullTerminator) {
                            dst[i - 1] = DirectorySeparator;
                            dst[i + 0] = Dot;
                            dst[i + 1] = Dot;
                            i += 2;
                            break;
                        }

                        dst[i] = src[i];
                        ++i;
                    }
                }

                dst[i] = StringTraits::NullTerminator;
            }
        public:
            static constexpr bool IsParentDirectoryPathReplacementNeeded(const char *path) {
                /* Use StringTraits names for remainder of scope. */
                using namespace StringTraits;

                if (path[0] != DirectorySeparator && path[0] != AlternateDirectorySeparator) {
                    return false;
                }

                /* Check to find a parent reference using alternate separators. */
                if (path[0] != NullTerminator && path[1] != NullTerminator && path[2] != NullTerminator) {
                    size_t i;
                    for (i = 0; path[i + 3] != NullTerminator; ++path) {
                        if (path[i + 1] != Dot || path[i + 2] != Dot) {
                            continue;
                        }

                        const char c0 = path[i + 0];
                        const char c3 = path[i + 3];

                        if (c0 == AlternateDirectorySeparator && (c3 == DirectorySeparator || c3 == AlternateDirectorySeparator || c3 == NullTerminator)) {
                            return true;
                        }

                        if (c3 == AlternateDirectorySeparator && (c0 == DirectorySeparator || c0 == AlternateDirectorySeparator)) {
                            return true;
                        }
                    }

                    if (path[i + 0] == AlternateDirectorySeparator && path[i + 1] == Dot && path[i + 2] == Dot /* && path[i + 3] == NullTerminator */) {
                        return true;
                    }
                }

                return false;
            }

            static constexpr Result IsNormalized(bool *out, size_t *out_len, const char *path, bool allow_all_characters = false) {
                /* Use StringTraits names for remainder of scope. */
                using namespace StringTraits;

                /* Parse the path. */
                auto state = PathState::Start;
                size_t len = 0;
                while (path[len] != NullTerminator) {
                    /* Get the current character. */
                    const char c = path[len++];

                    /* Check the current character is valid. */
                    if (!allow_all_characters && state != PathState::Start) {
                        R_UNLESS(!IsInvalidCharacter(c), fs::ResultInvalidCharacter());
                    }

                    /* Process depending on current state. */
                    switch (state) {
                        /* Import the PathState enums for convenience. */
                        using enum PathState;

                        case Start:
                            R_UNLESS(c == DirectorySeparator, fs::ResultInvalidPathFormat());
                            state = FirstSeparator;
                            break;
                        case Normal:
                            if (c == DirectorySeparator) {
                                state = Separator;
                            }
                            break;
                        case FirstSeparator:
                        case Separator:
                            if (c == DirectorySeparator) {
                                *out = false;
                                R_SUCCEED();
                            }

                            if (c == Dot) {
                                state = CurrentDir;
                            } else {
                                state = Normal;
                            }
                            break;
                        case CurrentDir:
                            if (c == DirectorySeparator) {
                                *out = false;
                                R_SUCCEED();
                            }

                            if (c == Dot) {
                                state = ParentDir;
                            } else {
                                state = Normal;
                            }
                            break;
                        case ParentDir:
                            if (c == DirectorySeparator) {
                                *out = false;
                                R_SUCCEED();
                            }

                            state = Normal;
                            break;
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }
                }

                /* Check the final state. */
                switch (state) {
                    /* Import the PathState enums for convenience. */
                    using enum PathState;
                    case Start:
                        R_THROW(fs::ResultInvalidPathFormat());
                    case Normal:
                    case FirstSeparator:
                        *out = true;
                        break;
                    case Separator:
                    case CurrentDir:
                    case ParentDir:
                        *out = false;
                        break;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }

                /* Set the output length. */
                *out_len = len;
                R_SUCCEED();
            }

            static constexpr Result Normalize(char *dst, size_t *out_len, const char *path, size_t max_out_size, bool is_windows_path, bool is_drive_relative_path, bool allow_all_characters = false) {
                /* Use StringTraits names for remainder of scope. */
                using namespace StringTraits;

                /* Prepare to iterate. */
                const char *cur_path = path;
                size_t total_len = 0;

                /* If path begins with a separator, check that we're not drive relative. */
                if (cur_path[0] != DirectorySeparator) {
                    R_UNLESS(is_drive_relative_path, fs::ResultInvalidPathFormat());

                    dst[total_len++] = DirectorySeparator;
                }

                /* We're going to need to do path replacement, potentially. */
                char *replacement_path = nullptr;
                size_t replacement_path_size = 0;
                ON_SCOPE_EXIT {
                    if (replacement_path != nullptr) {
                        if (std::is_constant_evaluated()) {
                            delete[] replacement_path;
                        } else {
                            ::ams::fs::impl::Deallocate(replacement_path, replacement_path_size);
                        }
                    }
                };

                /* Perform path replacement, if necessary. */
                if (IsParentDirectoryPathReplacementNeeded(cur_path)) {
                    if (std::is_constant_evaluated()) {
                        replacement_path_size = fs::EntryNameLengthMax + 1;
                        replacement_path      = new char[replacement_path_size];
                    } else {
                        replacement_path_size = fs::EntryNameLengthMax + 1;
                        replacement_path      = static_cast<char *>(::ams::fs::impl::Allocate(replacement_path_size));
                    }

                    ReplaceParentDirectoryPath(replacement_path, cur_path);

                    cur_path = replacement_path;
                }

                /* Iterate, normalizing path components. */
                bool skip_next_sep = false;
                size_t i = 0;

                while (cur_path[i] != NullTerminator) {
                    /* Process a directory separator, if we run into one. */
                    if (cur_path[i] == DirectorySeparator) {
                        /* Swallow separators. */
                        do { ++i; } while (cur_path[i] == DirectorySeparator);

                        /* Check if we hit end of string. */
                        if (cur_path[i] == NullTerminator) {
                            break;
                        }

                        /* If we aren't skipping the separator, write it, checking that we remain in bounds. */
                        if (!skip_next_sep) {
                            if (total_len + 1 == max_out_size) {
                                dst[total_len] = NullTerminator;
                                *out_len = total_len;
                                R_THROW(fs::ResultTooLongPath());
                            }

                            dst[total_len++] = DirectorySeparator;
                        }

                        /* Don't skip the next separator. */
                        skip_next_sep = false;
                    }

                    /* Get the length of the current directory component. */
                    size_t dir_len = 0;
                    while (cur_path[i + dir_len] != DirectorySeparator && cur_path[i + dir_len] != NullTerminator) {
                        /* Check for validity. */
                        if (!allow_all_characters) {
                            R_UNLESS(!IsInvalidCharacter(cur_path[i + dir_len]), fs::ResultInvalidCharacter());
                        }

                        ++dir_len;
                    }

                    /* Handle the current dir component. */
                    if (IsCurrentDirectory(cur_path + i)) {
                        skip_next_sep = true;
                    } else if (IsParentDirectory(cur_path + i)) {
                        /* We should have just written a separator. */
                        AMS_ASSERT(dst[total_len - 1] == DirectorySeparator);

                        /* We should have started with a separator, for non-windows paths. */
                        if (!is_windows_path) {
                            AMS_ASSERT(dst[0] == DirectorySeparator);
                        }

                        /* Remove the previous component. */
                        if (total_len == 1) {
                            R_UNLESS(is_windows_path, fs::ResultDirectoryUnobtainable());

                            --total_len;
                        } else {
                            total_len -= 2;

                            do {
                                if (dst[total_len] == DirectorySeparator) {
                                    break;
                                }
                            } while ((--total_len) != 0);
                        }

                        /* We should be pointing to a directory separator, for non-windows paths. */
                        if (!is_windows_path) {
                            AMS_ASSERT(dst[total_len] == DirectorySeparator);
                        }

                        /* We should remain in bounds. */
                        AMS_ASSERT(total_len < max_out_size);
                    } else {
                        /* Copy, possibly truncating. */
                        if (total_len + dir_len + 1 > max_out_size) {
                            const size_t copy_len = max_out_size - (total_len + 1);

                            for (size_t j = 0; j < copy_len; ++j) {
                                dst[total_len++] = cur_path[i + j];
                            }

                            dst[total_len] = NullTerminator;
                            *out_len       = total_len;
                            R_THROW(fs::ResultTooLongPath());
                        }

                        for (size_t j = 0; j < dir_len; ++j) {
                            dst[total_len++] = cur_path[i + j];
                        }
                    }

                    /* Advance past the current directory component. */
                    i += dir_len;
                }

                if (skip_next_sep) {
                    --total_len;
                }

                if (total_len == 0 && max_out_size != 0) {
                    total_len = 1;
                    dst[0] = DirectorySeparator;
                }

                /* NOTE: Probable nintendo bug, as max_out_size must be at least total_len + 1 for the null terminator. */
                R_UNLESS(max_out_size >= total_len - 1, fs::ResultTooLongPath());

                dst[total_len] = NullTerminator;

                /* Check that the result path is normalized. */
                bool is_normalized;
                size_t dummy;
                R_TRY(IsNormalized(std::addressof(is_normalized), std::addressof(dummy), dst, allow_all_characters));

                /* Assert that the result path is normalized. */
                AMS_ASSERT(is_normalized);

                /* Set the output length. */
                *out_len = total_len;
                R_SUCCEED();
            }
    };

    class PathFormatter {
        private:
            static constexpr ALWAYS_INLINE Result CheckSharedName(const char *name, size_t len) {
                /* Use StringTraits names for remainder of scope. */
                using namespace StringTraits;

                if (len == 1) {
                    R_UNLESS(name[0] != Dot, fs::ResultInvalidPathFormat());
                } else if (len == 2) {
                    R_UNLESS(name[0] != Dot || name[1] != Dot, fs::ResultInvalidPathFormat());
                }

                for (size_t i = 0; i < len; ++i) {
                    R_UNLESS(!IsInvalidCharacter(name[i]), fs::ResultInvalidCharacter());
                }

                R_SUCCEED();
            }

            static constexpr ALWAYS_INLINE Result CheckHostName(const char *name, size_t len) {
                /* Use StringTraits names for remainder of scope. */
                using namespace StringTraits;

                if (len == 2) {
                    R_UNLESS(name[0] != Dot || name[1] != Dot, fs::ResultInvalidPathFormat());
                }

                for (size_t i = 0; i < len; ++i) {
                    R_UNLESS(!IsInvalidCharacterForHostName(name[i]), fs::ResultInvalidCharacter());
                }

                R_SUCCEED();
            }

            static constexpr Result CheckInvalidBackslash(bool *out_contains_backslash, const char *path, bool allow_backslash) {
                /* Use StringTraits names for remainder of scope. */
                using namespace StringTraits;

                /* Default to no backslashes, so we can just write if we see one. */
                *out_contains_backslash = false;

                while (*path != NullTerminator) {
                    if (*(path++) == AlternateDirectorySeparator) {
                        *out_contains_backslash = true;

                        R_UNLESS(allow_backslash, fs::ResultInvalidCharacter());
                    }
                }

                R_SUCCEED();
            }
        public:
            static constexpr ALWAYS_INLINE Result CheckPathFormat(const char *path, const PathFlags &flags) {
                bool normalized;
                size_t len;
                R_RETURN(IsNormalized(std::addressof(normalized), std::addressof(len), path, flags));
            }

            static constexpr ALWAYS_INLINE Result SkipMountName(const char **out, size_t *out_len, const char *path) {
                R_RETURN(ParseMountName(out, out_len, nullptr, 0, path));
            }

            static constexpr Result ParseMountName(const char **out, size_t *out_len, char *out_mount_name, size_t out_mount_name_buffer_size, const char *path) {
                /* Check pre-conditions. */
                AMS_ASSERT(path != nullptr);
                AMS_ASSERT(out_len != nullptr);
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT((out_mount_name == nullptr) == (out_mount_name_buffer_size == 0));

                /* Use StringTraits names for remainder of scope. */
                using namespace StringTraits;

                /* Determine max mount length. */
                const auto max_mount_len = out_mount_name_buffer_size == 0 ? MountNameLengthMax + 1 : std::min(MountNameLengthMax + 1, out_mount_name_buffer_size);

                /* Parse the path until we see a drive separator. */
                size_t mount_len = 0;
                for (/* ... */; mount_len < max_mount_len && path[mount_len]; ++mount_len) {
                    const char c = path[mount_len];

                    /* If we see a drive separator, advance, then we're done with the pre-drive separator part of the mount. */
                    if (c == DriveSeparator) {
                        ++mount_len;
                        break;
                    }

                    /* If we see a directory separator, we're not in a mount name. */
                    if (c == DirectorySeparator || c == AlternateDirectorySeparator) {
                        *out     = path;
                        *out_len = 0;
                        R_SUCCEED();
                    }
                }


                /* Check to be sure we're actually looking at a mount name. */
                if (mount_len <= 2 || path[mount_len - 1] != DriveSeparator) {
                    *out     = path;
                    *out_len = 0;
                    R_SUCCEED();
                }

                /* Check that all characters in the mount name are allowable. */
                for (size_t i = 0; i < mount_len; ++i) {
                    R_UNLESS(!IsInvalidCharacterForMountName(path[i]), fs::ResultInvalidCharacter());
                }

                /* Copy out the mount name. */
                if (out_mount_name_buffer_size > 0) {
                    R_UNLESS(mount_len < out_mount_name_buffer_size, fs::ResultTooLongPath());

                    for (size_t i = 0; i < mount_len; ++i) {
                        out_mount_name[i] = path[i];
                    }
                    out_mount_name[mount_len] = NullTerminator;
                }

                /* Set the output. */
                *out     = path + mount_len;
                *out_len = mount_len;
                R_SUCCEED();
            }

            static constexpr ALWAYS_INLINE Result SkipRelativeDotPath(const char **out, size_t *out_len, const char *path) {
                R_RETURN(ParseRelativeDotPath(out, out_len, nullptr, 0, path));
            }

            static constexpr Result ParseRelativeDotPath(const char **out, size_t *out_len, char *out_relative, size_t out_relative_buffer_size, const char *path) {
                /* Check pre-conditions. */
                AMS_ASSERT(path != nullptr);
                AMS_ASSERT(out_len != nullptr);
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT((out_relative == nullptr) == (out_relative_buffer_size == 0));

                /* Use StringTraits names for remainder of scope. */
                using namespace StringTraits;

                /* Initialize the output buffer, if we have one. */
                if (out_relative_buffer_size > 0) {
                    out_relative[0] = NullTerminator;
                }

                /* Check if the path is relative. */
                if (path[0] == Dot && (path[1] == NullTerminator || path[1] == DirectorySeparator || path[1] == AlternateDirectorySeparator)) {
                    if (out_relative_buffer_size > 0) {
                        R_UNLESS(out_relative_buffer_size >= 2, fs::ResultTooLongPath());

                        out_relative[0] = Dot;
                        out_relative[1] = NullTerminator;
                    }

                    *out     = path + 1;
                    *out_len = 1;
                    R_SUCCEED();
                }

                /* Ensure the path isn't a parent directory. */
                R_UNLESS(!(path[0] == Dot && path[1] == Dot), fs::ResultDirectoryUnobtainable());

                /* There was no relative dot path. */
                *out = path;
                *out_len = 0;
                R_SUCCEED();
            }

            static constexpr Result SkipWindowsPath(const char **out, size_t *out_len, bool *out_normalized, const char *path, bool has_mount_name) {
                /* We're normalized if and only if the parsing doesn't throw ResultNotNormalized(). */
                *out_normalized = true;

                R_TRY_CATCH(ParseWindowsPath(out, out_len, nullptr, 0, path, has_mount_name)) {
                    R_CATCH(fs::ResultNotNormalized) { *out_normalized = false; }
                } R_END_TRY_CATCH;
                ON_RESULT_INCLUDED(fs::ResultNotNormalized) { *out_normalized = false; };

                R_SUCCEED();
            }

            static constexpr Result ParseWindowsPath(const char **out, size_t *out_len, char *out_win, size_t out_win_buffer_size, const char *path, bool has_mount_name) {
                /* Check pre-conditions. */
                AMS_ASSERT(path != nullptr);
                AMS_ASSERT(out_len != nullptr);
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT((out_win == nullptr) == (out_win_buffer_size == 0));

                /* Use StringTraits names for remainder of scope. */
                using namespace StringTraits;

                /* Initialize the output buffer, if we have one. */
                if (out_win_buffer_size > 0) {
                    out_win[0] = NullTerminator;
                }

                /* Handle path start. */
                const char *cur_path = path;
                if (has_mount_name && path[0] == DirectorySeparator) {
                    if (path[1] == AlternateDirectorySeparator && path[2] == AlternateDirectorySeparator) {
                        R_UNLESS(out_win_buffer_size > 0, fs::ResultNotNormalized());

                        ++cur_path;
                    } else if (IsWindowsDrive(path + 1)) {
                        R_UNLESS(out_win_buffer_size > 0, fs::ResultNotNormalized());

                        ++cur_path;
                    }
                }

                /* Handle windows drive. */
                if (IsWindowsDrive(cur_path)) {
                    /* Parse up to separator. */
                    size_t win_path_len = WindowsDriveLength;
                    for (/* ... */; cur_path[win_path_len] != NullTerminator; ++win_path_len) {
                        R_UNLESS(!IsInvalidCharacter(cur_path[win_path_len]), fs::ResultInvalidCharacter());

                        if (cur_path[win_path_len] == DirectorySeparator || cur_path[win_path_len] == AlternateDirectorySeparator) {
                            break;
                        }
                    }

                    /* Ensure that we're normalized, if we're required to be. */
                    if (out_win_buffer_size == 0) {
                        for (size_t i = 0; i < win_path_len; ++i) {
                            R_UNLESS(cur_path[i] != AlternateDirectorySeparator, fs::ResultNotNormalized());
                        }
                    } else {
                        /* Ensure we can copy into the normalized buffer. */
                        R_UNLESS(win_path_len < out_win_buffer_size, fs::ResultTooLongPath());

                        for (size_t i = 0; i < win_path_len; ++i) {
                            out_win[i] = cur_path[i];
                        }
                        out_win[win_path_len] = NullTerminator;

                        fs::Replace(out_win, win_path_len, AlternateDirectorySeparator, DirectorySeparator);
                    }

                    *out     = cur_path + win_path_len;
                    *out_len = win_path_len;
                    R_SUCCEED();
                }

                /* Handle DOS device. */
                if (IsDosDevicePath(cur_path)) {
                    size_t dos_prefix_len = DosDevicePathPrefixLength;

                    if (IsWindowsDrive(cur_path + dos_prefix_len)) {
                        dos_prefix_len += WindowsDriveLength;
                    } else {
                        --dos_prefix_len;
                    }

                    if (out_win_buffer_size > 0) {
                        /* Ensure we can copy into the normalized buffer. */
                        R_UNLESS(dos_prefix_len < out_win_buffer_size, fs::ResultTooLongPath());

                        for (size_t i = 0; i < dos_prefix_len; ++i) {
                            out_win[i] = cur_path[i];
                        }
                        out_win[dos_prefix_len] = NullTerminator;

                        fs::Replace(out_win, dos_prefix_len, DirectorySeparator, AlternateDirectorySeparator);
                    }

                    *out     = cur_path + dos_prefix_len;
                    *out_len = dos_prefix_len;
                    R_SUCCEED();
                }

                /* Handle UNC path. */
                if (IsUncPath(cur_path, false, true)) {
                    const char *final_path = cur_path;

                    R_UNLESS(cur_path[UncPathPrefixLength] != DirectorySeparator,          fs::ResultInvalidPathFormat());
                    R_UNLESS(cur_path[UncPathPrefixLength] != AlternateDirectorySeparator, fs::ResultInvalidPathFormat());

                    size_t cur_component_offset = 0;
                    size_t pos = UncPathPrefixLength;
                    for (/* ... */; cur_path[pos] != NullTerminator; ++pos) {
                        if (cur_path[pos] == DirectorySeparator || cur_path[pos] == AlternateDirectorySeparator) {
                            if (cur_component_offset != 0) {
                                R_TRY(CheckSharedName(cur_path + cur_component_offset, pos - cur_component_offset));

                                final_path = cur_path + pos;
                                break;
                            }

                            R_UNLESS(cur_path[pos + 1] != DirectorySeparator,          fs::ResultInvalidPathFormat());
                            R_UNLESS(cur_path[pos + 1] != AlternateDirectorySeparator, fs::ResultInvalidPathFormat());

                            R_TRY(CheckHostName(cur_path + 2, pos - 2));

                            cur_component_offset = pos + 1;
                        }
                    }

                    R_UNLESS(cur_component_offset != pos, fs::ResultInvalidPathFormat());

                    if (cur_component_offset != 0 && final_path == cur_path) {
                        R_TRY(CheckSharedName(cur_path + cur_component_offset, pos - cur_component_offset));

                        final_path = cur_path + pos;
                    }

                    size_t unc_prefix_len = final_path - cur_path;

                    /* Ensure that we're normalized, if we're required to be. */
                    if (out_win_buffer_size == 0) {
                        for (size_t i = 0; i < unc_prefix_len; ++i) {
                            R_UNLESS(cur_path[i] != DirectorySeparator, fs::ResultNotNormalized());
                        }
                    } else {
                        /* Ensure we can copy into the normalized buffer. */
                        R_UNLESS(unc_prefix_len < out_win_buffer_size, fs::ResultTooLongPath());

                        for (size_t i = 0; i < unc_prefix_len; ++i) {
                            out_win[i] = cur_path[i];
                        }
                        out_win[unc_prefix_len] = NullTerminator;

                        fs::Replace(out_win, unc_prefix_len, DirectorySeparator, AlternateDirectorySeparator);
                    }

                    *out     = cur_path + unc_prefix_len;
                    *out_len = unc_prefix_len;
                    R_SUCCEED();
                }

                /* There's no windows path to parse. */
                *out     = path;
                *out_len = 0;
                R_SUCCEED();
            }

            static constexpr Result IsNormalized(bool *out, size_t *out_len, const char *path, const PathFlags &flags = {}) {
                /* Ensure nothing is null. */
                R_UNLESS(out != nullptr,     fs::ResultNullptrArgument());
                R_UNLESS(out_len != nullptr, fs::ResultNullptrArgument());
                R_UNLESS(path != nullptr,    fs::ResultNullptrArgument());

                /* Verify that the path is valid utf-8. */
                R_TRY(fs::CheckUtf8(path));

                /* Use StringTraits names for remainder of scope. */
                using namespace StringTraits;

                /* Handle the case where the path is empty. */
                if (path[0] == NullTerminator) {
                    R_UNLESS(flags.IsEmptyPathAllowed(), fs::ResultInvalidPathFormat());

                    *out = true;
                    *out_len = 0;
                    R_SUCCEED();
                }

                /* All normalized paths start with a directory separator...unless they're windows paths, relative paths, or have mount names. */
                if (path[0] != DirectorySeparator) {
                    R_UNLESS(flags.IsWindowsPathAllowed() || flags.IsRelativePathAllowed() || flags.IsMountNameAllowed(), fs::ResultInvalidPathFormat());
                }

                /* Check that the path is allowed to be a windows path, if it is. */
                if (fs::IsWindowsPath(path, false)) {
                    R_UNLESS(flags.IsWindowsPathAllowed(), fs::ResultInvalidPathFormat());
                }

                /* Skip past the mount name, if one is present. */
                size_t total_len      = 0;
                size_t mount_name_len = 0;
                R_TRY(SkipMountName(std::addressof(path), std::addressof(mount_name_len), path));

                /* If we had a mount name, check that that was allowed. */
                if (mount_name_len > 0) {
                    R_UNLESS(flags.IsMountNameAllowed(), fs::ResultInvalidPathFormat());

                    total_len += mount_name_len;
                }

                /* Check that the path starts as a normalized path should. */
                if (path[0] != DirectorySeparator && !IsPathStartWithCurrentDirectory(path) && !IsWindowsPath(path, false)) {
                    R_UNLESS(flags.IsRelativePathAllowed(), fs::ResultInvalidPathFormat());
                    R_UNLESS(!IsInvalidCharacter(path[0]),  fs::ResultInvalidPathFormat());

                    *out = false;
                    R_SUCCEED();
                }

                /* Process relative path. */
                size_t relative_len = 0;
                R_TRY(SkipRelativeDotPath(std::addressof(path), std::addressof(relative_len), path));

                /* If we have a relative path, check that was allowed. */
                if (relative_len > 0) {
                    R_UNLESS(flags.IsRelativePathAllowed(), fs::ResultInvalidPathFormat());

                    total_len += relative_len;

                    if (path[0] == NullTerminator) {
                        *out = true;
                        *out_len = total_len;
                        R_SUCCEED();
                    }
                }

                /* Process windows path. */
                size_t windows_len = 0;
                bool normalized_win = false;
                R_TRY(SkipWindowsPath(std::addressof(path), std::addressof(windows_len), std::addressof(normalized_win), path, mount_name_len > 0));

                /* If the windows path wasn't normalized, we're not normalized. */
                if (!normalized_win) {
                    R_UNLESS(flags.IsWindowsPathAllowed(), fs::ResultInvalidPathFormat());

                    *out = false;
                    R_SUCCEED();
                }

                /* If we had a windows path, check that was allowed. */
                if (windows_len > 0) {
                    R_UNLESS(flags.IsWindowsPathAllowed(), fs::ResultInvalidPathFormat());

                    total_len += windows_len;

                    /* We can't have both a relative path and a windows path. */
                    R_UNLESS(relative_len == 0, fs::ResultInvalidPathFormat());

                    /* A path ending in a windows path isn't normalized. */
                    if (path[0] == NullTerminator) {
                        *out = false;
                        R_SUCCEED();
                    }

                    /* Check that there are no windows directory separators in the path. */
                    for (size_t i = 0; path[i] != NullTerminator; ++i) {
                        if (path[i] == AlternateDirectorySeparator) {
                            *out = false;
                            R_SUCCEED();
                        }
                    }
                }

                /* Check that parent directory replacement is not needed if backslashes are allowed. */
                if (flags.IsBackslashAllowed() && PathNormalizer::IsParentDirectoryPathReplacementNeeded(path)) {
                    *out = false;
                    R_SUCCEED();
                }

                /* Check that the backslash state is valid. */
                bool is_backslash_contained = false;
                R_TRY(CheckInvalidBackslash(std::addressof(is_backslash_contained), path, flags.IsWindowsPathAllowed() || flags.IsBackslashAllowed()));

                /* Check that backslashes are contained only if allowed. */
                if (is_backslash_contained && !flags.IsBackslashAllowed()) {
                    *out = false;
                    R_SUCCEED();
                }

                /* Check that the final result path is normalized. */
                size_t normal_len = 0;
                R_TRY(PathNormalizer::IsNormalized(out, std::addressof(normal_len), path, flags.IsAllCharactersAllowed()));

                /* Add the normal length. */
                total_len += normal_len;

                /* Set the output length. */
                *out_len = total_len;
                R_SUCCEED();
            }

            static constexpr Result Normalize(char *dst, size_t dst_size, const char *path, size_t path_len, const PathFlags &flags) {
                /* Use StringTraits names for remainder of scope. */
                using namespace StringTraits;

                /* Prepare to iterate. */
                const char *src = path;
                size_t cur_pos = 0;
                bool is_windows_path = false;

                /* Check if the path is empty. */
                if (src[0] == NullTerminator) {
                    if (dst_size != 0) {
                        dst[0] = NullTerminator;
                    }

                    R_UNLESS(flags.IsEmptyPathAllowed(), fs::ResultInvalidPathFormat());

                    R_SUCCEED();
                }

                /* Handle a mount name. */
                size_t mount_name_len = 0;
                if (flags.IsMountNameAllowed()) {
                    R_TRY(ParseMountName(std::addressof(src), std::addressof(mount_name_len), dst + cur_pos, dst_size - cur_pos, src));

                    cur_pos += mount_name_len;
                }

                /* Handle a drive-relative prefix. */
                bool is_drive_relative = false;
                if (src[0] != DirectorySeparator && !IsPathStartWithCurrentDirectory(src) && !IsWindowsPath(src, false)) {
                    R_UNLESS(flags.IsRelativePathAllowed(), fs::ResultInvalidPathFormat());
                    R_UNLESS(!IsInvalidCharacter(src[0]),   fs::ResultInvalidPathFormat());

                    dst[cur_pos++] = Dot;
                    is_drive_relative = true;
                }

                size_t relative_len = 0;
                if (flags.IsRelativePathAllowed()) {
                    R_UNLESS(cur_pos < dst_size, fs::ResultTooLongPath());

                    R_TRY(ParseRelativeDotPath(std::addressof(src), std::addressof(relative_len), dst + cur_pos, dst_size - cur_pos, src));

                    cur_pos += relative_len;

                    if (src[0] == NullTerminator) {
                        R_UNLESS(cur_pos < dst_size, fs::ResultTooLongPath());

                        dst[cur_pos] = NullTerminator;
                        R_SUCCEED();
                    }
                }

                /* Handle a windows path. */
                if (flags.IsWindowsPathAllowed()) {
                    const char * const orig = src;

                    R_UNLESS(cur_pos < dst_size, fs::ResultTooLongPath());

                    size_t windows_len = 0;
                    R_TRY(ParseWindowsPath(std::addressof(src), std::addressof(windows_len), dst + cur_pos, dst_size - cur_pos, src, mount_name_len != 0));

                    cur_pos += windows_len;

                    if (src[0] == NullTerminator) {
                        /* NOTE: Bug in original code here repeated, should be checking cur_pos + 2. */
                        R_UNLESS(cur_pos + 1 < dst_size, fs::ResultTooLongPath());

                        dst[cur_pos + 0] = DirectorySeparator;
                        dst[cur_pos + 1] = NullTerminator;
                        R_SUCCEED();
                    }

                    if ((src - orig) > 0) {
                        is_windows_path = true;
                    }
                }

                /* Check for invalid backslash. */
                bool backslash_contained = false;
                R_TRY(CheckInvalidBackslash(std::addressof(backslash_contained), src, flags.IsWindowsPathAllowed() || flags.IsBackslashAllowed()));

                /* Handle backslash replacement as necessary. */
                if (backslash_contained && flags.IsWindowsPathAllowed()) {
                    /* Create a temporary buffer holding a slash-replaced version of the path. */
                    /* NOTE: Nintendo unnecessarily allocates and replaces here a fully copy of the path, despite having skipped some of it already. */
                    const size_t replaced_src_len = path_len - (src - path);

                    char *replaced_src = nullptr;
                    ON_SCOPE_EXIT {
                        if (replaced_src != nullptr) {
                            if (std::is_constant_evaluated()) {
                                delete[] replaced_src;
                            } else {
                                ::ams::fs::impl::Deallocate(replaced_src, replaced_src_len);
                            }
                        }
                    };

                    if (std::is_constant_evaluated()) {
                        replaced_src = new char[replaced_src_len];
                    } else {
                        replaced_src = static_cast<char *>(::ams::fs::impl::Allocate(replaced_src_len));
                    }

                    util::Strlcpy<char>(replaced_src, src, replaced_src_len);

                    fs::Replace(replaced_src, replaced_src_len, AlternateDirectorySeparator, DirectorySeparator);

                    size_t dummy;
                    R_TRY(PathNormalizer::Normalize(dst + cur_pos, std::addressof(dummy), replaced_src, dst_size - cur_pos, is_windows_path, is_drive_relative, flags.IsAllCharactersAllowed()));
                } else {
                    /* We can just do normalization. */
                    size_t dummy;
                    R_TRY(PathNormalizer::Normalize(dst + cur_pos, std::addressof(dummy), src, dst_size - cur_pos, is_windows_path, is_drive_relative, flags.IsAllCharactersAllowed()));
                }

                R_SUCCEED();
            }
    };

    inline Result ConvertToFspPath(fssrv::sf::FspPath *out, const char *src) {
        /* Check pre-conditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(src != nullptr);

        /* Copy the path. */
        const size_t len = util::Strlcpy<char>(out->str, src, sizeof(out->str));
        R_UNLESS(len < sizeof(out->str), fs::ResultTooLongPath());

        /* Skip mount name. */
        const char *path_split_mount_name;
        size_t skip_len;
        R_TRY(PathFormatter::SkipMountName(std::addressof(path_split_mount_name), std::addressof(skip_len), src));

        /* Perform further validation. */
        if (fs::IsWindowsPath(path_split_mount_name, true)) {
            if ((skip_len == 0 || !util::Strncmp<char>(src, AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME ":", AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME_LEN + 1)) && fs::IsUncPath(out->str + skip_len, true, false)) {
                out->str[skip_len + 0] = '\\';
                out->str[skip_len + 1] = '\\';
            }
        } else {
            fs::Replace(out->str, sizeof(out->str) - 1, '\\', '/');
        }

        R_SUCCEED();
    }

    Result FormatToFspPath(fssrv::sf::FspPath *out, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

    inline Result FormatToFspPath(fssrv::sf::FspPath *out, const char *fmt, ...) {
        /* Check pre-conditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(fmt != nullptr);

        /* Format the path. */
        std::va_list vl;
        va_start(vl, fmt);
        const size_t len = util::VSNPrintf(out->str, sizeof(out->str), fmt, vl);
        va_end(vl);

        R_UNLESS(len < sizeof(out->str), fs::ResultTooLongPath());

        /* Skip mount name. */
        const char *path_split_mount_name;
        size_t skip_len;
        R_TRY(PathFormatter::SkipMountName(std::addressof(path_split_mount_name), std::addressof(skip_len), out->str));

        /* Perform further validation. */
        if (fs::IsWindowsPath(path_split_mount_name, true)) {
            if ((skip_len == 0 || !util::Strncmp<char>(out->str, AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME ":", AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME_LEN + 1)) && fs::IsUncPath(out->str + skip_len, true, false)) {
                out->str[skip_len + 0] = '\\';
                out->str[skip_len + 1] = '\\';
            }
        } else {
            fs::Replace(out->str, sizeof(out->str) - 1, '\\', '/');
        }

        R_SUCCEED();
    }

}
