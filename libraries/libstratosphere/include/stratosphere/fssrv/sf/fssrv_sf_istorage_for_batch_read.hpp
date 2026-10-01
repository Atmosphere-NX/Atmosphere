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
#include <stratosphere/fssrv/sf/fssrv_sf_istorage.hpp>

#define AMS_FSSRV_I_STORAGE_FOR_BATCH_READ_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 10, Result, BatchRead, (const ams::sf::OutNonSecureBuffer &buffer0, const ams::sf::OutNonSecureBuffer &buffer1, const ams::sf::OutNonSecureBuffer &buffer2, const ams::sf::OutNonSecureBuffer &buffer3, const ams::sf::OutNonSecureBuffer &buffer4, const ams::sf::OutNonSecureBuffer &buffer5, const ams::sf::OutNonSecureBuffer &buffer6, const ams::sf::InArray<s64> &offsets), (buffer0, buffer1, buffer2, buffer3, buffer4, buffer5, buffer6, offsets), hos::Version_23_0_0)

AMS_SF_DEFINE_INTERFACE_WITH_BASE(ams::fssrv::sf, IStorageForBatchRead, ::ams::fssrv::sf::IStorage, AMS_FSSRV_I_STORAGE_FOR_BATCH_READ_INTERFACE_INFO, 0xCD2D5606)
