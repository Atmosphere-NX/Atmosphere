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

namespace ams::sm::mitm {

    #if defined(ATMOSPHERE_OS_HORIZON)
    #if AMS_SF_MITM_SUPPORTED
    /* Mitm API. */
    Result InstallMitm(os::NativeHandle *out_port, os::NativeHandle *out_query, ServiceName name) {
        R_RETURN(impl::DoWithPerThreadSession([&](TipcService *fwd) {
            R_RETURN(smAtmosphereMitmInstall(fwd, out_port, out_query, impl::ConvertName(name)));
        }));
    }

    Result UninstallMitm(ServiceName name) {
        R_RETURN(smAtmosphereMitmUninstall(impl::ConvertName(name)));
    }

    Result DeclareFutureMitm(ServiceName name) {
        R_RETURN(smAtmosphereMitmDeclareFuture(impl::ConvertName(name)));
    }

    Result ClearFutureMitm(ServiceName name) {
        R_RETURN(smAtmosphereMitmClearFuture(impl::ConvertName(name)));
    }

    Result AcknowledgeSession(Service *out_service, MitmProcessInfo *out_info, ServiceName name) {
        return impl::DoWithMitmAcknowledgementSession([&]() {
            R_RETURN(smAtmosphereMitmAcknowledgeSession(out_service, reinterpret_cast<void *>(out_info), impl::ConvertName(name)));
        });
    }

    Result HasMitm(bool *out, ServiceName name) {
        R_RETURN(smAtmosphereHasMitm(out, impl::ConvertName(name)));
    }

    Result WaitMitm(ServiceName name) {
        R_RETURN(smAtmosphereWaitMitm(impl::ConvertName(name)));
    }
    #endif
    #endif

}
