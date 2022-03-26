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
#include "sm_utils.hpp"

namespace ams::sm {

    #if defined(ATMOSPHERE_OS_HORIZON)
    namespace {

        constinit int g_ref_count = 0;
        constinit os::SdkMutex g_mutex;

    }

    /* Initialization. */
    Result Initialize() {
        std::scoped_lock lk(g_mutex);

        if (g_ref_count > 0) {
            ++g_ref_count;
        } else {
            R_TRY(::smInitialize());
            g_ref_count = 1;
        }

        R_SUCCEED();
    }

    Result Finalize() {
        /* NOTE: Nintendo does nothing here. */
        R_SUCCEED();
    }

    /* Ordinary SM API. */
    Result GetServiceHandle(os::NativeHandle *out, ServiceName name) {
        R_RETURN(smGetServiceOriginal(out, impl::ConvertName(name)));
    }

    Result RegisterService(os::NativeHandle *out, ServiceName name, size_t max_sessions, bool is_light) {
        R_RETURN(smRegisterService(out, impl::ConvertName(name), is_light, static_cast<int>(max_sessions)));
    }

    Result UnregisterService(ServiceName name) {
        R_RETURN(smUnregisterService(impl::ConvertName(name)));
    }

    /* Atmosphere extensions. */
    Result HasService(bool *out, ServiceName name) {
        R_RETURN(smAtmosphereHasService(out, impl::ConvertName(name)));
    }

    Result WaitService(ServiceName name) {
        R_RETURN(smAtmosphereWaitService(impl::ConvertName(name)));
    }
    #else
    Result Initialize() {
        R_SUCCEED();
    }

    Result Finalize() {
        R_SUCCEED();
    }

    /* Ordinary SM API. */
    Result GetServiceHandle(os::NativeHandle *out, ServiceName name) {
        AMS_UNUSED(out, name);
        AMS_ABORT("TODO?");
    }

    Result RegisterService(os::NativeHandle *out, ServiceName name, size_t max_sessions, bool is_light) {
        AMS_UNUSED(out, name, max_sessions, is_light);
        AMS_ABORT("TODO?");
    }

    Result UnregisterService(ServiceName name) {
        AMS_UNUSED(name);
        AMS_ABORT("TODO?");
    }

    /* Atmosphere extensions. */
    Result HasService(bool *out, ServiceName name) {
        AMS_UNUSED(out, name);
        AMS_ABORT("TODO?");
    }

    Result WaitService(ServiceName name) {
        AMS_UNUSED(name);
        AMS_ABORT("TODO?");
    }
    #endif

}
