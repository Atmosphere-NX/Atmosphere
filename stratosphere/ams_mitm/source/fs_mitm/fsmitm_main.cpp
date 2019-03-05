/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include "fsmitm_main.hpp"
#include "fsmitm_service.hpp"

#include "../utils.hpp"

struct FsMitmManagerOptions {
    static const size_t PointerBufferSize = 0x800;
    static const size_t MaxDomains = 0x40;
    static const size_t MaxDomainObjects = 0x4000;
};
using FsMitmManager = WaitableManager<FsMitmManagerOptions>;

void FsMitmMain(void *arg) {    
    /* Create server manager. */
    auto server_manager = new FsMitmManager(5);

    /* Create fsp-srv mitm. */
    AddMitmServerToManager<FsMitmService>(server_manager, "fsp-srv", 61);
                
    /* Loop forever, servicing our services. */
    server_manager->Process();
    
    delete server_manager;
}

