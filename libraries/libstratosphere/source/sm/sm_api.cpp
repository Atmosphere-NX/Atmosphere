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

        return ResultSuccess();
    }

    Result Finalize() {
        /* NOTE: Nintendo does nothing here. */
        return ResultSuccess();
    }

    /* Ordinary SM API. */
    Result GetService(Service *out, ServiceName name) {
        return smGetServiceWrapper(out, impl::ConvertName(name));
    }

    Result RegisterService(os::NativeHandle *out, ServiceName name, size_t max_sessions, bool is_light) {
        return smRegisterService(out, impl::ConvertName(name), is_light, static_cast<int>(max_sessions));
    }

    Result UnregisterService(ServiceName name) {
        return smUnregisterService(impl::ConvertName(name));
    }

    /* Atmosphere extensions. */
    Result HasService(bool *out, ServiceName name) {
        return smAtmosphereHasService(out, impl::ConvertName(name));
    }

    Result WaitService(ServiceName name) {
        return smAtmosphereWaitService(impl::ConvertName(name));
    }

}
