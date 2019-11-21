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

#include "setmitm_main.hpp"
#include "setsys_mitm_service.hpp"
#include "setsys_settings_items.hpp"
#include "setsys_firmware_version.hpp"

#include "set_mitm_service.hpp"

#include "../utils.hpp"

struct SetSysManagerOptions {
    static const size_t PointerBufferSize = 0x100;
    static const size_t MaxDomains = 4;
    static const size_t MaxDomainObjects = 0x100;
};

using SetMitmManager = WaitableManager<SetSysManagerOptions>;

void SetMitmMain(void *arg) {
    /* Wait for SD to initialize. */
    Utils::WaitSdInitialized();

    /* Initialize version manager. */
    VersionManager::Initialize();

    /* Create server manager */
    static auto s_server_manager = SetMitmManager(4);

    /* Create set:sys mitm. */
    AddMitmServerToManager<SetSysMitmService>(&s_server_manager, "set:sys", 60);

    /* Create set mitm. */
    AddMitmServerToManager<SetMitmService>(&s_server_manager, "set", 60);

    /* Loop forever, servicing our services. */
    s_server_manager.Process();
}

