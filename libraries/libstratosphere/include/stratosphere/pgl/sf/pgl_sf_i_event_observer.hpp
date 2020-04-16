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
#include <stratosphere/os.hpp>
#include <stratosphere/pm.hpp>
#include <stratosphere/pgl/pgl_types.hpp>

namespace ams::pgl::sf {

    class IEventObserver : public ams::sf::IServiceObject {
        protected:
            enum class CommandId {
                GetProcessEventHandle = 0,
                GetProcessEventInfo   = 1,
            };
        public:
            /* Actual commands. */
            virtual Result GetProcessEventHandle(ams::sf::OutCopyHandle out) = 0;
            virtual Result GetProcessEventInfo(ams::sf::Out<pm::ProcessEventInfo> out) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetProcessEventHandle),
                MAKE_SERVICE_COMMAND_META(GetProcessEventInfo),
            };
    };

}