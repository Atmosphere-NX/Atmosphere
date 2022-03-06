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

        consteval bool TestNormalizeImpl(const char *path, size_t buffer_size, const char *normalized, bool windows_path, bool drive_relative, bool all_chars, size_t expected_length, Result expected_result) {
            /* Allocate a buffer to normalize into. */
            char *buffer = new char[buffer_size];
            ON_SCOPE_EXIT { delete[] buffer; };
            buffer[buffer_size - 1] = '\xcc';

            /* Perform normalization. */
            size_t actual_length;
            const Result actual_result = PathNormalizer::Normalize(buffer, std::addressof(actual_length), path, buffer_size, windows_path, drive_relative, all_chars);

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

            /* Check that the expected length matches the actual. */
            if (R_SUCCEEDED(expected_result) || fs::ResultTooLongPath::Includes(expected_result)) {
                if (expected_length != actual_length) {
                    #if defined(ENABLE_PRINT_FORMAT_TEST_DEBUGGING)
                    PrintResultMismatchImpl(static_cast<u32>(expected_length), static_cast<u32>(actual_length));
                    #endif
                    return false;
                }
            }

            return true;
        }

        struct NormalizeTestData {
            const char *path;
            bool windows;
            bool rel;
            bool allow_all;
            const char *normalized;
            size_t len;
            Result result;
        };

        template<size_t N, size_t Ix = 0>
        consteval bool DoNormalizeTests(const NormalizeTestData (&tests)[N]) {
            if constexpr (Ix >= N) {
                return true;
            }

            const auto &test = tests[Ix];
            if (!TestNormalizeImpl(test.path, DefaultPathBufferSize, test.normalized, test.windows, test.rel, test.allow_all, test.len, test.result)) {
                return false;
            }

            if constexpr (Ix < N) {
                return DoNormalizeTests<N, Ix + 1>(tests);
            } else {
                AMS_ASSUME(false);
            }
        }

        consteval bool TestNormalized() {
            constexpr NormalizeTestData Tests[] = {
                { "/aa/bb/c/", false, true, false, "/aa/bb/c", 8, ResultSuccess() },
                { "aa/bb/c/", false, false, false, "", 0, fs::ResultInvalidPathFormat() },
                { "aa/bb/c/", false, true, false, "/aa/bb/c", 8, ResultSuccess() },
                { "mount:a/b", false, true, false, "/", 0, fs::ResultInvalidCharacter() },
                { "mo|unt:a/b", false, true, true, "/mo|unt:a/b", 11, ResultSuccess() },
                { "/aa/bb/../..", true, false, false, "/", 1, ResultSuccess() },
                { "/aa/bb/../../..", true, false, false, "/", 1, ResultSuccess() },
                { "/aa/bb/../../..", false, false, false, "/aa/bb/", 0, fs::ResultDirectoryUnobtainable() },
                { "aa/bb/../../..", true, true, false, "/", 1, ResultSuccess() },
                { "aa/bb/../../..", false, true, false, "/aa/bb/", 0, fs::ResultDirectoryUnobtainable() },
                { "mount:a/b", false, true, true, "/mount:a/b", 10, ResultSuccess() },
                { "/a|/bb/cc", false, false, true, "/a|/bb/cc", 9, ResultSuccess() },
                { "/>a/bb/cc", false, false, true, "/>a/bb/cc", 9, ResultSuccess() },
                { "/aa/.</cc", false, false, true, "/aa/.</cc", 9, ResultSuccess() },
                { "/aa/..</cc", false, false, true, "/aa/..</cc", 10, ResultSuccess() },
                { "", false, false, false, "", 0, fs::ResultInvalidPathFormat() },
                { "/", false, false, false, "/", 1, ResultSuccess() },
                { "/.", false, false, false, "/", 1, ResultSuccess() },
                { "/./", false, false, false, "/", 1, ResultSuccess() },
                { "/..", false, false, false, "/", 0, fs::ResultDirectoryUnobtainable() },
                { "//.", false, false, false, "/", 1, ResultSuccess() },
                { "/ ..", false, false, false, "/ ..", 4, ResultSuccess() },
                { "/.. /", false, false, false, "/.. ", 4, ResultSuccess() },
                { "/. /.", false, false, false, "/. ", 3, ResultSuccess() },
                { "/aa/bb/cc/dd/./.././../..", false, false, false, "/aa", 3, ResultSuccess() },
                { "/aa/bb/cc/dd/./.././../../..", false, false, false, "/", 1, ResultSuccess() },
                { "/./aa/./bb/./cc/./dd/.", false, false, false, "/aa/bb/cc/dd", 12, ResultSuccess() },
                { "/aa\\bb/cc", false, false, false, "/aa\\bb/cc", 9, ResultSuccess() },
                { "/aa\\bb/cc", false, false, false, "/aa\\bb/cc", 9, ResultSuccess() },
                { "/a|/bb/cc", false, false, false, "/", 0, fs::ResultInvalidCharacter() },
                { "/>a/bb/cc", false, false, false, "/", 0, fs::ResultInvalidCharacter() },
                { "/aa/.</cc", false, false, false, "/aa/", 0, fs::ResultInvalidCharacter() },
                { "/aa/..</cc", false, false, false, "/aa/", 0, fs::ResultInvalidCharacter() },
                { "\\\\aa/bb/cc", false, false, false, "", 0, fs::ResultInvalidPathFormat() },
                { "\\\\aa\\bb\\cc", false, false, false, "", 0, fs::ResultInvalidPathFormat() },
                { "/aa/bb/..\\cc", false, false, false, "/aa/cc", 6, ResultSuccess() },
                { "/aa/bb\\..\\cc", false, false, false, "/aa/cc", 6, ResultSuccess() },
                { "/aa/bb\\..", false, false, false, "/aa", 3, ResultSuccess() },
                { "/aa\\bb/../cc", false, false, false, "/cc", 3, ResultSuccess() }
            };

            return DoNormalizeTests(Tests);
        }
        static_assert(TestNormalized());

        struct NormalizeTestDataSmallBuffer {
            const char *path;
            size_t buffer_size;
            const char *normalized;
            size_t len;
            Result result;
        };

        template<size_t N, size_t Ix = 0>
        consteval bool DoNormalizeTests(const NormalizeTestDataSmallBuffer (&tests)[N]) {
            if constexpr (Ix >= N) {
                return true;
            }

            const auto &test = tests[Ix];
            if (!TestNormalizeImpl(test.path, test.buffer_size, test.normalized, false, false, false, test.len, test.result)) {
                return false;
            }

            if constexpr (Ix < N) {
                return DoNormalizeTests<N, Ix + 1>(tests);
            } else {
                AMS_ASSUME(false);
            }
        }

        consteval bool TestNormalizedSmallBuffer() {
            constexpr NormalizeTestDataSmallBuffer Tests[] = {
                { "/aa/bb/cc/", 7, "/aa/bb", 6, fs::ResultTooLongPath() },
                { "/aa/bb/cc/", 8, "/aa/bb/", 7, fs::ResultTooLongPath() },
                { "/aa/bb/cc/", 9, "/aa/bb/c", 8, fs::ResultTooLongPath() },
                { "/aa/bb/cc/", 10, "/aa/bb/cc", 9, ResultSuccess() },
                { "/aa/bb/cc", 9, "/aa/bb/c", 8, fs::ResultTooLongPath() },
                { "/aa/bb/cc", 10, "/aa/bb/cc", 9, ResultSuccess() },
                { "/./aa/./bb/./cc", 9, "/aa/bb/c", 8, fs::ResultTooLongPath() },
                { "/./aa/./bb/./cc", 10, "/aa/bb/cc", 9, ResultSuccess() },
                { "/aa/bb/cc/../../..", 9, "/aa/bb/c", 8, fs::ResultTooLongPath() },
                { "/aa/bb/cc/../../..", 10, "/aa/bb/cc", 9, fs::ResultTooLongPath() },
                { "/aa/bb/.", 7, "/aa/bb", 6, fs::ResultTooLongPath() },
                { "/aa/bb/./", 7, "/aa/bb", 6, fs::ResultTooLongPath() },
                { "/aa/bb/..", 8, "/aa", 3, ResultSuccess() },
                { "/aa/bb", 1, "", 0, fs::ResultTooLongPath() },
                { "/aa/bb", 2, "/", 1, fs::ResultTooLongPath() },
                { "/aa/bb", 3, "/a", 2, fs::ResultTooLongPath() },
                { "aa/bb", 1, "", 0, fs::ResultInvalidPathFormat() }
            };

            return DoNormalizeTests(Tests);
        }
        static_assert(TestNormalizedSmallBuffer());

        consteval bool TestIsNormalizedImpl(const char *path, bool allow_all, bool expected_normalized, size_t expected_size, Result expected_result) {
            /* Perform normalization checking. */
            bool actual_normalized;
            size_t actual_size = 0;
            const Result actual_result = PathNormalizer::IsNormalized(std::addressof(actual_normalized), std::addressof(actual_size), path, allow_all);

            /* Check that the expected result matches the actual. */
            if (actual_result.GetValue() != expected_result.GetValue()) {
                #if defined(ENABLE_PRINT_FORMAT_TEST_DEBUGGING)
                PrintResultMismatch(expected_result.GetValue(), actual_result.GetValue());
                #endif

                return false;
            }

            if (expected_size != actual_size) {
                #if defined(ENABLE_PRINT_FORMAT_TEST_DEBUGGING)
                PrintResultMismatchImpl(static_cast<u32>(expected_size), static_cast<u32>(actual_size));
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
            }

            return true;
        }

        struct IsNormalizedTestData {
            const char *path;
            bool allow_all;
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
            if (!TestIsNormalizedImpl(test.path, test.allow_all, test.normalized, test.len, test.result)) {
                return false;
            }

            if constexpr (Ix < N) {
                return DoIsNormalizedTests<N, Ix + 1>(tests);
            } else {
                AMS_ASSUME(false);
            }
        }

        consteval bool TestIsNormalized() {
            constexpr IsNormalizedTestData Tests[] = {
                { "/aa/bb/c/", false, false, 9, ResultSuccess() },
                { "aa/bb/c/", false, false, 0, fs::ResultInvalidPathFormat() },
                { "aa/bb/c/", false, false, 0, fs::ResultInvalidPathFormat() },
                { "mount:a/b", false, false, 0, fs::ResultInvalidPathFormat() },
                { "mo|unt:a/b", true, false, 0, fs::ResultInvalidPathFormat() },
                { "/aa/bb/../..", false, false, 0, ResultSuccess() },
                { "/aa/bb/../../..", false, false, 0, ResultSuccess() },
                { "/aa/bb/../../..", false, false, 0, ResultSuccess() },
                { "aa/bb/../../..", false, false, 0, fs::ResultInvalidPathFormat() },
                { "aa/bb/../../..", false, false, 0, fs::ResultInvalidPathFormat() },
                { "mount:a/b", true, false, 0, fs::ResultInvalidPathFormat() },
                { "/a|/bb/cc", true, true, 9, ResultSuccess() },
                { "/>a/bb/cc", true, true, 9, ResultSuccess() },
                { "/aa/.</cc", true, true, 9, ResultSuccess() },
                { "/aa/..</cc", true, true, 10, ResultSuccess() },
                { "", false, false, 0, fs::ResultInvalidPathFormat() },
                { "/", false, true, 1, ResultSuccess() },
                { "/.", false, false, 2, ResultSuccess() },
                { "/./", false, false, 0, ResultSuccess() },
                { "/..", false, false, 3, ResultSuccess() },
                { "//.", false, false, 0, ResultSuccess() },
                { "/ ..", false, true, 4, ResultSuccess() },
                { "/.. /", false, false, 5, ResultSuccess() },
                { "/. /.", false, false, 5, ResultSuccess() },
                { "/aa/bb/cc/dd/./.././../..", false, false, 0, ResultSuccess() },
                { "/aa/bb/cc/dd/./.././../../..", false, false, 0, ResultSuccess() },
                { "/./aa/./bb/./cc/./dd/.", false, false, 0, ResultSuccess() },
                { "/aa\\bb/cc", false, true, 9, ResultSuccess() },
                { "/aa\\bb/cc", false, true, 9, ResultSuccess() },
                { "/a|/bb/cc", false, false, 0, fs::ResultInvalidCharacter() },
                { "/>a/bb/cc", false, false, 0, fs::ResultInvalidCharacter() },
                { "/aa/.</cc", false, false, 0, fs::ResultInvalidCharacter() },
                { "/aa/..</cc", false, false, 0, fs::ResultInvalidCharacter() },
                { "\\\\aa/bb/cc", false, false, 0, fs::ResultInvalidPathFormat() },
                { "\\\\aa\\bb\\cc", false, false, 0, fs::ResultInvalidPathFormat() },
                { "/aa/bb/..\\cc", false, true, 12, ResultSuccess() },
                { "/aa/bb\\..\\cc", false, true, 12, ResultSuccess() },
                { "/aa/bb\\..", false, true, 9, ResultSuccess() },
                { "/aa\\bb/../cc", false, false, 0, ResultSuccess() }
            };

            return DoIsNormalizedTests(Tests);
        }
        static_assert(TestIsNormalized());

    }

}
