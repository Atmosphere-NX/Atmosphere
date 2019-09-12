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

#include "hidmitm_main.hpp"
#include "hid_mitm_service.hpp"

#include "../utils.hpp"

struct HidMitmManagerOptions {
    static const size_t PointerBufferSize = 0x200;
    static const size_t MaxDomains = 0x0;
    static const size_t MaxDomainObjects = 0x0;
};
using HidMitmManager = WaitableManager<HidMitmManagerOptions>;

void HidMitmMain(void *arg) {
    /* This is only necessary on 9.x+ */
    if (GetRuntimeFirmwareVersion() < FirmwareVersion_900) {
        return;
    }

    /* Ensure we can talk to HID. */
    Utils::WaitHidAvailable();

    /* Create server manager */
    static auto s_server_manager = HidMitmManager(1);

    /* Create hid mitm. */
    /* Note: official HID passes 100 as sessions, despite this being > 0x40. */
    AddMitmServerToManager<HidMitmService>(&s_server_manager, "hid", 100);

    /* Loop forever, servicing our services. */
    s_server_manager.Process();
}
