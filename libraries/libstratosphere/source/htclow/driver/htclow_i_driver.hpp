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

namespace ams::htclow::driver {

    class IDriver {
        public:
            virtual Result Open()                              = 0;
            virtual void Close()                               = 0;
            virtual Result Connect(os::EventType *event)       = 0;
            virtual void Shutdown()                            = 0;
            virtual Result Send(const void *src, int src_size) = 0;
            virtual Result Receive(void *dst, int dst_size)    = 0;
            virtual void CancelSendReceive()                   = 0;
            virtual void Suspend()                             = 0;
            virtual void Resume()                              = 0;
    };

}
