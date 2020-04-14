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

namespace ams::ro {

    /* Access utilities. */
    void SetDevelopmentHardware(bool is_development_hardware);
    void SetDevelopmentFunctionEnabled(bool is_development_function_enabled);

    class Service final : public sf::IServiceObject {
        protected:
            enum class CommandId {
                LoadNro     = 0,
                UnloadNro   = 1,
                LoadNrr     = 2,
                UnloadNrr   = 3,
                Initialize  = 4,
                LoadNrrEx   = 10,
            };
        private:
            size_t context_id;
            ModuleType type;
        public:
            explicit Service(ModuleType t);
            virtual ~Service();
        private:
            /* Actual commands. */
            Result LoadNro(sf::Out<u64> out_load_address, const sf::ClientProcessId &client_pid, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size);
            Result UnloadNro(const sf::ClientProcessId &client_pid, u64 nro_address);
            Result LoadNrr(const sf::ClientProcessId &client_pid, u64 nrr_address, u64 nrr_size);
            Result UnloadNrr(const sf::ClientProcessId &client_pid, u64 nrr_address);
            Result Initialize(const sf::ClientProcessId &client_pid, sf::CopyHandle process_h);
            Result LoadNrrEx(const sf::ClientProcessId &client_pid, u64 nrr_address, u64 nrr_size, sf::CopyHandle process_h);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(LoadNro),
                MAKE_SERVICE_COMMAND_META(UnloadNro),
                MAKE_SERVICE_COMMAND_META(LoadNrr),
                MAKE_SERVICE_COMMAND_META(UnloadNrr),
                MAKE_SERVICE_COMMAND_META(Initialize),
                MAKE_SERVICE_COMMAND_META(LoadNrrEx,   hos::Version_7_0_0),
            };

    };

}
