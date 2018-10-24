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
#include "fsmitm_utils.hpp"

enum class SetSysCmd {
    GetFirmwareVersion = 3,
    GetFirmwareVersion2 = 4,
};

class SetSysMitMService : public IMitMServiceObject {      
    private:
    public:
        SetSysMitMService(Service *s) : IMitMServiceObject(s) {
            /* ... */
        }
        
        static bool should_mitm(u64 pid, u64 tid) {
            /* Only MitM qlaunch, maintenance. */
            return tid == 0x0100000000001000ULL || tid == 0x0100000000001015ULL;
        }
        
        SetSysMitMService *clone() override {
            auto new_srv = new SetSysMitMService((Service *)&this->forward_service);
            this->clone_to(new_srv);
            return new_srv;
        }
        
        void clone_to(void *o) override {
            /* ... */
        }
        
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual void postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual Result handle_deferred();
    
    protected:
        /* Overridden commands. */
        std::tuple<Result> get_firmware_version(OutPointerWithServerSize<SetSysFirmwareVersion, 0x1> out);
        std::tuple<Result> get_firmware_version2(OutPointerWithServerSize<SetSysFirmwareVersion, 0x1> out);
};
