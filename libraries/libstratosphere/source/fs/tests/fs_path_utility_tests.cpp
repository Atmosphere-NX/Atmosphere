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

        struct IsSubPathTestData {
            const char *lhs;
            const char *rhs;
            bool is_sub_path;
        };

        template<size_t N, size_t Ix = 0>
        consteval bool DoIsSubPathTests(const IsSubPathTestData (&tests)[N]) {
            if constexpr (Ix >= N) {
                return true;
            }

            const auto &test = tests[Ix];
            if (::ams::fs::IsSubPath(test.lhs, test.rhs) != test.is_sub_path) {
                return false;
            }

            if constexpr (Ix < N) {
                return DoIsSubPathTests<N, Ix + 1>(tests);
            } else {
                AMS_ASSUME(false);
            }
        }

        consteval bool TestIsSubPath() {
            constexpr IsSubPathTestData Tests[] = {
                { "//a/b", "/a", false },
                { "/a", "//a/b", false },
                { "//a/b", "\\\\a", false },
                { "//a/b", "//a", true },
                { "/", "/a", true },
                { "/a", "/", true },
                { "/", "/", false },
                { "", "", false },
                { "/", "", true },
                { "/", "mount:/a", false },
                { "mount:/", "mount:/", false },
                { "mount:/a/b", "mount:/a/b", false },
                { "mount:/a/b", "mount:/a/b/c", true },
                { "/a/b", "/a/b/c", true },
                { "/a/b/c", "/a/b", true },
                { "/a/b", "/a/b", false },
                { "/a/b", "/a/b\\c", false }
            };

            return DoIsSubPathTests(Tests);
        }
        static_assert(TestIsSubPath());

    }

}
