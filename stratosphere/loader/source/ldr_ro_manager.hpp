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

namespace ams::ldr {

    class RoManager {
        AMS_CONSTINIT_SINGLETON_TRAITS(RoManager);
        public:
            static constexpr PinId InvalidPinId = {};
            static constexpr int ProcessCount = 0x40;
            static constexpr int NsoCount     = 0x20;
        private:
            struct NsoInfo {
                bool in_use;
                ldr::ModuleInfo module_info;
            };

            struct ProcessInfo {
                bool in_use;
                PinId pin_id;
                os::ProcessId process_id;
                ncm::ProgramId program_id;
                cfg::OverrideStatus override_status;
                ncm::ProgramLocation program_location;
                NsoInfo nso_infos[NsoCount];
            };
        private:
            ProcessInfo m_processes[ProcessCount];
            u64 m_pin_id;
        public:
            bool Allocate(PinId *out, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &status);
            bool Free(PinId pin_id);

            void RegisterProcess(PinId pin_id, os::ProcessId process_id, ncm::ProgramId program_id, bool is_64_bit_address_space);

            bool GetProgramLocationAndStatus(ncm::ProgramLocation *out, cfg::OverrideStatus *out_status, PinId pin_id);

            void AddNso(PinId pin_id, const u8 *module_id, u64 address, u64 size);

            bool GetProcessModuleInfo(u32 *out_count, ModuleInfo *out, size_t max_out_count, os::ProcessId process_id);
        private:
            ProcessInfo *AllocateProcessInfo();
            ProcessInfo *FindProcessInfo(PinId pin_id);
            ProcessInfo *FindProcessInfo(os::ProcessId process_id);
            ProcessInfo *FindProcessInfo(ncm::ProgramId program_id);

            NsoInfo *AllocateNsoInfo(ProcessInfo *info);
    };

}
