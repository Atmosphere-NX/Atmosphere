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
#include <stratosphere/fs/fs_priority.hpp>
#include <stratosphere/fs/impl/fs_fs_inline_context_utils.hpp>

namespace ams::fs::impl {

    enum TlsIoPriority : u8 {
        TlsIoPriority_Normal     = 0,
        TlsIoPriority_Realtime   = 1,
        TlsIoPriority_Low        = 2,
        TlsIoPriority_Background = 3,
    };

    /* Ensure that TlsIo priority matches libnx priority. */
    static_assert(TlsIoPriority_Normal     == static_cast<TlsIoPriority>(::FsPriority_Normal));
    static_assert(TlsIoPriority_Realtime   == static_cast<TlsIoPriority>(::FsPriority_Realtime));
    static_assert(TlsIoPriority_Low        == static_cast<TlsIoPriority>(::FsPriority_Low));
    static_assert(TlsIoPriority_Background == static_cast<TlsIoPriority>(::FsPriority_Background));

    constexpr inline Result ConvertFsPriorityToTlsIoPriority(u8 *out, PriorityRaw priority) {
        AMS_ASSERT(out != nullptr);

        switch (priority) {
            case PriorityRaw_Normal:     *out = TlsIoPriority_Normal;     break;
            case PriorityRaw_Realtime:   *out = TlsIoPriority_Realtime;   break;
            case PriorityRaw_Low:        *out = TlsIoPriority_Low;        break;
            case PriorityRaw_Background: *out = TlsIoPriority_Background; break;
            default: return fs::ResultInvalidArgument();
        }

        return ResultSuccess();
    }

    constexpr inline Result ConvertTlsIoPriorityToFsPriority(PriorityRaw *out, u8 tls_io) {
        AMS_ASSERT(out != nullptr);

        switch (static_cast<TlsIoPriority>(tls_io)) {
            case TlsIoPriority_Normal:     *out = PriorityRaw_Normal;     break;
            case TlsIoPriority_Realtime:   *out = PriorityRaw_Realtime;   break;
            case TlsIoPriority_Low:        *out = PriorityRaw_Low;        break;
            case TlsIoPriority_Background: *out = PriorityRaw_Background; break;
            default: return fs::ResultInvalidArgument();
        }

        return ResultSuccess();
    }

    inline u8 GetTlsIoPriority(os::ThreadType *thread) {
        return sf::GetFsInlineContext(thread) & TlsIoPriorityMask;
    }

}
