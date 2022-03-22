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
#include "sprofile_srv_service_getter.hpp"

namespace ams::sprofile::srv {

    Result ServiceGetter::GetServiceForSystemProcess(sf::Out<sf::SharedPointer<sprofile::srv::ISprofileServiceForSystemProcess>> out) {
        /* Check that we have a service-for-system-process. */
        R_UNLESS(m_service_for_system_process != nullptr, sprofile::ResultNotPermitted());

        /* Set the output. */
        *out = m_service_for_system_process;
        R_SUCCEED();
    }

    Result ServiceGetter::GetServiceForBgAgent(sf::Out<sf::SharedPointer<sprofile::srv::ISprofileServiceForBgAgent>> out) {
        /* Check that we have a service-for-bg-agent. */
        R_UNLESS(m_service_for_bg_agent != nullptr, sprofile::ResultNotPermitted());

        /* Set the output. */
        *out = m_service_for_bg_agent;
        R_SUCCEED();
    }

}
