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
#include "bpc_mitm_service.hpp"
#include "bpc_ams_power_utils.hpp"

namespace ams::mitm::bpc {

    Result BpcMitmService::RebootSystem() {
        R_UNLESS(bpc::IsRebootManaged(), sm::mitm::ResultShouldForwardToSession());
        bpc::RebootSystem();
        return ResultSuccess();
    }

    Result BpcMitmService::ShutdownSystem() {
        bpc::ShutdownSystem();
        return ResultSuccess();
    }

}
