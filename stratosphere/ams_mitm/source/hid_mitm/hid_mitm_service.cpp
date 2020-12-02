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
#include "hid_mitm_service.hpp"
#include "hid_shim.h"

namespace ams::mitm::hid {

    Result HidMitmService::SetSupportedNpadStyleSet(const sf::ClientAppletResourceUserId &client_aruid, u32 style_set) {
        /* This code applies only to hbl, guaranteed by the check in ShouldMitm. */
        style_set |= HidNpadStyleTag_NpadSystem | HidNpadStyleTag_NpadSystemExt;
        return hidSetSupportedNpadStyleSetFwd(this->forward_service.get(), static_cast<u64>(this->client_info.process_id), static_cast<u64>(client_aruid.GetValue()), style_set);
    }

}
