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

#define RESULT_FORWARD_TO_SESSION 0xCAFEFC

class MitmSession final : public ServiceSession {    
    private:
        /* This will be for the actual session. */
        std::shared_ptr<Service> forward_service;
        
        /* Store a handler for the service. */
        void (*service_post_process_handler)(IMitmServiceObject *, IpcResponseContext *);
        
        /* For cleanup usage. */
        u32 num_fwd_copy_hnds = 0;
        Handle fwd_copy_hnds[8];
    public:
        template<typename T>
        MitmSession(Handle s_h, std::shared_ptr<Service> fs, std::shared_ptr<T> srv) : ServiceSession(s_h) {
            this->forward_service = std::move(fs);
            this->obj_holder = std::move(ServiceObjectHolder(std::move(srv)));
            
            this->service_post_process_handler = T::PostProcess;
            
            size_t pbs;
            if (R_FAILED(ipcQueryPointerBufferSize(forward_service->handle, &pbs))) {
                std::abort();
            }
            this->pointer_buffer.resize(pbs);
            this->control_holder.Reset();
            this->control_holder = std::move(ServiceObjectHolder(std::move(std::make_shared<IMitmHipcControlService>(this))));
        }
        
        MitmSession(Handle s_h, std::shared_ptr<Service> fs, ServiceObjectHolder &&h, void (*pph)(IMitmServiceObject *, IpcResponseContext *)) : ServiceSession(s_h) {
            this->session_handle = s_h;
            this->forward_service = std::move(fs);
            this->obj_holder = std::move(h);
            
            this->service_post_process_handler = pph;
            
            size_t pbs;
            if (R_FAILED(ipcQueryPointerBufferSize(forward_service->handle, &pbs))) {
                std::abort();
            }
            this->pointer_buffer.resize(pbs);
            this->control_holder.Reset();
            this->control_holder = std::move(ServiceObjectHolder(std::move(std::make_shared<IMitmHipcControlService>(this))));
        }
        
        virtual void PreProcessRequest(IpcResponseContext *ctx) override {
            u32 *cmdbuf = (u32 *)armGetTls();
            u32 *backup_cmdbuf = (u32 *)this->backup_tls;
            if (ctx->request.HasPid) {
                /* [ctrl 0] [ctrl 1] [handle desc 0] [pid low] [pid high] */
                cmdbuf[4] = 0xFFFE0000UL | (cmdbuf[4] & 0xFFFFUL);
                backup_cmdbuf[4] = cmdbuf[4];
            }
        }
        
        Result ForwardRequest(IpcResponseContext *ctx) {
            IpcParsedCommand r;
            Result rc = serviceIpcDispatch(this->forward_service.get());
            if (R_SUCCEEDED(rc)) {
                if (ctx->request.IsDomainRequest) { 
                    /* We never work with out object ids, so this should be fine. */
                    ipcParseDomainResponse(&r, 0);
                } else {
                    ipcParse(&r);
                }

                struct {
                    u64 magic;
                    u64 result;
                } *resp = (decltype(resp))r.Raw;

                rc = resp->result;
                
                for (unsigned int i = 0; i < r.NumHandles; i++) {
                    if (r.WasHandleCopied[i]) {
                        this->fwd_copy_hnds[num_fwd_copy_hnds++] = r.Handles[i];
                    }
                }
            }
            return rc;
        }
                        
        virtual Result GetResponse(IpcResponseContext *ctx) {
            Result rc = 0xF601;
            FirmwareVersion fw = GetRuntimeFirmwareVersion();
            
            const ServiceCommandMeta *dispatch_table = ctx->obj_holder->GetDispatchTable();
            size_t entry_count = ctx->obj_holder->GetDispatchTableEntryCount();
                        
            if (IsDomainObject(ctx->obj_holder)) {
                switch (ctx->request.InMessageType) {
                    case DomainMessageType_Invalid:
                        return 0xF601;
                    case DomainMessageType_Close:
                        rc = ForwardRequest(ctx);
                        if (R_SUCCEEDED(rc)) {
                            ctx->obj_holder->GetServiceObject<IDomainObject>()->FreeObject(ctx->request.InThisObjectId);
                        }
                        if (R_SUCCEEDED(rc) && ctx->request.InThisObjectId == serviceGetObjectId(this->forward_service.get())) {
                            /* If we're not longer MitMing anything, we don't need a mitm session. */
                            this->Reply();
                            this->GetSessionManager()->AddSession(this->session_handle, std::move(this->obj_holder));
                            this->session_handle = 0;
                            return 0xF601;
                        }
                        return rc;
                    case DomainMessageType_SendMessage:
                    {
                        auto sub_obj = ctx->obj_holder->GetServiceObject<IDomainObject>()->GetObject(ctx->request.InThisObjectId);
                        if (sub_obj == nullptr) {
                            rc = ForwardRequest(ctx);
                            return rc;
                        }
                        dispatch_table = sub_obj->GetDispatchTable();
                        entry_count = sub_obj->GetDispatchTableEntryCount();
                    }
                }
            }
            
            bool found_entry = false;

            for (size_t i = 0; i < entry_count; i++) {
                if (ctx->cmd_id == dispatch_table[i].cmd_id && dispatch_table[i].fw_low <= fw && fw <= dispatch_table[i].fw_high) {
                    rc = dispatch_table[i].handler(ctx);
                    found_entry = true;
                    break;
                }
            }
            
            if (!found_entry || rc == RESULT_FORWARD_TO_SESSION) {
                memcpy(armGetTls(), this->backup_tls, sizeof(this->backup_tls));
                rc = ForwardRequest(ctx);
            }
                                                                                    
            return rc;
        }
        
        virtual void PostProcessResponse(IpcResponseContext *ctx) override {
            if ((ctx->cmd_type == IpcCommandType_Request || ctx->cmd_type == IpcCommandType_RequestWithContext) && R_SUCCEEDED(ctx->rc)) {
                if (!IsDomainObject(ctx->obj_holder) || ctx->request.InThisObjectId == serviceGetObjectId(this->forward_service.get())) {
                    IMitmServiceObject *obj;
                    if (!IsDomainObject(ctx->obj_holder)) {
                        obj = ctx->obj_holder->GetServiceObjectUnsafe<IMitmServiceObject>();
                    } else {
                        obj = ctx->obj_holder->GetServiceObject<IDomainObject>()->GetObject(ctx->request.InThisObjectId)->GetServiceObjectUnsafe<IMitmServiceObject>();
                    }
                    this->service_post_process_handler(obj, ctx);
                }
            }
        }
        
        virtual void CleanupResponse(IpcResponseContext *ctx) override {
            /* Cleanup tls backup. */
            std::memset(this->backup_tls, 0, sizeof(this->backup_tls));
            
            /* Clean up copy handles. */
            for (unsigned int i = 0; i < ctx->request.NumHandles; i++) {
                if (ctx->request.WasHandleCopied[i]) {
                    svcCloseHandle(ctx->request.Handles[i]);
                }
            }
            for (unsigned int i = 0; i < this->num_fwd_copy_hnds; i++) {
                svcCloseHandle(this->fwd_copy_hnds[i]);
            }
            this->num_fwd_copy_hnds = 0;
        }
        
    public:
        class IMitmHipcControlService : public IServiceObject {
            private:
                MitmSession *session;
            public:
                explicit IMitmHipcControlService(MitmSession *s) : session(s) {
                    
                }
                
                virtual ~IMitmHipcControlService() override { }

            public:
                Result ConvertCurrentObjectToDomain(Out<u32> object_id) {
                    if (IsDomainObject(this->session->obj_holder)) {
                        return 0xF601;
                    }
                    
                    Result rc = serviceConvertToDomain(this->session->forward_service.get());
                    if (R_FAILED(rc)) {
                        return rc;
                    }
                    
                    u32 expected_id = serviceGetObjectId(this->session->forward_service.get());
                    
                    /* Allocate new domain. */
                    auto new_domain = this->session->GetDomainManager()->AllocateDomain();
                    if (new_domain == nullptr) {
                        /* If our domains mismatch, we're in trouble. */
                        return 0xF601;
                    }
                                        
                    /* Reserve the expected object in the domain for our session. */
                    if (R_FAILED(new_domain->ReserveSpecificObject(expected_id))) {
                        return 0xF601;
                    }
                    new_domain->SetObject(expected_id, std::move(this->session->obj_holder));
                    this->session->obj_holder = std::move(ServiceObjectHolder(std::move(new_domain)));
                    
                    /* Return the object id. */
                    object_id.SetValue(expected_id);
                    return 0;
                }
                
                Result CopyFromCurrentDomain(Out<MovedHandle> out_h, u32 id) {
                    auto domain = this->session->obj_holder.GetServiceObject<IDomainObject>();
                    if (domain == nullptr) {
                        return 0x3D60B;
                    }
                    
                    
                    auto object = domain->GetObject(id);
                    if (object == nullptr) {
                        /* Forward onwards. */
                        u32 *buf = (u32 *)armGetTls();
                        buf[0] = IpcCommandType_Control;
                        buf[1] = 0xA;
                        buf[4] = SFCI_MAGIC;
                        buf[5] = 0;
                        buf[6] = 1;
                        buf[7] = 0;
                        buf[8] = id;
                        buf[9] = 0;
                        Result rc = ipcDispatch(this->session->forward_service->handle);
                        if (R_SUCCEEDED(rc)) {
                            IpcParsedCommand r;
                            ipcParse(&r);
                            struct {
                                u64 magic;
                                u64 result;
                            } *raw = (decltype(raw))r.Raw;
                            rc = raw->result;
                            
                            if (R_SUCCEEDED(rc)) {
                                out_h.SetValue(r.Handles[0]);
                                this->session->fwd_copy_hnds[this->session->num_fwd_copy_hnds++] = r.Handles[0];
                            }
                        }
                        return rc;
                    }
                    
                    Handle server_h, client_h;
                    if (R_FAILED(SessionManagerBase::CreateSessionHandles(&server_h, &client_h))) {
                        /* N aborts here. Should we error code? */
                        std::abort();
                    }
                    out_h.SetValue(client_h);
                    
                    if (id == serviceGetObjectId(this->session->forward_service.get())) {
                        this->session->GetSessionManager()->AddWaitable(new MitmSession(server_h, this->session->forward_service, std::move(object->Clone()), this->session->service_post_process_handler));
                    } else {
                        this->session->GetSessionManager()->AddSession(server_h, std::move(object->Clone()));
                    }
                    return 0;
                }
                
                void CloneCurrentObject(Out<MovedHandle> out_h) {
                    Handle server_h, client_h;
                    if (R_FAILED(SessionManagerBase::CreateSessionHandles(&server_h, &client_h))) {
                        /* N aborts here. Should we error code? */
                        std::abort();
                    }
                    
                    this->session->GetSessionManager()->AddWaitable(new MitmSession(server_h, this->session->forward_service, std::move(this->session->obj_holder.Clone()), this->session->service_post_process_handler));
                    out_h.SetValue(client_h);
                }
                
                void QueryPointerBufferSize(Out<u16> size) {
                    size.SetValue(this->session->pointer_buffer.size());
                }
                
                void CloneCurrentObjectEx(Out<MovedHandle> out_h, u32 which) {
                    /* TODO: Figure out what this u32 controls. */
                    return CloneCurrentObject(out_h);
                }
                
            public:
                DEFINE_SERVICE_DISPATCH_TABLE {
                    MakeServiceCommandMeta<HipcControlCommand_ConvertCurrentObjectToDomain, &MitmSession::IMitmHipcControlService::ConvertCurrentObjectToDomain>(),
                    MakeServiceCommandMeta<HipcControlCommand_CopyFromCurrentDomain, &MitmSession::IMitmHipcControlService::CopyFromCurrentDomain>(),
                    MakeServiceCommandMeta<HipcControlCommand_CloneCurrentObject, &MitmSession::IMitmHipcControlService::CloneCurrentObject>(),
                    MakeServiceCommandMeta<HipcControlCommand_QueryPointerBufferSize, &MitmSession::IMitmHipcControlService::QueryPointerBufferSize>(),
                    MakeServiceCommandMeta<HipcControlCommand_CloneCurrentObjectEx, &MitmSession::IMitmHipcControlService::CloneCurrentObjectEx>(),
                };
        };
};
