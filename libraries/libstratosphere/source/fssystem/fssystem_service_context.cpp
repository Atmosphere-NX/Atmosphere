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

namespace ams::fssystem {

    namespace {

        os::SdkThreadLocalStorage g_tls_service_context;

    }

    void RegisterServiceContext(ServiceContext *context) {
        /* Check pre-conditions. */
        AMS_ASSERT(context != nullptr);
        AMS_ASSERT(g_tls_service_context.GetValue() == 0);

        /* Register context. */
        g_tls_service_context.SetValue(reinterpret_cast<uintptr_t>(context));
    }

    void UnregisterServiceContext() {
        /* Unregister context. */
        g_tls_service_context.SetValue(0);
    }

    ServiceContext *GetServiceContext() {
        /* Get context. */
        auto * const context = reinterpret_cast<ServiceContext *>(g_tls_service_context.GetValue());
        AMS_ASSERT(context != nullptr);

        return context;
    }

}
