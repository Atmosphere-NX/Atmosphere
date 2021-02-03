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
#pragma once
#include <vapours/prfile2/prfile2_common.hpp>

namespace ams::prfile2::str {

    enum CodeMode {
        CodeMode_Invalid = 0,
        CodeMode_Local   = 1,
        CodeMode_Unicode = 2,
    };

    enum TargetString {
        TargetString_Head = 1,
        TargetString_Tail = 2,
    };

    struct String {
        const char *head;
        const char *tail;
        CodeMode    code_mode;
    };

    pf::Error Initialize(String *str, const char *s, CodeMode code_mode);

    void SetCodeMode(String *str, CodeMode code_mode);
    CodeMode GetCodeMode(const String *str);

    char *GetPos(String *str, TargetString target);
    void MovePos(String *str, s16 num_char);

    u16 GetLength(String *str);
    u16 GetNumChar(String *str, TargetString target);

    int Compare(const String *str, const char *rhs);
    int Compare(const String *str, const WideChar *rhs);

    /* TODO: StrNCmp */
    /* TODO: ToUpperNStr */

    constexpr bool IsNull(const String *str) {
        return str->head == nullptr;
    }

}

namespace ams::prfile2 {

    using String = str::String;

}
