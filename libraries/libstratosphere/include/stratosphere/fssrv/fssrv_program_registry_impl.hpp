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
#include <vapours.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_i_program_registry.hpp>

namespace ams::fssrv {

    class ProgramRegistryServiceImpl;

    namespace impl {

        class ProgramInfo;

    }

    /* ACCURATE_TO_VERSION: Unknown */
    class ProgramRegistryImpl {
        NON_COPYABLE(ProgramRegistryImpl);
        NON_MOVEABLE(ProgramRegistryImpl);
        private:
            u64 m_process_id;
        public:
            ProgramRegistryImpl();
            ~ProgramRegistryImpl();
        public:
            static void Initialize(ProgramRegistryServiceImpl *service);
        public:
            Result RegisterProgram(u64 process_id, u64 program_id, u8 storage_id, const ams::sf::InBuffer &data, s64 data_size, const ams::sf::InBuffer &desc, s64 desc_size);
            Result UnregisterProgram(u64 process_id);
            Result SetCurrentProcess(const ams::sf::ClientProcessId &client_pid);
            Result SetEnabledProgramVerification(bool en);
    };
    static_assert(sf::IsIProgramRegistry<ProgramRegistryImpl>);

    /* ACCURATE_TO_VERSION: Unknown */
    class InvalidProgramRegistryImpl {
        public:
            Result RegisterProgram(u64 process_id, u64 program_id, u8 storage_id, const ams::sf::InBuffer &data, s64 data_size, const ams::sf::InBuffer &desc, s64 desc_size) {
                AMS_UNUSED(process_id, program_id, storage_id, data, data_size, desc, desc_size);
                R_THROW(fs::ResultPortAcceptableCountLimited());
            }

            Result UnregisterProgram(u64 process_id) {
                AMS_UNUSED(process_id);
                R_THROW(fs::ResultPortAcceptableCountLimited());
            }

            Result SetCurrentProcess(const ams::sf::ClientProcessId &client_pid) {
                AMS_UNUSED(client_pid);
                R_THROW(fs::ResultPortAcceptableCountLimited());
            }

            Result SetEnabledProgramVerification(bool en) {
                AMS_UNUSED(en);
                R_THROW(fs::ResultPortAcceptableCountLimited());
            }
    };
    static_assert(sf::IsIProgramRegistry<InvalidProgramRegistryImpl>);

}
