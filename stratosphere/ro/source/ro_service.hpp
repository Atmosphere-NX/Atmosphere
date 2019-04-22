/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>

#include <stratosphere.hpp>

#include "ro_registration.hpp"

enum RoServiceCmd {
    Ro_Cmd_LoadNro = 0,
    Ro_Cmd_UnloadNro = 1,
    Ro_Cmd_LoadNrr = 2,
    Ro_Cmd_UnloadNrr = 3,
    Ro_Cmd_Initialize = 4,
    Ro_Cmd_LoadNrrEx = 10,
};

class RelocatableObjectsService final : public IServiceObject {
    private:
        Registration::RoProcessContext *context = nullptr;
        RoModuleType type;
    public:
        explicit RelocatableObjectsService(RoModuleType t) : type(t) {
            /* ... */
        }
        virtual ~RelocatableObjectsService() override;
    private:
        bool IsInitialized() const {
            return this->context != nullptr;
        }
        bool IsProcessIdValid(u64 process_id);
        static u64 GetTitleId(Handle process_handle);
    private:
        /* Actual commands. */
        Result LoadNro(Out<u64> load_address, PidDescriptor pid_desc, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size);
        Result UnloadNro(PidDescriptor pid_desc, u64 nro_address);
        Result LoadNrr(PidDescriptor pid_desc, u64 nrr_address, u64 nrr_size);
        Result UnloadNrr(PidDescriptor pid_desc, u64 nrr_address);
        Result Initialize(PidDescriptor pid_desc, CopiedHandle process_h);
        Result LoadNrrEx(PidDescriptor pid_desc, u64 nrr_address, u64 nrr_size, CopiedHandle process_h);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<Ro_Cmd_LoadNro, &RelocatableObjectsService::LoadNro>(),
            MakeServiceCommandMeta<Ro_Cmd_UnloadNro, &RelocatableObjectsService::UnloadNro>(),
            MakeServiceCommandMeta<Ro_Cmd_LoadNrr, &RelocatableObjectsService::LoadNrr>(),
            MakeServiceCommandMeta<Ro_Cmd_UnloadNrr, &RelocatableObjectsService::UnloadNrr>(),
            MakeServiceCommandMeta<Ro_Cmd_Initialize, &RelocatableObjectsService::Initialize>(),
            MakeServiceCommandMeta<Ro_Cmd_LoadNrrEx, &RelocatableObjectsService::LoadNrrEx, FirmwareVersion_700>(),
        };
};
