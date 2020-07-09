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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        Result OutputDebugString(KUserPointer<const char *> debug_str, size_t len) {
            /* Succeed immediately if there's nothing to output. */
            R_SUCCEED_IF(len == 0);

            /* Ensure that the data being output is in range. */
            R_UNLESS(GetCurrentProcess().GetPageTable().Contains(KProcessAddress(debug_str.GetUnsafePointer()), len), svc::ResultInvalidCurrentMemory());

            /* Output the string. */
            return KDebugLog::PrintUserString(debug_str, len);
        }

    }

    /* =============================    64 ABI    ============================= */

    Result OutputDebugString64(KUserPointer<const char *> debug_str, ams::svc::Size len) {
        return OutputDebugString(debug_str, len);
    }

    /* ============================= 64From32 ABI ============================= */

    Result OutputDebugString64From32(KUserPointer<const char *> debug_str, ams::svc::Size len) {
        return OutputDebugString(debug_str, len);
    }

}
