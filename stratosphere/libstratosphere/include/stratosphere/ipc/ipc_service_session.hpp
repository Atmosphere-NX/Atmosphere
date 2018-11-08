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

#include "../iwaitable.hpp"
#include "ipc_service_object.hpp"
#include "ipc_serialization.hpp"

enum HipcControlCommand : u32 {
    HipcControlCommand_ConvertCurrentObjectToDomain = 0,
    HipcControlCommand_CopyFromCurrentDomain = 1,
    HipcControlCommand_CloneCurrentObject = 2,
    HipcControlCommand_QueryPointerBufferSize = 3,
    HipcControlCommand_CloneCurrentObjectEx = 4
};


#define RESULT_DEFER_SESSION (0x6580A)


class ServiceSession : public IWaitable
{
    protected:
        Handle session_handle;
        std::vector<unsigned char> pointer_buffer;
        ServiceObjectHolder obj_holder;
        ServiceObjectHolder control_holder = ServiceObjectHolder(std::make_shared<IHipcControlService>(this));
        u8 backup_tls[0x100];
        
        ServiceSession(Handle s_h) : session_handle(s_h) { }
    public:
        template<typename T>
        ServiceSession(Handle s_h, size_t pbs) : session_handle(s_h), pointer_buffer(pbs), obj_holder(std::make_shared<T>()) { }
        
        ServiceSession(Handle s_h, size_t pbs, ServiceObjectHolder &&h) : session_handle(s_h), pointer_buffer(pbs), obj_holder(std::move(h)) { }
        
        virtual ~ServiceSession() override {
            svcCloseHandle(this->session_handle);
        }

        SessionManagerBase *GetSessionManager() {
            return static_cast<SessionManagerBase *>(this->GetManager());
        }
        
        DomainManager *GetDomainManager() {
            return static_cast<DomainManager *>(this->GetSessionManager());
        }
        
        Result Receive() {
            int handle_index;
            /* Prepare pointer buffer... */
            IpcCommand c;
            ipcInitialize(&c);
            if (this->pointer_buffer.size() > 0) {
                ipcAddRecvStatic(&c, this->pointer_buffer.data(), this->pointer_buffer.size(), 0);
                ipcPrepareHeader(&c, 0);
                
                /* Fix libnx bug in serverside C descriptor handling. */
                ((u32 *)armGetTls())[1] &= 0xFFFFC3FF;
                ((u32 *)armGetTls())[1] |= (2) << 10;
            } else {
                ipcPrepareHeader(&c, 0);
            }
            
            /* Receive. */
            Result rc = svcReplyAndReceive(&handle_index, &this->session_handle, 1, 0, U64_MAX);
            if (R_SUCCEEDED(rc)) {
                std::memcpy(this->backup_tls, armGetTls(), sizeof(this->backup_tls));
            }
            
            return rc;
        }
        
        Result Reply() {
            int handle_index;
            return svcReplyAndReceive(&handle_index, &this->session_handle, 0, this->session_handle, 0);
        }
        
        /* For preparing basic replies. */
        Result PrepareBasicResponse(IpcResponseContext *ctx, Result rc) {
            ipcInitialize(&ctx->reply);
            struct {
                u64 magic;
                u64 result;
            } *raw = (decltype(raw))ipcPrepareHeader(&ctx->reply, sizeof(*raw));
            
            raw->magic = SFCO_MAGIC;
            raw->result = rc;
            return raw->result;
        }
        
        Result PrepareBasicDomainResponse(IpcResponseContext *ctx, Result rc) {
            ipcInitialize(&ctx->reply);
            struct {
                DomainResponseHeader hdr;
                u64 magic;
                u64 result;
            } *raw = (decltype(raw))ipcPrepareHeader(&ctx->reply, sizeof(*raw));
            
            raw->hdr = (DomainResponseHeader){0};
            raw->magic = SFCO_MAGIC;
            raw->result = rc;
            return raw->result;
        }
        
        /* For making a new response context. */
        void InitializeResponseContext(IpcResponseContext *ctx) {
            std::memset(ctx, 0, sizeof(*ctx));
            ctx->manager = this->GetSessionManager();
            ctx->obj_holder = &this->obj_holder;
            ctx->pb = this->pointer_buffer.data();
            ctx->pb_size = this->pointer_buffer.size();
        }
        
        /* IWaitable */        
        virtual Handle GetHandle() {
            return this->session_handle;
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
                        return PrepareBasicDomainResponse(ctx, ctx->obj_holder->GetServiceObject<IDomainObject>()->FreeObject(ctx->request.InThisObjectId));
                    case DomainMessageType_SendMessage:
                    {
                        auto sub_obj = ctx->obj_holder->GetServiceObject<IDomainObject>()->GetObject(ctx->request.InThisObjectId);
                        if (sub_obj == nullptr) {
                            return PrepareBasicDomainResponse(ctx, 0x3D80B);
                        }
                        dispatch_table = sub_obj->GetDispatchTable();
                        entry_count = sub_obj->GetDispatchTableEntryCount();
                    }
                }
            }

            for (size_t i = 0; i < entry_count; i++) {
                if (ctx->cmd_id == dispatch_table[i].cmd_id && dispatch_table[i].fw_low <= fw && fw <= dispatch_table[i].fw_high) {
                    rc = dispatch_table[i].handler(ctx);
                    break;
                }
            }
            
            return rc;
        }
        
        virtual Result HandleReceived() {
            IpcResponseContext ctx;
            this->InitializeResponseContext(&ctx);
            
            ctx.cmd_type = (IpcCommandType)(*(u16 *)(armGetTls()));
            
            ctx.rc = 0;
            
            /* Parse based on command type. */
            switch (ctx.cmd_type) {
                case IpcCommandType_Invalid:
                case IpcCommandType_LegacyRequest:
                case IpcCommandType_LegacyControl:
                    return 0xF601;
                case IpcCommandType_Close:
                    {
                        /* Clean up gracefully. */
                        PrepareBasicResponse(&ctx, 0);
                        this->Reply();
                    }
                    return 0xF601;
                case IpcCommandType_Control:
                case IpcCommandType_ControlWithContext:
                    ctx.rc = ipcParse(&ctx.request);
                    ctx.obj_holder = &this->control_holder;
                    break;
                case IpcCommandType_Request:
                case IpcCommandType_RequestWithContext:
                    if (IsDomainObject(&this->obj_holder)) {
                        ctx.rc = ipcParseDomainRequest(&ctx.request);
                    } else {
                        ctx.rc = ipcParse(&ctx.request);
                    }
                    break;
                default:
                    return 0xF601;
            }
            
            
            if (R_SUCCEEDED(ctx.rc)) {
                ctx.cmd_id = ((u32 *)ctx.request.Raw)[2];
                this->PreProcessRequest(&ctx);
                ctx.rc = this->GetResponse(&ctx);
            }
            
            if (ctx.rc == RESULT_DEFER_SESSION) {
                /* Session defer. */
                this->SetDeferred(true);
            } else if (ctx.rc == 0xF601) {
                /* Session close, nothing to do. */
            } else {
                if (R_SUCCEEDED(ctx.rc)) {
                    this->PostProcessResponse(&ctx);
                }
                
                ctx.rc = this->Reply();
                
                if (ctx.rc == 0xEA01) {
                    ctx.rc = 0x0;
                }
                
                this->CleanupResponse(&ctx);
            }
            
            return ctx.rc;
        }
        
        virtual Result HandleDeferred() override {
            memcpy(armGetTls(), this->backup_tls, sizeof(this->backup_tls));
            Result rc = this->HandleReceived();
            
            if (rc != RESULT_DEFER_SESSION) {
                this->SetDeferred(false);
            }
            return rc;
        }
        
        virtual Result HandleSignaled(u64 timeout) {
            Result rc;
                        
            if (R_SUCCEEDED(rc = this->Receive())) {
                rc = this->HandleReceived();
            }
            
            return rc;
        }
        
        virtual void PreProcessRequest(IpcResponseContext *ctx) {
            /* ... */
            (void)(ctx);
        }
        
        virtual void PostProcessResponse(IpcResponseContext *ctx) {
            /* ... */
            (void)(ctx);
        }
        
        virtual void CleanupResponse(IpcResponseContext *ctx) {
            std::memset(this->backup_tls, 0, sizeof(this->backup_tls));
        }

                
    public:
        class IHipcControlService : public IServiceObject  {
            private:
                ServiceSession *session;
            public:
                explicit IHipcControlService(ServiceSession *s) : session(s) {
                    
                }
                
                virtual ~IHipcControlService() override { }
                
                Result ConvertCurrentObjectToDomain(Out<u32> object_id) {
                    /* Allocate new domain. */
                    auto new_domain = this->session->GetDomainManager()->AllocateDomain();
                    if (new_domain == nullptr) {
                        return 0x1900B;
                    }
                                        
                    /* Reserve an object in the domain for our session. */
                    u32 reserved_id;
                    Result rc = new_domain->ReserveObject(&reserved_id);
                    if (R_FAILED(rc)) {
                        return rc;
                    }
                    new_domain->SetObject(reserved_id, std::move(this->session->obj_holder));
                    this->session->obj_holder = std::move(ServiceObjectHolder(std::move(new_domain)));
                    
                    /* Return the object id. */
                    object_id.SetValue(reserved_id);
                    return 0;
                }
                
                Result CopyFromCurrentDomain(Out<MovedHandle> out_h, u32 id) {
                    auto domain = this->session->obj_holder.GetServiceObject<IDomainObject>();
                    if (domain == nullptr) {
                        return 0x3D60B;
                    }
                    
                    
                    auto object = domain->GetObject(id);
                    if (object == nullptr) {
                        return 0x3D80B;
                    }
                    
                    Handle server_h, client_h;
                    if (R_FAILED(SessionManagerBase::CreateSessionHandles(&server_h, &client_h))) {
                        /* N aborts here. Should we error code? */
                        std::abort();
                    }
                    
                    this->session->GetSessionManager()->AddSession(server_h, std::move(object->Clone()));
                    out_h.SetValue(client_h);
                    return 0;
                }
                
                void CloneCurrentObject(Out<MovedHandle> out_h) {
                    Handle server_h, client_h;
                    if (R_FAILED(SessionManagerBase::CreateSessionHandles(&server_h, &client_h))) {
                        /* N aborts here. Should we error code? */
                        std::abort();
                    }
                    
                    this->session->GetSessionManager()->AddSession(server_h, std::move(this->session->obj_holder.Clone()));
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
                    MakeServiceCommandMeta<HipcControlCommand_ConvertCurrentObjectToDomain, &ServiceSession::IHipcControlService::ConvertCurrentObjectToDomain>(),
                    MakeServiceCommandMeta<HipcControlCommand_CopyFromCurrentDomain, &ServiceSession::IHipcControlService::CopyFromCurrentDomain>(),
                    MakeServiceCommandMeta<HipcControlCommand_CloneCurrentObject, &ServiceSession::IHipcControlService::CloneCurrentObject>(),
                    MakeServiceCommandMeta<HipcControlCommand_QueryPointerBufferSize, &ServiceSession::IHipcControlService::QueryPointerBufferSize>(),
                    MakeServiceCommandMeta<HipcControlCommand_CloneCurrentObjectEx, &ServiceSession::IHipcControlService::CloneCurrentObjectEx>(),
                };
        };
};
