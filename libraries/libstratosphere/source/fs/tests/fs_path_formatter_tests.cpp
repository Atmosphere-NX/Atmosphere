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

namespace ams::fs {

    namespace {

        constexpr size_t DefaultPathBufferSize = fs::EntryNameLengthMax + 1;

        consteval PathFlags DecodeFlags(const char *desc) {
            PathFlags flags{};

            while (*desc) {
                switch (*(desc++)) {
                    case 'B':
                        flags.AllowBackslash();
                        break;
                    case 'E':
                        flags.AllowEmptyPath();
                        break;
                    case 'M':
                        flags.AllowMountName();
                        break;
                    case 'R':
                        flags.AllowRelativePath();
                        break;
                    case 'W':
                        flags.AllowWindowsPath();
                        break;
                    case 'C':
                        flags.AllowAllCharacters();
                        break;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }

            return flags;
        }

        consteval size_t Strlen(const char *p) {
            size_t len = 0;

            while (p[len] != StringTraits::NullTerminator) {
                ++len;
            }

            return len;
        }

        //#define ENABLE_PRINT_FORMAT_TEST_DEBUGGING

        #if defined(ENABLE_PRINT_FORMAT_TEST_DEBUGGING)
        template<u32 Expected, u32 Actual>
        struct Print {
            constexpr Print() {
                if (std::is_constant_evaluated()) {
                    __builtin_unreachable();
                }
            }
        };

        template<u32 E = 0, u32 A = 0, size_t N = 0>
        consteval void PrintResultMismatchImpl(u32 e, u32 a) {
            if constexpr (N == 32) {
                Print<E, A>{};
            } else {
                const bool is_e = (e & (1 << N)) != 0;
                const bool is_a = (a & (1 << N)) != 0;
                if (is_e) {
                    if (is_a) {
                        PrintResultMismatchImpl<E | (1 << N), A | (1 << N), N + 1>(e, a);
                    } else {
                        PrintResultMismatchImpl<E | (1 << N), A | (0 << N), N + 1>(e, a);
                    }
                } else {
                    if (is_a) {
                        PrintResultMismatchImpl<E | (0 << N), A | (1 << N), N + 1>(e, a);
                    } else {
                        PrintResultMismatchImpl<E | (0 << N), A | (0 << N), N + 1>(e, a);
                    }
                }
            }
        }

        consteval void PrintResultMismatch(const Result &lhs, const Result &rhs) {
            PrintResultMismatchImpl(lhs.GetDescription(), rhs.GetDescription());
        }

        template<size_t Index, char Expected, char Actual>
        struct PrintMismatchChar {
            constexpr PrintMismatchChar() {
                if (std::is_constant_evaluated()) {
                    __builtin_unreachable();
                }
            }
        };

        template<size_t Ix, char Expected, size_t C = 0>
        consteval void PrintCharacterMismatch(char c) {
            if (c == static_cast<char>(C)) {
                PrintMismatchChar<Ix, Expected, static_cast<char>(C)>{};
                return;
            }

            if constexpr (C < std::numeric_limits<unsigned char>::max()) {
                PrintCharacterMismatch<Ix, Expected, C + 1>(c);
            }
        }

        template<size_t Ix, char C = 0>
        consteval void PrintCharacterMismatch(char c, char c2) {
            if (c == static_cast<char>(C)) {
                PrintCharacterMismatch<Ix, static_cast<char>(C)>(c2);
                return;
            }


            if constexpr (C < std::numeric_limits<unsigned char>::max()) {
                PrintCharacterMismatch<Ix, C + 1>(c, c2);
            }
        }

        template<size_t Ix = 0>
        consteval void PrintCharacterMismatch(size_t ix, char c, char c2) {
            if (Ix == ix) {
                PrintCharacterMismatch<Ix>(c, c2);
                return;
            }

            if constexpr (Ix <= DefaultPathBufferSize) {
                PrintCharacterMismatch<Ix + 1>(ix, c, c2);
            }
        }
        #endif

        consteval bool TestNormalizedImpl(const char *path, const PathFlags &flags, size_t buffer_size, const char *normalized, Result expected_result) {
            /* Allocate a buffer to normalize into. */
            char *buffer = new char[buffer_size];
            ON_SCOPE_EXIT { delete[] buffer; };
            buffer[buffer_size - 1] = '\xcc';

            /* Perform normalization. */
            const Result actual_result = PathFormatter::Normalize(buffer, buffer_size, path, Strlen(path) + 1, flags);

            /* Check that the expected result matches the actual. */
            if (actual_result.GetValue() != expected_result.GetValue()) {
                #if defined(ENABLE_PRINT_FORMAT_TEST_DEBUGGING)
                PrintResultMismatch(expected_result.GetValue(), actual_result.GetValue());
                #endif

                return false;
            }

            /* Check that the expected string matches the actual. */
            for (size_t i = 0; i < buffer_size; ++i) {
                if (normalized[i] != StringTraits::NullTerminator || R_SUCCEEDED(expected_result)) {
                    if (buffer[i] != normalized[i]) {
                        #if defined(ENABLE_PRINT_FORMAT_TEST_DEBUGGING)
                        PrintCharacterMismatch(i, normalized[i], buffer[i]);
                        #endif
                        return false;
                    }
                }

                if (normalized[i] == StringTraits::NullTerminator || buffer[i] == StringTraits::NullTerminator) {
                    break;
                }
            }

            return true;
        }

        struct NormalizeTestData {
            const char *path;
            const char *flag_desc;
            const char *normalized;
            Result result;
            size_t buffer_size = DefaultPathBufferSize;
        };

        template<size_t N, size_t Ix = 0>
        consteval bool DoNormalizeTests(const NormalizeTestData (&tests)[N]) {
            if constexpr (Ix >= N) {
                return true;
            }

            const auto &test = tests[Ix];
            if (!TestNormalizedImpl(test.path, DecodeFlags(test.flag_desc), test.buffer_size, test.normalized, test.result)) {
                return false;
            }

            if constexpr (Ix < N) {
                return DoNormalizeTests<N, Ix + 1>(tests);
            } else {
                AMS_ASSUME(false);
            }
        }

        consteval bool TestNormalizeEmptyPath() {
            constexpr NormalizeTestData Tests[] = {
                { "", "",  "", fs::ResultInvalidPathFormat() },
                { "", "E", "", ResultSuccess() },
                { "/aa/bb/../cc", "E", "/aa/cc", ResultSuccess() },
            };

            return DoNormalizeTests(Tests);
        }
        static_assert(TestNormalizeEmptyPath());

        consteval bool TestNormalizeMountName() {
            constexpr NormalizeTestData Tests[] = {
                { "mount:/aa/bb", "", "", fs::ResultInvalidPathFormat() },
                { "mount:/aa/bb", "W", "", fs::ResultInvalidPathFormat() },
                { "mount:/aa/bb", "M", "mount:/aa/bb", ResultSuccess() },
                { "mount:/aa/./bb", "M", "mount:/aa/bb", ResultSuccess() },
                { "mount:\\aa\\bb", "M", "mount:", fs::ResultInvalidPathFormat() },
                { "m:/aa/bb", "M", "", fs::ResultInvalidPathFormat() },
                { "mo>unt:/aa/bb", "M", "", fs::ResultInvalidCharacter() },
                { "moun?t:/aa/bb", "M", "", fs::ResultInvalidCharacter() },
                { "mo&unt:/aa/bb", "M", "mo&unt:/aa/bb", ResultSuccess() },
                { "/aa/./bb", "M", "/aa/bb", ResultSuccess() },
                { "mount/aa/./bb", "M", "", fs::ResultInvalidPathFormat() }
            };

            return DoNormalizeTests(Tests);
        }
        static_assert(TestNormalizeMountName());

        consteval bool TestNormalizeWindowsPath() {
            constexpr NormalizeTestData Tests[] = {
                { "c:/aa/bb", "", "", fs::ResultInvalidPathFormat() },
                { "c:\\aa\\bb", "", "", fs::ResultInvalidCharacter() },
                { "\\\\host\\share", "", "", fs::ResultInvalidCharacter() },
                { "\\\\.\\c:\\", "", "", fs::ResultInvalidCharacter() },
                { "\\\\.\\c:/aa/bb/.", "", "", fs::ResultInvalidCharacter() },
                { "\\\\?\\c:\\", "", "", fs::ResultInvalidCharacter() },
                { "mount:\\\\host\\share\\aa\\bb", "M", "mount:", fs::ResultInvalidCharacter() },
                { "mount:\\\\host/share\\aa\\bb", "M", "mount:", fs::ResultInvalidCharacter() },
                { "c:\\aa\\..\\..\\..\\bb", "W", "c:/bb", ResultSuccess() },
                { "mount:/\\\\aa\\..\\bb", "MW", "mount:", fs::ResultInvalidPathFormat() },
                { "mount:/c:\\aa\\..\\bb", "MW", "mount:c:/bb", ResultSuccess() },
                { "mount:/aa/bb", "MW", "mount:/aa/bb", ResultSuccess() },
                { "/mount:/aa/bb", "MW", "/", fs::ResultInvalidCharacter() },
                { "/mount:/aa/bb", "W", "/", fs::ResultInvalidCharacter() },
                { "a:aa/../bb", "MW", "a:aa/bb", ResultSuccess() },
                { "a:aa\\..\\bb", "MW", "a:aa/bb", ResultSuccess() },
                { "/a:aa\\..\\bb", "W", "/", fs::ResultInvalidCharacter() },
                { "\\\\?\\c:\\.\\aa", "W", "\\\\?\\c:/aa", ResultSuccess() },
                { "\\\\.\\c:\\.\\aa", "W", "\\\\.\\c:/aa", ResultSuccess() },
                { "\\\\.\\mount:\\.\\aa", "W", "\\\\./", fs::ResultInvalidCharacter() },
                { "\\\\./.\\aa", "W", "\\\\./aa", ResultSuccess() },
                { "\\\\/aa", "W", "", fs::ResultInvalidPathFormat() },
                { "\\\\\\aa", "W", "", fs::ResultInvalidPathFormat() },
                { "\\\\", "W", "/", ResultSuccess() },
                { "\\\\host\\share", "W", "\\\\host\\share/", ResultSuccess() },
                { "\\\\host\\share\\path", "W", "\\\\host\\share/path", ResultSuccess() },
                { "\\\\host\\share\\path\\aa\\bb\\..\\cc\\.", "W", "\\\\host\\share/path/aa/cc", ResultSuccess() },
                { "\\\\host\\", "W", "", fs::ResultInvalidPathFormat() },
                { "\\\\ho$st\\share\\path", "W", "", fs::ResultInvalidCharacter() },
                { "\\\\host:\\share\\path", "W", "", fs::ResultInvalidCharacter() },
                { "\\\\..\\share\\path", "W", "", fs::ResultInvalidPathFormat() },
                { "\\\\host\\s:hare\\path", "W", "", fs::ResultInvalidCharacter() },
                { "\\\\host\\.\\path", "W", "", fs::ResultInvalidPathFormat() },
                { "\\\\host\\..\\path", "W", "", fs::ResultInvalidPathFormat() },
                { "\\\\host\\sha:re", "W", "", fs::ResultInvalidCharacter() },
                { ".\\\\host\\share", "RW", "..\\\\host\\share/", ResultSuccess() }
            };

            return DoNormalizeTests(Tests);
        }
        static_assert(TestNormalizeWindowsPath());

        consteval bool TestNormalizeRelativePath() {
            constexpr NormalizeTestData Tests[] = {
                { "./aa/bb", "", "", fs::ResultInvalidPathFormat() },
                { "./aa/bb/../cc", "R", "./aa/cc", ResultSuccess() },
                { ".\\aa/bb/../cc", "R", "..", fs::ResultInvalidCharacter() },
                { ".", "R", ".", ResultSuccess() },
                { "../aa/bb", "R", "", fs::ResultDirectoryUnobtainable() },
                { "/aa/./bb", "R", "/aa/bb", ResultSuccess() },
                { "mount:./aa/bb", "MR", "mount:./aa/bb", ResultSuccess() },
                { "mount:./aa/./bb", "MR", "mount:./aa/bb", ResultSuccess() },
                { "mount:./aa/bb", "M", "mount:", fs::ResultInvalidPathFormat() }
            };

            return DoNormalizeTests(Tests);
        }
        static_assert(TestNormalizeRelativePath());

        consteval bool TestNormalizeBackslash() {
            constexpr NormalizeTestData Tests[] = {
                { "\\aa\\bb\\..\\cc", "", "", fs::ResultInvalidPathFormat() },
                { "\\aa\\bb\\..\\cc", "B", "", fs::ResultInvalidPathFormat() },
                { "/aa\\bb\\..\\cc", "", "", fs::ResultInvalidCharacter() },
                { "/aa\\bb\\..\\cc", "B", "/cc", ResultSuccess() },
                { "/aa\\bb\\cc", "", "", fs::ResultInvalidCharacter() },
                { "/aa\\bb\\cc", "B", "/aa\\bb\\cc", ResultSuccess() },
                { "\\\\host\\share\\path\\aa\\bb\\cc", "W", "\\\\host\\share/path/aa/bb/cc", ResultSuccess() },
                { "\\\\host\\share\\path\\aa\\bb\\cc", "WB", "\\\\host\\share/path/aa/bb/cc", ResultSuccess() },
                { "/aa/bb\\../cc/..\\dd\\..\\ee/..", "", "", fs::ResultInvalidCharacter() },
                { "/aa/bb\\../cc/..\\dd\\..\\ee/..", "B", "/aa", ResultSuccess() }
            };

            return DoNormalizeTests(Tests);
        }
        static_assert(TestNormalizeBackslash());

        consteval bool TestNormalizeAllowAllChars() {
            constexpr NormalizeTestData Tests[] = {
                { "/aa/b:b/cc", "", "/aa/", fs::ResultInvalidCharacter() },
                { "/aa/b*b/cc", "", "/aa/", fs::ResultInvalidCharacter() },
                { "/aa/b?b/cc", "", "/aa/", fs::ResultInvalidCharacter() },
                { "/aa/b<b/cc", "", "/aa/", fs::ResultInvalidCharacter() },
                { "/aa/b>b/cc", "", "/aa/", fs::ResultInvalidCharacter() },
                { "/aa/b|b/cc", "", "/aa/", fs::ResultInvalidCharacter() },
                { "/aa/b:b/cc", "C", "/aa/b:b/cc", ResultSuccess() },
                { "/aa/b*b/cc", "C", "/aa/b*b/cc", ResultSuccess() },
                { "/aa/b?b/cc", "C", "/aa/b?b/cc", ResultSuccess() },
                { "/aa/b<b/cc", "C", "/aa/b<b/cc", ResultSuccess() },
                { "/aa/b>b/cc", "C", "/aa/b>b/cc", ResultSuccess() },
                { "/aa/b|b/cc", "C", "/aa/b|b/cc", ResultSuccess() },
                { "/aa/b'b/cc", "", "/aa/b'b/cc", ResultSuccess() },
                { "/aa/b\"b/cc", "", "/aa/b\"b/cc", ResultSuccess() },
                { "/aa/b(b/cc", "", "/aa/b(b/cc", ResultSuccess() },
                { "/aa/b)b/cc", "", "/aa/b)b/cc", ResultSuccess() },
                { "/aa/b'b/cc", "C", "/aa/b'b/cc", ResultSuccess() },
                { "/aa/b\"b/cc", "C", "/aa/b\"b/cc", ResultSuccess() },
                { "/aa/b(b/cc", "C", "/aa/b(b/cc", ResultSuccess() },
                { "/aa/b)b/cc", "C", "/aa/b)b/cc", ResultSuccess() },
                { "mount:/aa/b<b/cc", "MC", "mount:/aa/b<b/cc", ResultSuccess() },
                { "mo>unt:/aa/bb/cc", "MC", "", fs::ResultInvalidCharacter() }
            };

            return DoNormalizeTests(Tests);
        }
        static_assert(TestNormalizeAllowAllChars());

        consteval bool TestNormalizeAll() {
            constexpr NormalizeTestData Tests[] = {
                { "mount:./aa/bb", "WRM", "mount:./aa/bb", ResultSuccess() },
                { "mount:./aa/bb\\cc/dd", "WRM", "mount:./aa/bb/cc/dd", ResultSuccess() },
                { "mount:./aa/bb\\cc/dd", "WRMB", "mount:./aa/bb/cc/dd", ResultSuccess() },
                { "mount:./.c:/aa/bb", "RM", "mount:./", fs::ResultInvalidCharacter() },
                { "mount:.c:/aa/bb", "WRM", "mount:./", fs::ResultInvalidCharacter() },
                { "mount:./cc:/aa/bb", "WRM", "mount:./", fs::ResultInvalidCharacter() },
                { "mount:./\\\\host\\share/aa/bb", "MW", "mount:", fs::ResultInvalidPathFormat() },
                { "mount:./\\\\host\\share/aa/bb", "WRM", "mount:.\\\\host\\share/aa/bb", ResultSuccess() },
                { "mount:.\\\\host\\share/aa/bb", "WRM", "mount:..\\\\host\\share/aa/bb", ResultSuccess() },
                { "mount:..\\\\host\\share/aa/bb", "WRM", "mount:.", fs::ResultDirectoryUnobtainable() },
                { ".\\\\host\\share/aa/bb", "WRM", "..\\\\host\\share/aa/bb", ResultSuccess() },
                { "..\\\\host\\share/aa/bb", "WRM", ".", fs::ResultDirectoryUnobtainable() },
                { "mount:\\\\host\\share/aa/bb", "MW", "mount:\\\\host\\share/aa/bb", ResultSuccess() },
                { "mount:\\aa\\bb", "BM", "mount:", fs::ResultInvalidPathFormat() },
                { "mount:/aa\\bb", "BM", "mount:/aa\\bb", ResultSuccess() },
                { ".//aa/bb", "RW", "./aa/bb", ResultSuccess() },
                { "./aa/bb", "R", "./aa/bb", ResultSuccess() },
                { "./c:/aa/bb", "RW", "./", fs::ResultInvalidCharacter() },
                { "mount:./aa/b:b\\cc/dd", "WRMBC", "mount:./aa/b:b/cc/dd", ResultSuccess() }
            };

            return DoNormalizeTests(Tests);
        }
        static_assert(TestNormalizeAll());

        consteval bool TestNormalizeSmallBuffer() {
            constexpr NormalizeTestData Tests[] = {
                { "/aa/bb", "M", "", fs::ResultTooLongPath(), 1},
                { "mount:/aa/bb", "MR", "", fs::ResultTooLongPath(), 6 },
                { "mount:/aa/bb", "MR", "mount:", fs::ResultTooLongPath(), 7 },
                { "aa/bb", "MR", "./", fs::ResultTooLongPath(), 3 },
                { "\\\\host\\share", "W", "\\\\host\\share", fs::ResultTooLongPath(), 13 }
            };

            return DoNormalizeTests(Tests);
        }
        static_assert(TestNormalizeSmallBuffer());

        consteval bool TestIsNormalizedImpl(const char *path, const PathFlags &flags, bool expected_normalized, size_t expected_size, Result expected_result) {
            /* Perform normalization checking. */
            bool actual_normalized;
            size_t actual_size;
            const Result actual_result = PathFormatter::IsNormalized(std::addressof(actual_normalized), std::addressof(actual_size), path, flags);

            /* Check that the expected result matches the actual. */
            if (actual_result.GetValue() != expected_result.GetValue()) {
                #if defined(ENABLE_PRINT_FORMAT_TEST_DEBUGGING)
                PrintResultMismatch(expected_result.GetValue(), actual_result.GetValue());
                #endif

                return false;
            }

            /* Check that the expected output matches the actual. */
            if (R_SUCCEEDED(expected_result)) {
                if (expected_normalized != actual_normalized) {
                    #if defined(ENABLE_PRINT_FORMAT_TEST_DEBUGGING)
                    PrintResultMismatchImpl(static_cast<u32>(expected_normalized), static_cast<u32>(actual_normalized));
                    #endif
                    return false;
                }

                if (expected_normalized) {
                    if (expected_size != actual_size) {
                        #if defined(ENABLE_PRINT_FORMAT_TEST_DEBUGGING)
                        PrintResultMismatchImpl(static_cast<u32>(expected_size), static_cast<u32>(actual_size));
                        #endif
                        return false;
                    }
                }
            }

            return true;
        }

        struct IsNormalizedTestData {
            const char *path;
            const char *flag_desc;
            bool normalized;
            size_t len;
            Result result;
        };

        template<size_t N, size_t Ix = 0>
        consteval bool DoIsNormalizedTests(const IsNormalizedTestData (&tests)[N]) {
            if constexpr (Ix >= N) {
                return true;
            }

            const auto &test = tests[Ix];
            if (!TestIsNormalizedImpl(test.path, DecodeFlags(test.flag_desc), test.normalized, test.len, test.result)) {
                return false;
            }

            if constexpr (Ix < N) {
                return DoIsNormalizedTests<N, Ix + 1>(tests);
            } else {
                AMS_ASSUME(false);
            }
        }

        consteval bool TestIsNormalizedEmptyPath() {
            constexpr IsNormalizedTestData Tests[] = {
                { "", "", false, 0, fs::ResultInvalidPathFormat() },
                { "", "E", true, 0, ResultSuccess() },
                { "/aa/bb/../cc", "E", false, 0, ResultSuccess() }
            };

            return DoIsNormalizedTests(Tests);
        }
        static_assert(TestIsNormalizedEmptyPath());

        consteval bool TestIsNormalizedMountName() {
            constexpr IsNormalizedTestData Tests[] = {
                { "mount:/aa/bb", "", false, 0, fs::ResultInvalidPathFormat() },
                { "mount:/aa/bb", "W", false, 0, fs::ResultInvalidPathFormat() },
                { "mount:/aa/bb", "M", true, 12, ResultSuccess() },
                { "mount:/aa/./bb", "M", false, 6, ResultSuccess() },
                { "mount:\\aa\\bb", "M", false, 0, fs::ResultInvalidPathFormat() },
                { "m:/aa/bb", "M", false, 0, fs::ResultInvalidPathFormat() },
                { "mo>unt:/aa/bb", "M", false, 0, fs::ResultInvalidCharacter() },
                { "moun?t:/aa/bb", "M", false, 0, fs::ResultInvalidCharacter() },
                { "mo&unt:/aa/bb", "M", true, 13, ResultSuccess() },
                { "/aa/./bb", "M", false, 0, ResultSuccess() },
                { "mount/aa/./bb", "M", false, 0, fs::ResultInvalidPathFormat() }
            };

            return DoIsNormalizedTests(Tests);
        }
        static_assert(TestIsNormalizedMountName());

        consteval bool TestIsNormalizedWindowsPath() {
            constexpr IsNormalizedTestData Tests[] = {
                { "c:/aa/bb", "", false, 0, fs::ResultInvalidPathFormat() },
                { "c:\\aa\\bb", "", false, 0, fs::ResultInvalidPathFormat() },
                { "\\\\host\\share", "", false, 0, fs::ResultInvalidPathFormat() },
                { "\\\\.\\c:\\", "", false, 0, fs::ResultInvalidPathFormat() },
                { "\\\\.\\c:/aa/bb/.", "", false, 0, fs::ResultInvalidPathFormat() },
                { "\\\\?\\c:\\", "", false, 0, fs::ResultInvalidPathFormat() },
                { "mount:\\\\host\\share\\aa\\bb", "M", false, 0, fs::ResultInvalidPathFormat() },
                { "mount:\\\\host/share\\aa\\bb", "M", false, 0, fs::ResultInvalidPathFormat() },
                { "c:\\aa\\..\\..\\..\\bb", "W", false, 0, ResultSuccess() },
                { "mount:/\\\\aa\\..\\bb", "MW", false, 0, ResultSuccess() },
                { "mount:/c:\\aa\\..\\bb", "MW", false, 0, ResultSuccess() },
                { "mount:/aa/bb", "MW", true, 12, ResultSuccess() },
                { "/mount:/aa/bb", "MW", false, 0, fs::ResultInvalidCharacter() },
                { "/mount:/aa/bb", "W", false, 0, fs::ResultInvalidCharacter() },
                { "a:aa/../bb", "MW", false, 8, ResultSuccess() },
                { "a:aa\\..\\bb", "MW", false, 0, ResultSuccess() },
                { "/a:aa\\..\\bb", "W", false, 0, ResultSuccess() },
                { "\\\\?\\c:\\.\\aa", "W", false, 0, ResultSuccess() },
                { "\\\\.\\c:\\.\\aa", "W", false, 0, ResultSuccess() },
                { "\\\\.\\mount:\\.\\aa", "W", false, 0, ResultSuccess() },
                { "\\\\./.\\aa", "W", false, 0, ResultSuccess() },
                { "\\\\/aa", "W", false, 0, fs::ResultInvalidPathFormat() },
                { "\\\\\\aa", "W", false, 0, fs::ResultInvalidPathFormat() },
                { "\\\\", "W", false, 0, ResultSuccess() },
                { "\\\\host\\share", "W", false, 0, ResultSuccess() },
                { "\\\\host\\share\\path", "W", false, 0, ResultSuccess() },
                { "\\\\host\\share\\path\\aa\\bb\\..\\cc\\.", "W", false, 0, ResultSuccess() },
                { "\\\\host\\", "W", false, 0, fs::ResultInvalidPathFormat() },
                { "\\\\ho$st\\share\\path", "W", false, 0, fs::ResultInvalidCharacter() },
                { "\\\\host:\\share\\path", "W", false, 0, fs::ResultInvalidCharacter() },
                { "\\\\..\\share\\path", "W", false, 0, fs::ResultInvalidPathFormat() },
                { "\\\\host\\s:hare\\path", "W", false, 0, fs::ResultInvalidCharacter() },
                { "\\\\host\\.\\path", "W", false, 0, fs::ResultInvalidPathFormat() },
                { "\\\\host\\..\\path", "W", false, 0, fs::ResultInvalidPathFormat() },
                { "\\\\host\\sha:re", "W", false, 0, fs::ResultInvalidCharacter() },
                { ".\\\\host\\share", "RW", false, 0, ResultSuccess() }
            };

            return DoIsNormalizedTests(Tests);
        }
        static_assert(TestIsNormalizedWindowsPath());

        consteval bool TestIsNormalizedRelativePath() {
            constexpr IsNormalizedTestData Tests[] = {
                { "./aa/bb", "", false, 0, fs::ResultInvalidPathFormat() },
                { "./aa/bb/../cc", "R", false, 1, ResultSuccess() },
                { ".\\aa/bb/../cc", "R", false, 0, ResultSuccess() },
                { ".", "R", true, 1, ResultSuccess() },
                { "../aa/bb", "R", false, 0, fs::ResultDirectoryUnobtainable() },
                { "/aa/./bb", "R", false, 0, ResultSuccess() },
                { "mount:./aa/bb", "MR", true, 13, ResultSuccess() },
                { "mount:./aa/./bb", "MR", false, 7, ResultSuccess() },
                { "mount:./aa/bb", "M", false, 0, fs::ResultInvalidPathFormat() }
            };

            return DoIsNormalizedTests(Tests);
        }
        static_assert(TestIsNormalizedRelativePath());

        consteval bool TestIsNormalizedBackslash() {
            constexpr IsNormalizedTestData Tests[] = {
                { "\\aa\\bb\\..\\cc", "", false, 0, fs::ResultInvalidPathFormat() },
                { "\\aa\\bb\\..\\cc", "B", false, 0, fs::ResultInvalidPathFormat() },
                { "/aa\\bb\\..\\cc", "", false, 0, fs::ResultInvalidCharacter() },
                { "/aa\\bb\\..\\cc", "B", false, 0, ResultSuccess() },
                { "/aa\\bb\\cc", "", false, 0, fs::ResultInvalidCharacter() },
                { "/aa\\bb\\cc", "B", true, 9, ResultSuccess() },
                { "\\\\host\\share\\path\\aa\\bb\\cc", "W", false, 0, ResultSuccess() },
                { "\\\\host\\share\\path\\aa\\bb\\cc", "WB", false, 0, ResultSuccess() },
                { "/aa/bb\\../cc/..\\dd\\..\\ee/..", "", false, 0, fs::ResultInvalidCharacter() },
                { "/aa/bb\\../cc/..\\dd\\..\\ee/..", "B", false, 0, ResultSuccess() }
            };

            return DoIsNormalizedTests(Tests);
        }
        static_assert(TestIsNormalizedBackslash());

        consteval bool TestIsNormalizedAllowAllCharacters() {
            constexpr IsNormalizedTestData Tests[] = {
                { "/aa/b:b/cc", "", false, 0, fs::ResultInvalidCharacter() },
                { "/aa/b*b/cc", "", false, 0, fs::ResultInvalidCharacter() },
                { "/aa/b?b/cc", "", false, 0, fs::ResultInvalidCharacter() },
                { "/aa/b<b/cc", "", false, 0, fs::ResultInvalidCharacter() },
                { "/aa/b>b/cc", "", false, 0, fs::ResultInvalidCharacter() },
                { "/aa/b|b/cc", "", false, 0, fs::ResultInvalidCharacter() },
                { "/aa/b:b/cc", "C", true, 10, ResultSuccess() },
                { "/aa/b*b/cc", "C", true, 10, ResultSuccess() },
                { "/aa/b?b/cc", "C", true, 10, ResultSuccess() },
                { "/aa/b<b/cc", "C", true, 10, ResultSuccess() },
                { "/aa/b>b/cc", "C", true, 10, ResultSuccess() },
                { "/aa/b|b/cc", "C", true, 10, ResultSuccess() },
                { "/aa/b'b/cc", "", true, 10, ResultSuccess() },
                { "/aa/b\"b/cc", "", true, 10, ResultSuccess() },
                { "/aa/b(b/cc", "", true, 10, ResultSuccess() },
                { "/aa/b)b/cc", "", true, 10, ResultSuccess() },
                { "/aa/b'b/cc", "C", true, 10, ResultSuccess() },
                { "/aa/b\"b/cc", "C", true, 10, ResultSuccess() },
                { "/aa/b(b/cc", "C", true, 10, ResultSuccess() },
                { "/aa/b)b/cc", "C", true, 10, ResultSuccess() },
                { "mount:/aa/b<b/cc", "MC", true, 16, ResultSuccess() },
                { "mo>unt:/aa/bb/cc", "MC", false, 0, fs::ResultInvalidCharacter() }
            };

            return DoIsNormalizedTests(Tests);
        }
        static_assert(TestIsNormalizedAllowAllCharacters());

        consteval bool TestIsNormalizedAll() {
            constexpr IsNormalizedTestData Tests[] = {
                { "mount:./aa/bb", "WRM", true, 13, ResultSuccess() },
                { "mount:./aa/bb\\cc/dd", "WRM", false, 0, ResultSuccess() },
                { "mount:./aa/bb\\cc/dd", "WRMB", true, 19, ResultSuccess() },
                { "mount:./.c:/aa/bb", "RM", false, 0, fs::ResultInvalidCharacter() },
                { "mount:.c:/aa/bb", "WRM", false, 0, ResultSuccess() },
                { "mount:./cc:/aa/bb", "WRM", false, 0, fs::ResultInvalidCharacter() },
                { "mount:./\\\\host\\share/aa/bb", "MW", false, 0, fs::ResultInvalidPathFormat() },
                { "mount:./\\\\host\\share/aa/bb", "WRM", false, 0, ResultSuccess() },
                { "mount:.\\\\host\\share/aa/bb", "WRM", false, 0, ResultSuccess() },
                { "mount:..\\\\host\\share/aa/bb", "WRM", false, 0, ResultSuccess() },
                { ".\\\\host\\share/aa/bb", "WRM", false, 0, ResultSuccess() },
                { "..\\\\host\\share/aa/bb", "WRM", false, 0, ResultSuccess() },
                { "mount:\\\\host\\share/aa/bb", "MW", true, 24, ResultSuccess() },
                { "mount:\\aa\\bb", "BM", false, 0, fs::ResultInvalidPathFormat() },
                { "mount:/aa\\bb", "BM", true, 12, ResultSuccess() },
                { ".//aa/bb", "RW", false, 1, ResultSuccess() },
                { "./aa/bb", "R", true, 7, ResultSuccess() },
                { "./c:/aa/bb", "RW", false, 0, fs::ResultInvalidCharacter() },
                { "mount:./aa/b:b\\cc/dd", "WRMBC", true, 20, ResultSuccess() }
            };

            return DoIsNormalizedTests(Tests);
        }
        static_assert(TestIsNormalizedAll());

    }

}
