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

namespace ams::sm::mitm {

    /* Mitm API. */
    Result InstallMitm(Handle *out_port, Handle *out_query, ServiceName name) {
        return impl::DoWithPerThreadSession([&](Service *fwd) {
            return smAtmosphereMitmInstall(fwd, out_port, out_query, impl::ConvertName(name));
        });
    }

    Result UninstallMitm(ServiceName name) {
        return impl::DoWithUserSession([&]() {
            return smAtmosphereMitmUninstall(impl::ConvertName(name));
        });
    }

    Result DeclareFutureMitm(ServiceName name) {
        return impl::DoWithUserSession([&]() {
            return smAtmosphereMitmDeclareFuture(impl::ConvertName(name));
        });
    }

    Result ClearFutureMitm(ServiceName name) {
        return impl::DoWithUserSession([&]() {
            return smAtmosphereMitmClearFuture(impl::ConvertName(name));
        });
    }

    Result AcknowledgeSession(Service *out_service, MitmProcessInfo *out_info, ServiceName name) {
        return impl::DoWithMitmAcknowledgementSession([&]() {
            return smAtmosphereMitmAcknowledgeSession(out_service, reinterpret_cast<void *>(out_info), impl::ConvertName(name));
        });
    }

    Result HasMitm(bool *out, ServiceName name) {
        return impl::DoWithUserSession([&]() {
            return smAtmosphereHasMitm(out, impl::ConvertName(name));
        });
    }

    Result WaitMitm(ServiceName name) {
        return impl::DoWithUserSession([&]() {
            return smAtmosphereWaitMitm(impl::ConvertName(name));
        });
    }

}
