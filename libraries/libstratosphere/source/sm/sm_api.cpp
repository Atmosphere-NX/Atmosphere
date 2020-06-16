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
#include <stratosphere.hpp>
#include "sm_utils.hpp"

namespace ams::sm {

    /* Ordinary SM API. */
    Result GetService(Service *out, ServiceName name) {
        return impl::DoWithUserSession([&]() {
            return smGetServiceWrapper(out, impl::ConvertName(name));
        });
    }

    Result RegisterService(Handle *out, ServiceName name, size_t max_sessions, bool is_light) {
        return impl::DoWithUserSession([&]() {
            return smRegisterService(out, impl::ConvertName(name), is_light, static_cast<int>(max_sessions));
        });
    }

    Result UnregisterService(ServiceName name) {
        return impl::DoWithUserSession([&]() {
            return smUnregisterService(impl::ConvertName(name));
        });
    }

    /* Atmosphere extensions. */
    Result HasService(bool *out, ServiceName name) {
        return impl::DoWithUserSession([&]() {
            return smAtmosphereHasService(out, impl::ConvertName(name));
        });
    }

    Result WaitService(ServiceName name) {
        return impl::DoWithUserSession([&]() {
            return smAtmosphereWaitService(impl::ConvertName(name));
        });
    }

    namespace impl {

        void DoWithSessionImpl(void (*Invoker)(void *), void *Function) {
            impl::DoWithUserSession([&]() {
                Invoker(Function);
                return ResultSuccess();
            });
        }

    }

}
