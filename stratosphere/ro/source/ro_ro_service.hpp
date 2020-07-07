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

    class RoService {
        private:
            size_t context_id;
            ModuleType type;
        protected:
            explicit RoService(ModuleType t);
        public:
            virtual ~RoService();
        public:
            /* Actual commands. */
            Result MapManualLoadModuleMemory(sf::Out<u64> out_load_address, const sf::ClientProcessId &client_pid, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size);
            Result UnmapManualLoadModuleMemory(const sf::ClientProcessId &client_pid, u64 nro_address);
            Result RegisterModuleInfo(const sf::ClientProcessId &client_pid, u64 nrr_address, u64 nrr_size);
            Result UnregisterModuleInfo(const sf::ClientProcessId &client_pid, u64 nrr_address);
            Result RegisterProcessHandle(const sf::ClientProcessId &client_pid, sf::CopyHandle process_h);
            Result RegisterModuleInfoEx(const sf::ClientProcessId &client_pid, u64 nrr_address, u64 nrr_size, sf::CopyHandle process_h);
    };
    static_assert(ro::impl::IsIRoInterface<RoService>);

    class RoServiceForSelf final : public RoService {
        public:
            RoServiceForSelf() : RoService(ro::ModuleType::ForSelf) { /* ... */ }
    };

    /* TODO: This is really JitPlugin... */
    class RoServiceForOthers final : public RoService {
        public:
            RoServiceForOthers() : RoService(ro::ModuleType::ForOthers) { /* ... */ }
    };

}
