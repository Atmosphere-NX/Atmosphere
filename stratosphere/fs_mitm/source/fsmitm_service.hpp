/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include "fs_istorage.hpp"
#include "fsmitm_utils.hpp"

enum FspSrvCmd : u32 {
    FspSrvCmd_SetCurrentProcess = 1,
    FspSrvCmd_OpenBisStorage = 12,
    FspSrvCmd_OpenDataStorageByCurrentProcess = 200,
    FspSrvCmd_OpenDataStorageByDataId = 202,
};

class FsMitmService : public IMitmServiceObject {
    private:
        bool has_initialized = false;
    public:
        FsMitmService(std::shared_ptr<Service> s) : IMitmServiceObject(s) {
            /* ... */
        }
        
        static bool ShouldMitm(u64 pid, u64 tid) {
            /* Always intercept NS, so that we can protect the boot partition. */
            if (tid == 0x010000000000001FULL) {
                return true;
            }
            
            if (Utils::HasSdDisableMitMFlag(tid)) {
                return false;
            }
            
            return (tid >= 0x0100000000010000ULL || Utils::HasSdMitMFlag(tid)) && Utils::HasOverrideButton(tid);
        }
        
        static void PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx);
            
    protected:
        /* Overridden commands. */
        Result OpenBisStorage(Out<std::shared_ptr<IStorageInterface>> out, u32 bis_partition_id);
        Result OpenDataStorageByCurrentProcess(Out<std::shared_ptr<IStorageInterface>> out);
        Result OpenDataStorageByDataId(Out<std::shared_ptr<IStorageInterface>> out, u64 data_id, u8 storage_id);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<FspSrvCmd_OpenBisStorage, &FsMitmService::OpenBisStorage>(),
            MakeServiceCommandMeta<FspSrvCmd_OpenDataStorageByCurrentProcess, &FsMitmService::OpenDataStorageByCurrentProcess>(),
            MakeServiceCommandMeta<FspSrvCmd_OpenDataStorageByDataId, &FsMitmService::OpenDataStorageByDataId>(),
        };
};
