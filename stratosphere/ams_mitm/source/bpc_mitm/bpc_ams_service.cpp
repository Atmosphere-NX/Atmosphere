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
#include "../amsmitm_initialization.hpp"
#include "bpc_ams_service.hpp"
#include "bpc_ams_power_utils.hpp"

namespace ams::mitm::bpc {

    namespace {

        bool g_set_initial_payload = false;

    }

    void AtmosphereService::RebootToFatalError(const ams::FatalErrorContext &ctx) {
        bpc::RebootForFatalError(&ctx);
    }

    void AtmosphereService::SetRebootPayload(const ams::sf::InBuffer &payload) {
        /* Set the reboot payload. */
        bpc::SetRebootPayload(payload.GetPointer(), payload.GetSize());

        /* If this is being called for the first time (by boot sysmodule), */
        /* Then we should kick off the rest of init. */
        if (!g_set_initial_payload) {
            g_set_initial_payload = true;

            /* Start the initialization process. */
            ::ams::mitm::StartInitialize();
        }
    }

}
