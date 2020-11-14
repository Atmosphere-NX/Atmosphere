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

#pragma once
#include <vapours.hpp>
#include <stratosphere/psc/psc_types.hpp>
#include <stratosphere/psc/psc_pm_module_id.hpp>

namespace ams::psc::sf {

    #define AMS_PSC_I_PM_MODULE_INTERFACE_INFO(C, H)                                                                                                                         \
        AMS_SF_METHOD_INFO(C, H, 0, Result, Initialize,    (ams::sf::OutCopyHandle out, psc::PmModuleId module_id, const ams::sf::InBuffer &child_list))                     \
        AMS_SF_METHOD_INFO(C, H, 1, Result, GetRequest,    (ams::sf::Out<PmState> out_state, ams::sf::Out<PmFlagSet> out_flags))                                             \
        AMS_SF_METHOD_INFO(C, H, 2, Result, Acknowledge,   ())                                                                                                               \
        AMS_SF_METHOD_INFO(C, H, 3, Result, Finalize,      ())                                                                                                               \
        AMS_SF_METHOD_INFO(C, H, 4, Result, AcknowledgeEx, (PmState state),                                                                              hos::Version_5_1_0)

    AMS_SF_DEFINE_INTERFACE(IPmModule, AMS_PSC_I_PM_MODULE_INTERFACE_INFO)

}