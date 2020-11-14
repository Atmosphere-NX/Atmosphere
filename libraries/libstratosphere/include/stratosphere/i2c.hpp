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
#include <stratosphere/i2c/i2c_types.hpp>
#include <stratosphere/i2c/i2c_select_device_name.hpp>
#include <stratosphere/i2c/i2c_command_list_formatter.hpp>
#include <stratosphere/i2c/sf/i2c_sf_i_session.hpp>
#include <stratosphere/i2c/sf/i2c_sf_i_manager.hpp>
#include <stratosphere/i2c/server/i2c_server_api.hpp>
#include <stratosphere/i2c/driver/i2c_select_driver_api.hpp>
#include <stratosphere/i2c/driver/i2c_driver_service_api.hpp>
#include <stratosphere/i2c/driver/i2c_driver_client_api.hpp>
#include <stratosphere/i2c/driver/i2c_bus_api.hpp>
#include <stratosphere/i2c/driver/impl/i2c_i2c_session_impl.hpp>
#include <stratosphere/i2c/i2c_api.hpp>
#include <stratosphere/i2c/i2c_bus_api.hpp>
#include <stratosphere/i2c/i2c_register_accessor.hpp>
