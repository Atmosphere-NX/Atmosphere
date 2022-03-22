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
#pragma once
#include <stratosphere.hpp>
#include "sprofile_srv_i_service_getter.hpp"

namespace ams::sprofile::srv {

    class ServiceGetter {
        private:
            sf::SharedPointer<::ams::sprofile::srv::ISprofileServiceForBgAgent> m_service_for_bg_agent;
            sf::SharedPointer<::ams::sprofile::srv::ISprofileServiceForSystemProcess> m_service_for_system_process;
        public:
            constexpr ServiceGetter(sf::SharedPointer<::ams::sprofile::srv::ISprofileServiceForBgAgent> bg, sf::SharedPointer<::ams::sprofile::srv::ISprofileServiceForSystemProcess> sp) : m_service_for_bg_agent(bg), m_service_for_system_process(sp) { /* ... */ }
        public:
            Result GetServiceForSystemProcess(sf::Out<sf::SharedPointer<sprofile::srv::ISprofileServiceForSystemProcess>> out);
            Result GetServiceForBgAgent(sf::Out<sf::SharedPointer<sprofile::srv::ISprofileServiceForBgAgent>> out);
    };
    static_assert(sprofile::srv::IsIServiceGetter<ServiceGetter>);

}
