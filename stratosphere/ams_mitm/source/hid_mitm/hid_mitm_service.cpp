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

#include <unordered_map>
#include <switch.h>
#include <stratosphere.hpp>

#include "hid_shim.h"
#include "hid_mitm_service.hpp"

namespace {

    std::unordered_map<u64, u64> g_hbl_map;

    bool ShouldSetSystemExt(u64 process_id) {
        return g_hbl_map.find(process_id) != g_hbl_map.end();
    }

    void AddSession(u64 process_id) {
        if (g_hbl_map.find(process_id) != g_hbl_map.end()) {
            g_hbl_map[process_id]++;
        } else {
            g_hbl_map[process_id] = 0;
        }
    }

    void RemoveSession(u64 process_id) {
        if ((--g_hbl_map[process_id]) == 0) {
            g_hbl_map.erase(process_id);
        }
    }

}

HidMitmService::~HidMitmService() {
    if (this->should_set_system_ext) {
        RemoveSession(this->process_id);
    }
}

void HidMitmService::PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx) {
    /* Nothing to do here */
}

Result HidMitmService::SetSupportedNpadStyleSet(u64 applet_resource_user_id, u32 style_set, PidDescriptor pid_desc) {
    if (!this->should_set_system_ext && (style_set & (TYPE_SYSTEM | TYPE_SYSTEM_EXT))) {
        /* Guaranteed: hbmenu 3.1.1 will cause this to be true. */
        /* This prevents setting this for non-HBL. */
        this->should_set_system_ext = true;
        AddSession(this->process_id);
    }
    if (ShouldSetSystemExt(this->process_id)) {
        style_set |= TYPE_SYSTEM | TYPE_SYSTEM_EXT;
    }
    return hidSetSupportedNpadStyleSetFwd(this->forward_service.get(), this->process_id, applet_resource_user_id, static_cast<HidControllerType>(style_set));
}
