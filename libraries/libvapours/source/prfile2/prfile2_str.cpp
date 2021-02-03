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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif

namespace ams::prfile2::str {

    namespace {

        /* TODO: Where does this come from? */
        /* It's maximum path length * 2, but where should the definition live? */
        constexpr inline size_t StringLengthMax = 520;

    }

    pf::Error Initialize(String *str, const char *s, CodeMode code_mode) {
        /* Check parameters. */
        if (str == nullptr || s == nullptr) {
            return pf::Error_InvalidParameter;
        }

        /* Initialize the string. */
        switch (code_mode) {
            case CodeMode_Local:
                {
                    str->head = s;
                    str->tail = s + sizeof(char) * strnlen(s, StringLengthMax);
                }
                break;
            case CodeMode_Unicode:
                {
                    str->head = s;
                    str->tail = s + sizeof(WideChar) * w_strnlen(reinterpret_cast<const WideChar *>(s), StringLengthMax);
                }
                break;
            default:
                return pf::Error_InvalidParameter;
        }

        /* Set the code mode. */
        str->code_mode = code_mode;

        return pf::Error_Ok;
    }

    void SetCodeMode(String *str, CodeMode code_mode) {
        str->code_mode = code_mode;
    }

    CodeMode GetCodeMode(const String *str) {
        return str->code_mode;
    }

    char *GetPos(String *str, TargetString target) {
        if (target == TargetString_Head) {
            return const_cast<char *>(str->head);
        } else {
            return const_cast<char *>(str->tail);
        }
    }

    void MovePos(String *str, s16 num_char) {
        AMS_UNUSED(str, num_char);
        AMS_ABORT("TODO: oem charset");
    }

    u16 GetLength(String *str) {
        if (str->code_mode == CodeMode_Unicode) {
            return (str->tail - str->head) / sizeof(WideChar);
        } else {
            return (str->tail - str->head) / sizeof(char);
        }
    }

    u16 GetNumChar(String *str, TargetString target) {
        AMS_UNUSED(str, target);
        AMS_ABORT("TODO: oem charset");
    }

    int Compare(const String *str, const char *rhs) {
        AMS_UNUSED(str, rhs);
        AMS_ABORT("TODO: oem charset");
    }

    int Compare(const String *str, const WideChar *rhs) {
        AMS_UNUSED(str, rhs);
        AMS_ABORT("TODO: oem charset");
    }

}
