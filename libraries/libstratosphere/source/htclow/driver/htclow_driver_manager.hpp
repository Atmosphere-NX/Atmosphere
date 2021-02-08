/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include "htclow_i_driver.hpp"

namespace ams::htclow::driver {

    class DriverManager {
        private:
            std::optional<htclow::impl::DriverType> m_driver_type{};
            /* TODO: SocketDriver m_socket_driver; */
            /* TODO: UsbDriver m_usb_driver; */
            /* TODO: PlainChannelDriver m_plain_channel_driver; */
            os::SdkMutex m_mutex{};
            IDriver *m_open_driver{};
        public:
            DriverManager() = default;
    };

}
