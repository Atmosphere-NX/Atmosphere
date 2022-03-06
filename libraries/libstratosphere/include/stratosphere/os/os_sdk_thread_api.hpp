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

#pragma once
#include <stratosphere/os/os_common_types.hpp>
#include <stratosphere/os/os_thread_types.hpp>
#include <stratosphere/os/os_sdk_thread_types.hpp>
#include <stratosphere/os/os_thread_local_storage_common.hpp>

namespace ams::os {

    ALWAYS_INLINE SdkInternalTlsType *GetSdkInternalTlsArray(ThreadType *thread = os::GetCurrentThread()) {
        #if defined(ATMOSPHERE_OS_HORIZON)
        return std::addressof(thread->sdk_internal_tls);
        #else
        return reinterpret_cast<SdkInternalTlsType *>(std::addressof(thread->tls_value_array[TlsSlotCountMax]));
        #endif
    }

}
