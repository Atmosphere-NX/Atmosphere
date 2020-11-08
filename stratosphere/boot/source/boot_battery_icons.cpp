/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere.hpp>
#include "boot_battery_icons.hpp"
#include "boot_display.hpp"

namespace ams::boot {

    namespace {

        /* Pull in icon definitions. */
        #include "boot_battery_icon_low.inc"
        #include "boot_battery_icon_charging.inc"
        #include "boot_battery_icon_charging_red.inc"

        /* Helpers. */
        void FillBatteryMeter(u32 *icon, const size_t icon_w, const size_t icon_h, const size_t meter_x, const size_t meter_y, const size_t meter_w, const size_t meter_h, const size_t fill_w) {
            const size_t fill_x = meter_x + meter_w - fill_w;

            if (fill_x + fill_w > icon_w || meter_y + meter_h > icon_h || fill_x == 0) {
                return;
            }

            u32 *cur_row = icon + meter_y * icon_w + fill_x;
            for (size_t y = 0; y < meter_h; y++) {
                /* Make last column of meter identical to first column of meter. */
                cur_row[-1] = icon[(meter_y + y) * icon_w + meter_x];

                /* Black out further pixels. */
                for (size_t x = 0; x < fill_w; x++) {
                    cur_row[x] = 0xFF000000;
                }
                cur_row += icon_w;
            }
        }

    }

    void ShowLowBatteryIcon() {
        InitializeDisplay();
        {
            /* Low battery icon is shown for 5 seconds. */
            ShowDisplay(LowBatteryX, LowBatteryY, LowBatteryW, LowBatteryH, LowBattery);
            os::SleepThread(TimeSpan::FromSeconds(5));
        }
        FinalizeDisplay();
    }

    void StartShowChargingIcon(int battery_percentage, bool wait) {
        const bool is_red = battery_percentage <= 15;

        const size_t IconX = is_red ? ChargingRedBatteryX : ChargingBatteryX;
        const size_t IconY = is_red ? ChargingRedBatteryY : ChargingBatteryY;
        const size_t IconW = is_red ? ChargingRedBatteryW : ChargingBatteryW;
        const size_t IconH = is_red ? ChargingRedBatteryH : ChargingBatteryH;
        const size_t IconMeterX = is_red ? ChargingRedBatteryMeterX : ChargingBatteryMeterX;
        const size_t IconMeterY = is_red ? ChargingRedBatteryMeterY : ChargingBatteryMeterY;
        const size_t IconMeterW = is_red ? ChargingRedBatteryMeterW : ChargingBatteryMeterW;
        const size_t IconMeterH = is_red ? ChargingRedBatteryMeterH : ChargingBatteryMeterH;
        const size_t MeterFillW = static_cast<size_t>(IconMeterW * (1.0 - (0.0404 + 0.0096 * static_cast<double>(battery_percentage))) + 0.5);

        /* Create stack buffer, copy icon into it, draw fill meter, draw. */
        {
            u32 Icon[IconW * IconH];
            std::memcpy(Icon, is_red ? ChargingRedBattery : ChargingBattery, sizeof(Icon));
            FillBatteryMeter(Icon, IconW, IconH, IconMeterX, IconMeterY, IconMeterW, IconMeterH, MeterFillW);

            InitializeDisplay();
            ShowDisplay(IconX, IconY, IconW, IconH, Icon);
        }

        /* Wait for 2 seconds if we're supposed to. */
        if (wait) {
            os::SleepThread(TimeSpan::FromSeconds(2));
        }
    }

    void EndShowChargingIcon() {
        FinalizeDisplay();
    }

}
