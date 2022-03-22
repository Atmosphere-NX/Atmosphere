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

namespace ams::htclow::ctrl {

    constexpr inline const impl::ChannelInternalType ServiceChannels[] = {
        {
            .channel_id = 0, /* TODO: htcfs::ChannelId? */
            .module_id  = ModuleId::Htcfs,
        },
        {
            .channel_id = 1, /* TODO: htcmisc::ClientChannelId? */
            .module_id  = ModuleId::Htcmisc,
        },
        {
            .channel_id = 2, /* TODO: htcmisc::ServerChannelId? */
            .module_id  = ModuleId::Htcmisc,
        },
        {
            .channel_id = 0, /* TODO: htcs::ChannelId? */
            .module_id  = ModuleId::Htcs,
        },
    };

}
