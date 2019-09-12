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

#include "hid_shim.h"
#include "hid_mitm_service.hpp"

void HidMitmService::PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx) {
    /* Nothing to do here */
}

Result HidMitmService::SetSupportedNpadStyleSet(u64 applet_resource_user_id, u32 style_set, PidDescriptor pid_desc) {
    const HidControllerType new_style_set = static_cast<HidControllerType>(style_set | TYPE_SYSTEM | TYPE_SYSTEM_EXT);
    return hidSetSupportedNpadStyleSetFwd(this->forward_service.get(), this->process_id, applet_resource_user_id, new_style_set);;
}
