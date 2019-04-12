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
 
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <atmosphere.h>
#include <stratosphere.hpp>

#include "bpcmitm_main.hpp"
#include "bpc_mitm_service.hpp"
#include "bpc_ams_service.hpp"
#include "bpcmitm_reboot_manager.hpp"

#include "../utils.hpp"

void BpcMitmMain(void *arg) {
    /* Wait for initialization to occur */
    Utils::WaitSdInitialized();
    
    /* Load a payload off of the SD */
    BpcRebootManager::Initialize();
    
    /* Create server manager */
    auto server_manager = new WaitableManager(2);
    
    /* Create bpc mitm. */
    const char *service_name = "bpc";
    if (GetRuntimeFirmwareVersion() < FirmwareVersion_200) {
        service_name = "bpc:c";
    }
    AddMitmServerToManager<BpcMitmService>(server_manager, service_name, 13);

    /* Extension: Allow for reboot-to-error. */
    /* Must be managed port in order for sm to be able to access. */
    server_manager->AddWaitable(new ManagedPortServer<BpcAtmosphereService>("bpc:ams", 1));

    /* Loop forever, servicing our services. */
    server_manager->Process();
    
    delete server_manager;
    
}

