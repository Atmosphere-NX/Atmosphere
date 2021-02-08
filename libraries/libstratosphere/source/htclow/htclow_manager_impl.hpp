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
#include "htclow_packet_factory.hpp"
#include "driver/htclow_driver_manager.hpp"
#include "mux/htclow_mux.hpp"
#include "ctrl/htclow_ctrl_packet_factory.hpp"
#include "ctrl/htclow_ctrl_state_machine.hpp"
#include "ctrl/htclow_ctrl_service.hpp"
#include "htclow_worker.hpp"
#include "htclow_listener.hpp"

namespace ams::htclow {

    class HtclowManagerImpl {
        private:
            PacketFactory m_packet_factory;
            driver::DriverManager m_driver_manager;
            mux::Mux m_mux;
            ctrl::HtcctrlPacketFactory m_ctrl_packet_factory;
            ctrl::HtcctrlStateMachine m_ctrl_state_machine;
            ctrl::HtcctrlService m_ctrl_service;
            Worker m_worker;
            Listener m_listener;
            bool m_is_driver_open;
        public:
            HtclowManagerImpl(mem::StandardAllocator *allocator);
            ~HtclowManagerImpl();
        public:
            Result OpenDriver(impl::DriverType driver_type);
            void CloseDriver();

            void Disconnect();
    };

}
