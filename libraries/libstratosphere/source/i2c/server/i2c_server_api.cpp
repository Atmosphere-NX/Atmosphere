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
#include "i2c_server_manager_impl.hpp"

namespace ams::i2c::server {

    namespace {

        ManagerImpl g_manager_impl;
        ManagerImpl g_pcv_manager_impl;

        std::shared_ptr<i2c::sf::IManager> GetManagerServiceObject() {
            static std::shared_ptr<i2c::sf::IManager> s_sp = ams::sf::GetSharedPointerTo<i2c::sf::IManager>(g_manager_impl);
            return s_sp;
        }

        std::shared_ptr<i2c::sf::IManager> GetManagerServiceObjectPowerBus() {
            static std::shared_ptr<i2c::sf::IManager> s_sp = ams::sf::GetSharedPointerTo<i2c::sf::IManager>(g_pcv_manager_impl);
            return s_sp;
        }

    }

    std::shared_ptr<i2c::sf::IManager> GetServiceObject() {
        return GetManagerServiceObject();
    }

    std::shared_ptr<i2c::sf::IManager> GetServiceObjectPowerBus() {
        return GetManagerServiceObjectPowerBus();
    }

}
