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
#include "htc_tenv_service.hpp"

namespace ams::htc::tenv {

    Result Service::GetVariable(sf::Out<s64> out_size, const sf::OutBuffer &out_buffer, const htc::tenv::VariableName &name) {
        /* TODO */
        AMS_UNUSED(out_size, out_buffer, name);
        AMS_ABORT("Service::GetVariable");
    }

    Result Service::GetVariableLength(sf::Out<s64> out_size, const htc::tenv::VariableName &name) {
        /* TODO */
        AMS_UNUSED(out_size, name);
        AMS_ABORT("Service::GetVariableLength");
    }

    Result Service::WaitUntilVariableAvailable(s64 timeout_ms) {
        /* TODO */
        AMS_UNUSED(timeout_ms);
        AMS_ABORT("Service::WaitUntilVariableAvailable");
    }

}
