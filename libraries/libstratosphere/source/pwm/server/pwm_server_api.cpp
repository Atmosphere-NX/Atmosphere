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
#include <stratosphere.hpp>
#include "pwm_server_manager_impl.hpp"

namespace ams::pwm::server {

    namespace {

        ams::sf::UnmanagedServiceObject<pwm::sf::IManager, pwm::server::ManagerImpl> g_manager_impl;

    }

    ams::sf::SharedPointer<pwm::sf::IManager> GetServiceObject() {
        return g_manager_impl.GetShared();
    }

}
