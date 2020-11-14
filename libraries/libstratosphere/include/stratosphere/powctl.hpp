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

#pragma once
#include <stratosphere/powctl/powctl_types.hpp>
#include <stratosphere/powctl/powctl_select_devices.hpp>
#include <stratosphere/powctl/powctl_session_api.hpp>
#include <stratosphere/powctl/powctl_battery_api.hpp>
#include <stratosphere/powctl/powctl_charger_api.hpp>
#include <stratosphere/powctl/impl/powctl_battery_charge_percentage.hpp>
#include <stratosphere/powctl/driver/powctl_driver_api.hpp>
#include <stratosphere/powctl/driver/impl/powctl_select_charger_parameters.hpp>
#include <stratosphere/powctl/driver/impl/powctl_charge_arbiter.hpp>
