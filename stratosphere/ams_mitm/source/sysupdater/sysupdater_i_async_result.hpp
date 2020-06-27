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
#include <stratosphere.hpp>

namespace ams::mitm::sysupdater {

    class IAsyncBase : public sf::IServiceObject {
        public:
            virtual Result Cancel() = 0;
            virtual Result GetErrorContext(sf::Out<err::ErrorContext> out) = 0;
    };

    class IAsyncResult : public IAsyncBase {
        private:
            enum class CommandId {
                Get             = 0,
                Cancel          = 1,
                GetErrorContext = 2,
            };
        public:
            virtual Result Get() = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(Get),
                MAKE_SERVICE_COMMAND_META(Cancel),
                MAKE_SERVICE_COMMAND_META(GetErrorContext),
            };
    };

}
