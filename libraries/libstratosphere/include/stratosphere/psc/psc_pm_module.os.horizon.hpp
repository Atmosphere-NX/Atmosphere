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
#include <stratosphere/psc/sf/psc_sf_i_pm_module.hpp>

namespace ams::psc {

    class PmModule {
        NON_COPYABLE(PmModule);
        NON_MOVEABLE(PmModule);
        private:
            std::shared_ptr<psc::sf::IPmModule> intf;
            os::SystemEvent system_event;
            bool initialized;
            PmModuleId module_id;
            uintptr_t reserved;
        public:
            PmModule();
            ~PmModule();

            Result Initialize(const PmModuleId mid, const PmModuleId *dependencies, u32 dependency_count, os::EventClearMode clear_mode);
            Result Finalize();

            constexpr PmModuleId GetId() const { return this->module_id; }

            Result GetRequest(PmState *out_state, PmFlagSet *out_flags);
            Result Acknowledge(PmState state, Result res);

            os::SystemEvent *GetEventPointer();
    };

}