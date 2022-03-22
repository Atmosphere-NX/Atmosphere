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
#include "lm_log_service_impl.hpp"

namespace ams::lm::srv {

    class LoggerImpl {
        private:
            LogServiceImpl *m_parent;
            u64 m_process_id;
        public:
            explicit LoggerImpl(LogServiceImpl *parent, os::ProcessId process_id);
            ~LoggerImpl();
        public:
            Result Log(const sf::InAutoSelectBuffer &message);
            Result SetDestination(u32 destination);
    };
    static_assert(lm::IsILogger<LoggerImpl>);

}
