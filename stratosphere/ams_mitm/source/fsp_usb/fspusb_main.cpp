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

#include "fspusb_main.hpp"
#include "fspusb_service.hpp"

// fsp-srv's ones, but without domains (need this specific buf size as we're using fsp-like IFileSystems)
struct FspUsbManagerOptions {
    static const size_t PointerBufferSize = 0x800;
    static const size_t MaxDomains = 0;
    static const size_t MaxDomainObjects = 0;
};

using FspUsbManager = WaitableManager<FspUsbManagerOptions>;

void USBUpdateLoop(void *arg) {
    while(true) {
        USBDriveSystem::Update();
        svcSleepThread(100000000L);
    }
}

void FspUsbMain(void *arg) {
    /* Wait for initialization to occur */
    Utils::WaitSdInitialized();

    Result rc;
    /* Initialize USB system (usb:hs service). */
    DoWithSmSession([&]() {
        do
        {
            rc = USBDriveSystem::Initialize();
        } while(rc != 0);
    });

    /* Create server manager */
    static auto s_server_manager = FspUsbManager(5);

    HosThread t_usb_loop;
    t_usb_loop.Initialize(&USBUpdateLoop, nullptr, 0x4000, 0x2b);
    t_usb_loop.Start();

    /* Create fsp-usb. */
    s_server_manager.AddWaitable(new ServiceServer<FspUsbService>("fsp-usb", 0x20));

    /* Loop forever, servicing our services. */
    s_server_manager.Process();
}