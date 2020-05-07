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
#include <stratosphere/psc/psc_types.hpp>
#include <stratosphere/psc/psc_pm_module_id.hpp>

namespace ams::psc::sf {

    class IPmModule : public ams::sf::IServiceObject {
        protected:
            enum class CommandId {
                Initialize    = 0,
                GetRequest    = 1,
                Acknowledge   = 2,
                Finalize      = 3,
                AcknowledgeEx = 4,
            };
        public:
            /* Actual commands. */
            virtual Result Initialize(ams::sf::OutCopyHandle out, psc::PmModuleId module_id, const ams::sf::InBuffer &child_list) = 0;
            virtual Result GetRequest(ams::sf::Out<PmState> out_state, ams::sf::Out<PmFlagSet> out_flags) = 0;
            virtual Result Acknowledge() = 0;
            virtual Result Finalize() = 0;
            virtual Result AcknowledgeEx(PmState state) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(Initialize),
                MAKE_SERVICE_COMMAND_META(GetRequest),
                MAKE_SERVICE_COMMAND_META(Acknowledge),
                MAKE_SERVICE_COMMAND_META(Finalize),
                MAKE_SERVICE_COMMAND_META(AcknowledgeEx, hos::Version_5_1_0),
            };
    };

}