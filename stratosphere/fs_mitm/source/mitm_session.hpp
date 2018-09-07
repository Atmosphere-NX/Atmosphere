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
#include "imitmserviceobject.hpp"

#include "mitm_query_service.hpp"
#include "mitm_server.hpp"
#include "fsmitm_worker.hpp"

#include "debug.hpp"

template <typename T>
class MitMServer;

template <typename T>
class MitMSession final : public ISession<T> {
    static_assert(std::is_base_of<IMitMServiceObject, T>::value, "MitM Service Objects must derive from IMitMServiceObject");
    
    /* This will be for the actual session. */
    Service forward_service;
    IpcParsedCommand cur_out_r;
    u32 mitm_domain_id = 0;
    bool got_first_message;
        
    public:
        MitMSession<T>(MitMServer<T> *s, Handle s_h, Handle c_h, const char *srv) : ISession<T>(s, s_h, c_h, NULL, 0), got_first_message(false) {
            this->server = s;
            this->server_handle = s_h;
            this->client_handle = c_h;
            if (R_FAILED(smMitMGetService(&forward_service, srv))) {
                /* TODO: Panic. */
            }
            size_t pointer_buffer_size = 0;
            if (R_FAILED(ipcQueryPointerBufferSize(forward_service.handle, &pointer_buffer_size))) {
                /* TODO: Panic. */
            }
            this->service_object = std::make_shared<T>(&forward_service);
            this->pointer_buffer.resize(pointer_buffer_size);
        }
        MitMSession<T>(MitMServer<T> *s, Handle s_h, Handle c_h, Handle f_h) : ISession<T>(s, s_h, c_h, NULL, 0), got_first_message(true) {
            this->server = s;
            this->server_handle = s_h;
            this->client_handle = c_h;
            serviceCreate(&this->forward_service, f_h);
            size_t pointer_buffer_size = 0;
            if (R_FAILED(ipcQueryPointerBufferSize(forward_service.handle, &pointer_buffer_size))) {
                /* TODO: Panic. */
            }
            this->service_object = std::make_shared<T>(&forward_service);
            this->pointer_buffer.resize(pointer_buffer_size);
        }
        
        virtual ~MitMSession() {
            serviceClose(&forward_service);
        }
        
        Result handle_message(IpcParsedCommand &r) override {
            IpcCommand c;
            ipcInitialize(&c);
            u64 cmd_id = ((u32 *)r.Raw)[2];
            Result retval = 0xF601;
            
            cur_out_r.NumHandles = 0;
            
            Log(armGetTls(), 0x100);
                        
            u32 *cmdbuf = (u32 *)armGetTls();
            if (r.CommandType == IpcCommandType_Close) {
                Reboot();
            }
                        
            if (r.CommandType == IpcCommandType_Request || r.CommandType == IpcCommandType_RequestWithContext) {
                std::shared_ptr<IServiceObject> obj;
                if (r.IsDomainMessage) {
                    obj = this->domain->get_domain_object(r.ThisObjectId);
                    if (obj != nullptr && r.MessageType == DomainMessageType_Close) {
                        if (r.ThisObjectId == this->mitm_domain_id) {
                            Reboot();
                        }
                        this->domain->delete_object(r.ThisObjectId);
                        struct {
                            u64 magic;
                            u64 result;
                        } *o_resp;

                        o_resp = (decltype(o_resp)) ipcPrepareHeaderForDomain(&c, sizeof(*o_resp), 0);
                        *(DomainMessageHeader *)((uintptr_t)o_resp - sizeof(DomainMessageHeader)) = {0};
                        o_resp->magic = SFCO_MAGIC;
                        o_resp->result = 0x0;
                        Log(armGetTls(), 0x100);
                        return o_resp->result;
                    }
                } else {
                    obj = this->service_object;
                }
                if (obj != nullptr) {
                    retval = obj->dispatch(r, c, cmd_id, (u8 *)this->pointer_buffer.data(), this->pointer_buffer.size());
                    if (R_SUCCEEDED(retval)) {
                        if (r.IsDomainMessage) { 
                            ipcParseForDomain(&cur_out_r);
                        } else {
                            ipcParse(&cur_out_r);
                        }
                        return retval;
                    }
                }
            } else if (r.CommandType == IpcCommandType_Control || r.CommandType == IpcCommandType_ControlWithContext) {
                /* Ipc Clone Current Object. */
                retval = serviceIpcDispatch(&forward_service);
                Log(armGetTls(), 0x100);
                if (R_SUCCEEDED(retval)) {
                    ipcParse(&cur_out_r);
                    struct {
                        u64 magic;
                        u64 result;
                    } *resp = (decltype(resp))cur_out_r.Raw;
                    retval = resp->result;
                    if ((cmd_id == IpcCtrl_Cmd_CloneCurrentObject || cmd_id == IpcCtrl_Cmd_CloneCurrentObjectEx)) {
                        if (R_SUCCEEDED(retval)) {
                            Handle s_h;
                            Handle c_h;
                            Result rc;
                            if (R_FAILED((rc = svcCreateSession(&s_h, &c_h, 0, 0)))) {
                                fatalSimple(rc);
                            }
                            
                            if (cur_out_r.NumHandles != 1) {
                                Reboot();
                            }
                            
                            MitMSession<T> *new_sess = new MitMSession<T>((MitMServer<T> *)this->server, s_h, c_h, cur_out_r.Handles[0]);
                            new_sess->service_object = this->service_object;

                            if (this->is_domain) {
                                new_sess->is_domain = true;
                                new_sess->domain = this->domain;
                                new_sess->mitm_domain_id = this->mitm_domain_id;
                                new_sess->forward_service.type = this->forward_service.type;
                                new_sess->forward_service.object_id = this->forward_service.object_id;
                            }
                            this->get_manager()->add_waitable(new_sess);
                            ipcSendHandleMove(&c, c_h);
                            struct {
                                u64 magic;
                                u64 result;
                            } *o_resp;

                            o_resp = (decltype(o_resp)) ipcPrepareHeader(&c, sizeof(*o_resp));
                            o_resp->magic = SFCO_MAGIC;
                            o_resp->result = 0x0;
                        }
                    }
                }
                Log(armGetTls(), 0x100);
                return retval;
            }
            
            /* 0xF601 --> Dispatch onwards. */
            if (retval == 0xF601) {
                /* Patch PID Descriptor, if relevant. */
                if (r.HasPid) {
                    /* [ctrl 0] [ctrl 1] [handle desc 0] [pid low] [pid high] */
                    cmdbuf[4] = 0xFFFE0000UL | (cmdbuf[4] & 0xFFFFUL);
                }
                
                Log(armGetTls(), 0x100);
                retval = serviceIpcDispatch(&forward_service);
                if (R_SUCCEEDED(retval)) {
                    if (r.IsDomainMessage) { 
                        ipcParseForDomain(&cur_out_r);
                    } else {
                        ipcParse(&cur_out_r);
                    }

                    struct {
                        u64 magic;
                        u64 result;
                    } *resp = (decltype(resp))cur_out_r.Raw;

                    retval = resp->result;
                }
            }
            
            Log(armGetTls(), 0x100);
            Log(&cmd_id, sizeof(u64));
            u64 retval_for_log = retval;
            Log(&retval_for_log, sizeof(u64));
            if (R_FAILED(retval)) {
                //Reboot();
            }
            
            return retval;
        }
        
        void postprocess(IpcParsedCommand &r, u64 cmd_id) override {
            if (this->active_object == this->service_object && (r.CommandType == IpcCommandType_Request || r.CommandType == IpcCommandType_RequestWithContext)) {
                IpcCommand c;
                ipcInitialize(&c);
                this->service_object->postprocess(cur_out_r, c, cmd_id, (u8 *)this->pointer_buffer.data(), this->pointer_buffer.size());
            } else if (r.CommandType == IpcCommandType_Control || r.CommandType == IpcCommandType_ControlWithContext) {
                if (cmd_id == IpcCtrl_Cmd_ConvertCurrentObjectToDomain) {
                    this->is_domain = true;
                    this->domain = std::make_shared<DomainOwner>();
                    struct {
                        u64 magic;
                        u64 result;
                        u32 domain_id;
                    } *resp = (decltype(resp))cur_out_r.Raw;
                    Result rc;
                    if (R_FAILED((rc = this->domain->set_object(this->service_object, resp->domain_id)))) {
                        fatalSimple(rc);
                    }
                    this->mitm_domain_id = resp->domain_id;
                    this->forward_service.type = ServiceType_Domain;
                    this->forward_service.object_id = resp->domain_id;
                }
            }
        }
        
        void cleanup() override {
            /* Clean up copy handles. */
            for (unsigned int i = 0; i < cur_out_r.NumHandles; i++) {
                if (cur_out_r.WasHandleCopied[i]) {
                    svcCloseHandle(cur_out_r.Handles[i]);
                }
            }
        }
};
