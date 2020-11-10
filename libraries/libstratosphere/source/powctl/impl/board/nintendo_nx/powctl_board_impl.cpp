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
#include "../../powctl_device_management.hpp"
#include "powctl_board_impl.hpp"
#include "powctl_battery_driver.hpp"
#include "powctl_charger_driver.hpp"

namespace ams::powctl::impl::board::nintendo_nx {

    namespace {

        constinit std::optional<ChargerDriver> g_charger_driver;
        constinit std::optional<BatteryDriver> g_battery_driver;

        void InitializeChargerDriver(bool use_event_handlers) {
            /* Create the charger driver. */
            g_charger_driver.emplace(use_event_handlers);

            /* Register the driver. */
            powctl::impl::RegisterDriver(std::addressof(*g_charger_driver));
        }

        void InitializeBatteryDriver(bool use_event_handlers) {
            /* Create the battery driver. */
            g_battery_driver.emplace(use_event_handlers);

            /* Register the driver. */
            powctl::impl::RegisterDriver(std::addressof(*g_battery_driver));
        }

        void FinalizeChargerDriver() {
            /* Unregister the driver. */
            powctl::impl::UnregisterDriver(std::addressof(*g_charger_driver));

            /* Destroy the battery driver. */
            g_charger_driver = std::nullopt;
        }

        void FinalizeBatteryDriver() {
            /* Unregister the driver. */
            powctl::impl::UnregisterDriver(std::addressof(*g_battery_driver));

            /* Destroy the battery driver. */
            g_battery_driver = std::nullopt;
        }

    }

    void Initialize(bool use_event_handlers) {
        InitializeChargerDriver(use_event_handlers);
        InitializeBatteryDriver(use_event_handlers);
    }

    void Finalize() {
        FinalizeBatteryDriver();
        FinalizeChargerDriver();
    }

}