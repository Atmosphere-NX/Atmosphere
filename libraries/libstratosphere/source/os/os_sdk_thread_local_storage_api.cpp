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
#include "impl/os_tls_manager.hpp"

namespace ams::os {

    #if defined(ATMOSPHERE_OS_HORIZON)
    /* TODO: Nintendo reserves half the TLS slots for SDK usage. */
    /* We don't have that ability...how should this work? */
    Result SdkAllocateTlsSlot(TlsSlot *out, TlsDestructor destructor) {
        R_RETURN(os::AllocateTlsSlot(out, destructor));
    }
    #else
    Result SdkAllocateTlsSlot(TlsSlot *out, TlsDestructor destructor) {
        R_UNLESS(impl::GetTlsManager().AllocateTlsSlot(out, destructor, true), os::ResultOutOfResource());
        R_SUCCEED();
    }
    #endif

}
