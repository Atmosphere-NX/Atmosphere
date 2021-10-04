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
#include "../../htclow/htclow_manager.hpp"
#include "htc_htcmisc_impl.hpp"
#include "htc_observer.hpp"

namespace ams::htc::server {

    class HtcServiceObject {
        private:
            static constexpr inline auto MaxSetElements = 0x48;
            using Set = util::FixedSet<u32>;
        private:
            u8 m_set_memory[Set::GetRequiredMemorySize(MaxSetElements)];
            Set m_set;
            HtcmiscImpl m_misc_impl;
            Observer m_observer;
            os::SdkMutex m_mutex;
        public:
            HtcServiceObject(htclow::HtclowManager *htclow_manager);
        public:
            HtcmiscImpl *GetHtcmiscImpl();
        public:
            Result GetEnvironmentVariable(sf::Out<s32> out_size, const sf::OutBuffer &out, const sf::InBuffer &name);
            Result GetEnvironmentVariableLength(sf::Out<s32> out_size, const sf::InBuffer &name);
            Result GetHostConnectionEvent(sf::OutCopyHandle out);
            Result GetHostDisconnectionEvent(sf::OutCopyHandle out);
            Result GetHostConnectionEventForSystem(sf::OutCopyHandle out);
            Result GetHostDisconnectionEventForSystem(sf::OutCopyHandle out);
            Result GetBridgeIpAddress(const sf::OutBuffer &out);
            Result GetBridgePort(const sf::OutBuffer &out);
            Result SetCradleAttached(bool attached);
            Result GetBridgeSubnetMask(const sf::OutBuffer &out);
            Result GetBridgeMacAddress(const sf::OutBuffer &out);
            Result GetWorkingDirectoryPath(const sf::OutBuffer &out, s32 max_len);
            Result GetWorkingDirectoryPathSize(sf::Out<s32> out_size);
            Result RunOnHostStart(sf::Out<u32> out_id, sf::OutCopyHandle out, const sf::InBuffer &args);
            Result RunOnHostResults(sf::Out<s32> out_result, u32 id);
            Result SetBridgeIpAddress(const sf::InBuffer &arg);
            Result SetBridgeSubnetMask(const sf::InBuffer &arg);
            Result SetBridgePort(const sf::InBuffer &arg);

    };
    static_assert(tma::IsIHtcManager<HtcServiceObject>);

}
