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
#include "os_utility.hpp"

namespace ams::os::impl {

    namespace {

        constexpr void ExpandUnsignedValueToAsciiImpl(char *dst, u64 value) {
            /* Determine size needed. */
            constexpr size_t SizeNeeded = (BITSIZEOF(value) / 4) * sizeof(char);

            /* Write null-terminator. */
            dst[SizeNeeded] = '\x00';

            /* Write characters. */
            for (size_t i = 0; i < SizeNeeded; ++i) {
                /* Determine current character. */
                const auto nybble = (value & 0xF);
                const char cur_c  = (nybble < 10) ? ('0' + nybble) : ('A' + nybble - 10);

                /* Write current character. */
                dst[SizeNeeded - 1 - i] = cur_c;

                /* Shift to next nybble. */
                value >>= 4;
            }
        }

    }

    void ExpandUnsignedValueToAscii(char *dst, u64 value) {
        return ExpandUnsignedValueToAsciiImpl(dst, value);
    }

    /* Test correctness. */
    #if 0
    namespace {

        consteval bool TestExpandUnsignedValueToAsciiImpl(const char *expected, u64 value) {
            /* Create buffer. */
            char buffer[17] = {
                '\xCC', '\xCC', '\xCC', '\xCC', '\xCC', '\xCC', '\xCC', '\xCC', '\xCC', '\xCC', '\xCC', '\xCC', '\xCC', '\xCC', '\xCC', '\xCC', '\xCC'
            };

            /* Validate buffer is initially garbage. */
            for (size_t i = 0; i < util::size(buffer); ++i) {
                if (buffer[i] != '\xCC') {
                    return false;
                }
            }

            /* Expand the value into the buffer. */
            ExpandUnsignedValueToAsciiImpl(buffer, value);

            /* Verify the result. */
            for (size_t i = 0; i < util::size(buffer); ++i) {
                if (buffer[i] != expected[i]) {
                    return false;
                }
            }

            /* Verify null-termination. */
            if (buffer[util::size(buffer) - 1] != 0) {
                return false;
            }

            return true;
        }

        #define DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL(v) static_assert(TestExpandUnsignedValueToAsciiImpl( #v , UINT64_C(0x##v)))

        DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL(0000000000000000);
        DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL(FFFFFFFFFFFFFFFF);
        DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL(FFFFFFFF00000000);
        DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL(00000000FFFFFFFF);
        DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL(FFFF0000FFFF0000);
        DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL(0000FFFF0000FFFF);
        DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL(F0F0F0F0F0F0F0F0);
        DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL(0F0F0F0F0F0F0F0F);
        DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL(0123456789ABCDEF);
        DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL(FEDCBA9876543210);
        DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL(A5A5A5A5A5A5A5A5);

        #undef DO_TEST_EXPAND_UNSIGNED_VALUE_TO_ASCII_IMPL

    }
    #endif

}
