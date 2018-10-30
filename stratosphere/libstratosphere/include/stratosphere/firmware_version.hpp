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
 
#pragma once

enum FirmwareVersion : u32 {
    FirmwareVersion_Min = 0,
    FirmwareVersion_100 = FirmwareVersion_Min,
    FirmwareVersion_200 = 1,
    FirmwareVersion_300 = 2,
    FirmwareVersion_400 = 3,
    FirmwareVersion_500 = 4,
    FirmwareVersion_600 = 5,
    FirmwareVersion_Current = FirmwareVersion_600,
    FirmwareVersion_Max = 32,
};

static inline FirmwareVersion GetRuntimeFirmwareVersion() {
    FirmwareVersion fw = FirmwareVersion_Min;
    if (kernelAbove200()) {
        fw = FirmwareVersion_200;
    }
    if (kernelAbove300()) {
        fw = FirmwareVersion_300;
    }
    if (kernelAbove400()) {
        fw = FirmwareVersion_400;
    }
    if (kernelAbove500()) {
        fw = FirmwareVersion_500;
    }
    if (kernelAbove600()) {
        fw = FirmwareVersion_600;
    }
    return fw;
}
