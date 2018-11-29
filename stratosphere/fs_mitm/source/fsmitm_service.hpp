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
        bool should_override_contents;
    public:
        FsMitmService(std::shared_ptr<Service> s, u64 pid) : IMitmServiceObject(s, pid) {
            if (Utils::HasSdDisableMitMFlag(this->title_id)) {
                this->should_override_contents = false;
            } else {
                this->should_override_contents = (this->title_id >= 0x0100000000010000ULL || Utils::HasSdMitMFlag(this->title_id)) && Utils::HasOverrideButton(this->title_id);
            }
        }
        
        static bool ShouldMitm(u64 pid, u64 tid) {
            /* Don't Mitm KIPs */
            if (pid < 0x50) {
                return false;
            }
            
            static std::atomic_bool has_launched_qlaunch = false;

            /* TODO: intercepting everything seems to cause issues with sleep mode, for some reason. */
            /* Figure out why, and address it. */
            if (tid == 0x0100000000001000ULL) {
                has_launched_qlaunch = true;
            }
            
            return has_launched_qlaunch || tid == 0x010000000000001FULL || tid >= 0x0100000000010000ULL || Utils::HasSdMitMFlag(tid);
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
