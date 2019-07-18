/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/sm.hpp>

#include "sm_ams.h"
#include "sm_utils.hpp"

namespace sts::sm::mitm {

    /* Mitm API. */
    Result InstallMitm(Handle *out_port, Handle *out_query, ServiceName name) {
        return impl::DoWithMitmSession([&]() {
            return smAtmosphereMitmInstall(out_port, out_query, name.name);
        });
    }

    Result UninstallMitm(ServiceName name) {
        return impl::DoWithMitmSession([&]() {
            return smAtmosphereMitmUninstall(name.name);
        });
    }

    Result DeclareFutureMitm(ServiceName name) {
        return impl::DoWithMitmSession([&]() {
            return smAtmosphereMitmDeclareFuture(name.name);
        });
    }

    Result AcknowledgeSession(Service *out_service, u64 *out_pid, ncm::TitleId *out_tid, ServiceName name) {
        return impl::DoWithMitmSession([&]() {
            return smAtmosphereMitmAcknowledgeSession(out_service, out_pid, &out_tid->value, name.name);
        });
    }

    Result HasMitm(bool *out, ServiceName name) {
        return impl::DoWithUserSession([&]() {
            return smAtmosphereHasMitm(out, name.name);
        });
    }

    Result WaitMitm(ServiceName name) {
        return impl::DoWithUserSession([&]() {
            return smAtmosphereWaitMitm(name.name);
        });
    }

}
