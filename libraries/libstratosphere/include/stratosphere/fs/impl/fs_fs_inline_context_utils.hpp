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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/sf/sf_fs_inline_context.hpp>

namespace ams::fs::impl {

    constexpr inline u8 TlsIoPriorityMask      = 0x7;
    constexpr inline u8 TlsIoRecursiveCallMask = 0x8;

    struct TlsIoValueForInheritance {
        u8 _tls_value;
    };

    inline void SetCurrentRequestRecursive() {
        os::ThreadType * const cur_thread = os::GetCurrentThread();
        sf::SetFsInlineContext(cur_thread, TlsIoRecursiveCallMask | sf::GetFsInlineContext(cur_thread));
    }

    inline bool IsCurrentRequestRecursive() {
        return (sf::GetFsInlineContext(os::GetCurrentThread()) & TlsIoRecursiveCallMask) != 0;
    }

    inline TlsIoValueForInheritance GetTlsIoValueForInheritance() {
        return TlsIoValueForInheritance { sf::GetFsInlineContext(os::GetCurrentThread()) };
    }

    inline void SetTlsIoValueForInheritance(TlsIoValueForInheritance tls_io) {
        sf::SetFsInlineContext(os::GetCurrentThread(), tls_io._tls_value);
    }

}
