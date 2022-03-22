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

namespace ams::htc::tenv {

    class Service {
        private:
            os::ProcessId m_process_id;
        public:
            constexpr Service(os::ProcessId pid) : m_process_id(pid) { /* ... */ }
        public:
            Result GetVariable(sf::Out<s64> out_size, const sf::OutBuffer &out_buffer, const htc::tenv::VariableName &name);
            Result GetVariableLength(sf::Out<s64> out_size,const htc::tenv::VariableName &name);
            Result WaitUntilVariableAvailable(s64 timeout_ms);
    };
    static_assert(htc::tenv::IsIService<Service>);

}
