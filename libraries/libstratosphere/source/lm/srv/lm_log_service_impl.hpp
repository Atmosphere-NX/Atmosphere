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
#include "../sf/lm_i_log_service.hpp"

namespace ams::lm::srv {

    class LogServiceImpl {
        public:
            Result OpenLogger(sf::Out<sf::SharedPointer<::ams::lm::ILogger>> out, const sf::ClientProcessId &client_process_id);
    };
    static_assert(lm::IsILogService<LogServiceImpl>);

}
