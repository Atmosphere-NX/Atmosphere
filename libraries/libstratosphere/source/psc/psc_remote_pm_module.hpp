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

namespace ams::psc {

    class RemotePmModule {
        NON_COPYABLE(RemotePmModule);
        NON_MOVEABLE(RemotePmModule);
        private:
            ::PscPmModule m_module;
        public:
            constexpr RemotePmModule(const ::PscPmModule &m) : m_module(m) { /* ... */ }
            ~RemotePmModule() {
                ::pscPmModuleClose(std::addressof(m_module));
            }

            Result Initialize(ams::sf::OutCopyHandle out, psc::PmModuleId module_id, const ams::sf::InBuffer &child_list) {
                /* NOTE: This functionality is already implemented by the libnx command we use to instantiate the PscPmModule. */
                AMS_UNUSED(out, module_id, child_list);
                AMS_ABORT();
            }

            Result GetRequest(ams::sf::Out<PmState> out_state, ams::sf::Out<PmFlagSet> out_flags) {
                static_assert(sizeof(PmState) == sizeof(::PscPmState));
                static_assert(sizeof(PmFlagSet) == sizeof(u32));
                return ::pscPmModuleGetRequest(std::addressof(m_module), reinterpret_cast<::PscPmState *>(out_state.GetPointer()), reinterpret_cast<u32 *>(out_flags.GetPointer()));
            }

            Result Acknowledge() {
                /* NOTE: libnx does not separate acknowledge/acknowledgeEx. */
                return ::pscPmModuleAcknowledge(std::addressof(m_module), static_cast<::PscPmState>(0));
            }

            Result Finalize() {
                return ::pscPmModuleFinalize(std::addressof(m_module));
            }

            Result AcknowledgeEx(PmState state) {
                static_assert(sizeof(state) == sizeof(::PscPmState));
                return ::pscPmModuleAcknowledge(std::addressof(m_module), static_cast<::PscPmState>(state));
            }
    };
    static_assert(psc::sf::IsIPmModule<RemotePmModule>);

}