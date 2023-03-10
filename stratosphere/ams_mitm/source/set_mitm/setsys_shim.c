/*
 * Copyright (c) Atmosphère-NX
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
#include "setsys_shim.h"

Result setsysSetBluetoothDevicesSettingsFwd(Service *s, const SetSysBluetoothDevicesSettings *settings, s32 count) {
    return serviceDispatch(s, 11,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count * sizeof(SetSysBluetoothDevicesSettings) } },
    );
}

Result setsysGetBluetoothDevicesSettingsFwd(Service *s, s32 *total_out, SetSysBluetoothDevicesSettings *settings, s32 count) {
    return serviceDispatchOut(s, 12, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count * sizeof(SetSysBluetoothDevicesSettings) } },
    );
}
