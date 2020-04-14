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

namespace ams::psc {

    class RemotePmModule final : public psc::sf::IPmModule {
        NON_COPYABLE(RemotePmModule);
        NON_MOVEABLE(RemotePmModule);
        private:
            ::PscPmModule module;
        public:
            constexpr RemotePmModule(const ::PscPmModule &m) : module(m) { /* ... */ }
            virtual ~RemotePmModule() override {
                ::pscPmModuleClose(std::addressof(this->module));
            }

            virtual Result Initialize(ams::sf::OutCopyHandle out, psc::PmModuleId module_id, const ams::sf::InBuffer &child_list) override final {
                /* NOTE: This functionality is already implemented by the libnx command we use to instantiate the PscPmModule. */
                AMS_ABORT();
            }

            virtual Result GetRequest(ams::sf::Out<PmState> out_state, ams::sf::Out<PmFlagSet> out_flags) override final {
                static_assert(sizeof(PmState) == sizeof(::PscPmState));
                static_assert(sizeof(PmFlagSet) == sizeof(u32));
                return ::pscPmModuleGetRequest(std::addressof(this->module), reinterpret_cast<::PscPmState *>(out_state.GetPointer()), reinterpret_cast<u32 *>(out_flags.GetPointer()));
            }

            virtual Result Acknowledge() override final {
                /* NOTE: libnx does not separate acknowledge/acknowledgeEx. */
                return ::pscPmModuleAcknowledge(std::addressof(this->module), static_cast<::PscPmState>(0));
            }

            virtual Result Finalize() override final {
                return ::pscPmModuleFinalize(std::addressof(this->module));
            }

            virtual Result AcknowledgeEx(PmState state) override final {
                static_assert(sizeof(state) == sizeof(::PscPmState));
                return ::pscPmModuleAcknowledge(std::addressof(this->module), static_cast<::PscPmState>(state));
            }
    };

}