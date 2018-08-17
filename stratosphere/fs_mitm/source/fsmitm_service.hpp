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
#include <stratosphere/iserviceobject.hpp>
#include "imitmserviceobject.hpp"
#include "fs_istorage.hpp"
#include "fsmitm_utils.hpp"

enum class FspSrvCmd {
    SetCurrentProcess = 1,
    OpenDataStorageByCurrentProcess = 200,
    OpenDataStorageByDataId = 202,
};

class FsMitMService : public IMitMServiceObject {      
    private:
        bool has_initialized = false;
        u64 init_pid = 0;
        std::shared_ptr<IStorageInterface> romfs_storage;
    public:
        FsMitMService(Service *s) : IMitMServiceObject(s) {
            /* ... */
        }
        
        static bool should_mitm(u64 pid, u64 tid) {
            if(!(tid >= 0x0100000000010000ULL || Utils::HasSdMitMFlag(tid))) return false;
            
            FsDir tst;
            char slash = '/';
            bool ret = R_SUCCEEDED(Utils::OpenSdDirForAtmosphere(tid, &slash, &tst));
            if(!ret) return false;
            fsDirClose(&tst);
            return true;
        }
        
        FsMitMService *clone() override {
            auto new_srv = new FsMitMService((Service *)&this->forward_service);
            this->clone_to(new_srv);
            return new_srv;
        }
        
        void clone_to(void *o) override {
            FsMitMService *other = (FsMitMService *)o;
            other->has_initialized = has_initialized;
            other->init_pid = init_pid;
        }
        
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual void postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual Result handle_deferred();
    
    protected:
        /* Overridden commands. */
        std::tuple<Result, OutSession<IStorageInterface>> open_data_storage_by_current_process();
        std::tuple<Result, OutSession<IStorageInterface>> open_data_storage_by_data_id(u64 storage_id, u64 data_id);
};
