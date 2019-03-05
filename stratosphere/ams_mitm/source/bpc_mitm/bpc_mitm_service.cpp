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

#include <mutex>
#include <switch.h>
#include <stratosphere.hpp>
#include "bpc_mitm_service.hpp"
#include "bpcmitm_reboot_manager.hpp"

void BpcMitmService::PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx) {
    /* Nothing to do here */
}

Result BpcMitmService::ShutdownSystem() {
    /* Use exosphere + reboot to perform real shutdown, instead of fake shutdown. */
    PerformShutdownSmc();
    return 0;
}

Result BpcMitmService::RebootSystem() {
    return BpcRebootManager::PerformReboot();
}
