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

#define AMS_LM_I_LOGGER_INTERFACE_INFO(C, H)                                                                    \
    AMS_SF_METHOD_INFO(C, H, 0, Result, Log,            (const sf::InAutoSelectBuffer &message), (message))     \
    AMS_SF_METHOD_INFO(C, H, 1, Result, SetDestination, (u32 destination),                       (destination))

AMS_SF_DEFINE_INTERFACE(ams::lm, ILogger, AMS_LM_I_LOGGER_INTERFACE_INFO)

#define AMS_LM_I_LOG_SERVICE_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 0, Result, OpenLogger, (sf::Out<sf::SharedPointer<::ams::lm::ILogger>> out, const sf::ClientProcessId &client_process_id), (out, client_process_id))

AMS_SF_DEFINE_INTERFACE(ams::lm, ILogService, AMS_LM_I_LOG_SERVICE_INTERFACE_INFO)
