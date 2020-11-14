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
#include "psc_remote_pm_module.hpp"

namespace ams::psc {

    PmModule::PmModule() : intf(nullptr), initialized(false), reserved(0) { /* ... */ }

    PmModule::~PmModule() {
        if (this->initialized) {
            this->intf = nullptr;
            os::DestroySystemEvent(this->system_event.GetBase());
        }
    }

    Result PmModule::Initialize(const PmModuleId mid, const PmModuleId *dependencies, u32 dependency_count, os::EventClearMode clear_mode) {
        R_UNLESS(!this->initialized, psc::ResultAlreadyInitialized());

        static_assert(sizeof(*dependencies) == sizeof(u16));
        ::PscPmModule module;
        R_TRY(::pscmGetPmModule(std::addressof(module), static_cast<::PscPmModuleId>(mid), reinterpret_cast<const u16 *>(dependencies), dependency_count, clear_mode == os::EventClearMode_AutoClear));

        this->intf = ams::sf::MakeShared<psc::sf::IPmModule, RemotePmModule>(module);
        this->system_event.AttachReadableHandle(module.event.revent, false, clear_mode);
        this->initialized = true;
        return ResultSuccess();
    }

    Result PmModule::Finalize() {
        R_UNLESS(this->initialized, psc::ResultNotInitialized());

        R_TRY(this->intf->Finalize());
        this->intf = nullptr;
        os::DestroySystemEvent(this->system_event.GetBase());
        this->initialized = false;
        return ResultSuccess();
    }

    Result PmModule::GetRequest(PmState *out_state, PmFlagSet *out_flags) {
        R_UNLESS(this->initialized, psc::ResultNotInitialized());

        return this->intf->GetRequest(out_state, out_flags);
    }

    Result PmModule::Acknowledge(PmState state, Result res) {
        R_ABORT_UNLESS(res);
        R_UNLESS(this->initialized, psc::ResultNotInitialized());

        if (hos::GetVersion() >= hos::Version_5_1_0) {
            return this->intf->AcknowledgeEx(state);
        } else {
            return this->intf->Acknowledge();
        }
    }

    os::SystemEvent *PmModule::GetEventPointer() {
        return std::addressof(this->system_event);
    }

}