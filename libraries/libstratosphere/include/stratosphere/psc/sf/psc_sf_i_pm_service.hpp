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
#include <stratosphere/psc/sf/psc_sf_i_pm_module.hpp>

namespace ams::psc::sf {

    #define AMS_PSC_I_PM_SERVICE_INTERFACE_INFO(C, H)                                                            \
        AMS_SF_METHOD_INFO(C, H, 0, Result, Initialize, (ams::sf::Out<std::shared_ptr<psc::sf::IPmModule>> out))

    AMS_SF_DEFINE_INTERFACE(IPmService, AMS_PSC_I_PM_SERVICE_INTERFACE_INFO)

}