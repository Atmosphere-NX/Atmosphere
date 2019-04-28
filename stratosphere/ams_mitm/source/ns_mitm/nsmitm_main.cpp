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

#include "../utils.hpp"

#include "nsmitm_main.hpp"

#include "nsmitm_am_service.hpp"
#include "nsmitm_web_service.hpp"

void NsMitmMain(void *arg) {
    /* Wait for initialization to occur */
    Utils::WaitSdInitialized();

    /* Ensure we can talk to NS. */
    DoWithSmSession([&]() {
        if (R_FAILED(nsInitialize())) {
            std::abort();
        }
        nsExit();
    });

    /* Create server manager */
    auto server_manager = new WaitableManager(1);
    
    /* Create ns mitm. */
    if (GetRuntimeFirmwareVersion() < FirmwareVersion_300) {
        AddMitmServerToManager<NsAmMitmService>(server_manager, "ns:am", 5);
    } else {
        AddMitmServerToManager<NsWebMitmService>(server_manager, "ns:web", 5);
    }

    /* Loop forever, servicing our services. */
    server_manager->Process();
    
    delete server_manager;
    
}

