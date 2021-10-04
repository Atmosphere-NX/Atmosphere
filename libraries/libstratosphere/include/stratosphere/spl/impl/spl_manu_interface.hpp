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
#include <stratosphere/sf.hpp>
#include <stratosphere/spl/spl_types.hpp>
#include <stratosphere/spl/impl/spl_device_unique_data_interface.hpp>

#define AMS_SPL_I_MANU_INTERFACE_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 30, Result, ReencryptDeviceUniqueData, (const sf::OutPointerBuffer &out, const sf::InPointerBuffer &src, spl::AccessKey access_key_dec, spl::KeySource source_dec, spl::AccessKey access_key_enc, spl::KeySource source_enc, u32 option), (out, src, access_key_dec, source_dec, access_key_enc, source_enc, option))

AMS_SF_DEFINE_INTERFACE_WITH_BASE(ams::spl::impl, IManuInterface, ::ams::spl::impl::IDeviceUniqueDataInterface, AMS_SPL_I_MANU_INTERFACE_INTERFACE_INFO)
