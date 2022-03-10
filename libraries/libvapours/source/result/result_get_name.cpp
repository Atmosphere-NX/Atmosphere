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
#if !defined(ATMOSPHERE_OS_HORIZON) && (defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING))
    #define AMS_AUTO_GENERATE_RESULT_NAMES
#endif

#include <vapours.hpp>

namespace ams {

    #if defined(AMS_AUTO_GENERATE_RESULT_NAMES)

    #define AMS_INVOKE_MACRO_01(EXPR, n) EXPR(n); EXPR(n + (1 << 0));

    #define AMS_INVOKE_MACRO_02(EXPR, n) AMS_INVOKE_MACRO_01(EXPR, n); AMS_INVOKE_MACRO_01(EXPR, n + (1 <<  1));
    #define AMS_INVOKE_MACRO_03(EXPR, n) AMS_INVOKE_MACRO_02(EXPR, n); AMS_INVOKE_MACRO_02(EXPR, n + (1 <<  2));
    #define AMS_INVOKE_MACRO_04(EXPR, n) AMS_INVOKE_MACRO_03(EXPR, n); AMS_INVOKE_MACRO_03(EXPR, n + (1 <<  3));
    #define AMS_INVOKE_MACRO_05(EXPR, n) AMS_INVOKE_MACRO_04(EXPR, n); AMS_INVOKE_MACRO_04(EXPR, n + (1 <<  4));
    #define AMS_INVOKE_MACRO_06(EXPR, n) AMS_INVOKE_MACRO_05(EXPR, n); AMS_INVOKE_MACRO_05(EXPR, n + (1 <<  5));
    #define AMS_INVOKE_MACRO_07(EXPR, n) AMS_INVOKE_MACRO_06(EXPR, n); AMS_INVOKE_MACRO_06(EXPR, n + (1 <<  6));
    #define AMS_INVOKE_MACRO_08(EXPR, n) AMS_INVOKE_MACRO_07(EXPR, n); AMS_INVOKE_MACRO_07(EXPR, n + (1 <<  7));
    #define AMS_INVOKE_MACRO_09(EXPR, n) AMS_INVOKE_MACRO_08(EXPR, n); AMS_INVOKE_MACRO_08(EXPR, n + (1 <<  8));
    #define AMS_INVOKE_MACRO_10(EXPR, n) AMS_INVOKE_MACRO_09(EXPR, n); AMS_INVOKE_MACRO_09(EXPR, n + (1 <<  9));
    #define AMS_INVOKE_MACRO_11(EXPR, n) AMS_INVOKE_MACRO_10(EXPR, n); AMS_INVOKE_MACRO_10(EXPR, n + (1 << 10));
    #define AMS_INVOKE_MACRO_12(EXPR, n) AMS_INVOKE_MACRO_11(EXPR, n); AMS_INVOKE_MACRO_11(EXPR, n + (1 << 11));
    #define AMS_INVOKE_MACRO_13(EXPR, n) AMS_INVOKE_MACRO_12(EXPR, n); AMS_INVOKE_MACRO_12(EXPR, n + (1 << 12));

    namespace {

        template<int Module, int Description>
        constexpr const char *GetResultNameByModuleAndDescription() {
            return ::ams::result::impl::ResultNameSpaceExistsImpl<Module>::template NameHolder<Description>::Name;
        }

        template<int Module>
        constexpr const char *GetResultNameByModule(int description) {
            #define AMS_TEST_RESULT_DESCRIPTION_DEFINED(n) if constexpr (::ams::result::impl::ResultNameSpaceExistsImpl<Module>::template NameHolder<n>::Exists) { if (description == n) { return GetResultNameByModuleAndDescription<Module, n>(); } }

            AMS_INVOKE_MACRO_13(AMS_TEST_RESULT_DESCRIPTION_DEFINED, 0)

            return "Unknown";
        }

    }

    const char *GetResultName(int module, int description) {
        #define AMS_TEST_RESULT_MODULE_DEFINED(n) if constexpr (::ams::result::impl::ResultNameSpaceExistsImpl<n>::Exists) { if (module == n) { return GetResultNameByModule<n>(description); } }

        AMS_INVOKE_MACRO_08(AMS_TEST_RESULT_MODULE_DEFINED, 0)

        return "Unknown";
    }

    #else
    const char *GetResultName(int, int) {
        return "Unknown";
    }
    #endif

}
