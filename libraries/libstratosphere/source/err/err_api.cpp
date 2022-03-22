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
#include "impl/err_string_util.hpp"

namespace ams::err {

    namespace impl {

        namespace {

            constexpr int ErrorCodeCategoryPlatformPrefixForResultModule = 2000;

            ALWAYS_INLINE ErrorCode ConvertResultToErrorCode(const Result &result) {
                return {
                    .category = static_cast<ErrorCodeCategory>(ErrorCodeCategoryPlatformPrefixForResultModule + result.GetModule()),
                    .number   = static_cast<ErrorCodeNumber>(result.GetDescription()),
                };
            }

            ALWAYS_INLINE Result ConvertErrorCodeToResult(const ErrorCode &error_code) {
                const auto result_value = ::ams::result::impl::ResultTraits::MakeValue(error_code.category - ErrorCodeCategoryPlatformPrefixForResultModule, error_code.number);
                return ::ams::result::impl::MakeResult(result_value);
            }

        }

    }

    ErrorCode ConvertResultToErrorCode(const Result &result) {
        AMS_ASSERT(R_FAILED(result));

        return ::ams::err::impl::ConvertResultToErrorCode(result);
    }

    void GetErrorCodeString(char *dst, size_t dst_size, ErrorCode error_code) {
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(dst_size >= static_cast<size_t>(ErrorCode::StringLengthMax));
        AMS_ASSERT(error_code.IsValid());

        return ::ams::err::impl::MakeErrorCodeString(dst, dst_size, error_code);
    }

}
