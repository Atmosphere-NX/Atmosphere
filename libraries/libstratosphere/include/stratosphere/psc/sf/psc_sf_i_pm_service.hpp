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
#include <vapours.hpp>
#include <stratosphere/psc/sf/psc_sf_i_pm_module.hpp>

#define AMS_PSC_I_PM_SERVICE_INTERFACE_INFO(C, H)                                                                  \
    AMS_SF_METHOD_INFO(C, H, 0, Result, Initialize, (ams::sf::Out<ams::sf::SharedPointer<psc::sf::IPmModule>> out), (out))

AMS_SF_DEFINE_INTERFACE(ams::psc::sf, IPmService, AMS_PSC_I_PM_SERVICE_INTERFACE_INFO)
