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
#include "imitmserviceobject.hpp"

#include "../results.hpp"
#include "mitm_query_service.hpp"

class MitmSession final : public ServiceSession {
    private:
        /* This will be for the actual session. */
        std::shared_ptr<Service> forward_service;

        struct PostProcessHandlerContext {
            bool closed;
            void (*handler)(IMitmServiceObject *, IpcResponseContext *);
        };
        /* Store a handler for the service. */
        std::shared_ptr<PostProcessHandlerContext> service_post_process_ctx;

        /* For cleanup usage. */
        u64 client_pid;
        u32 num_fwd_copy_hnds = 0;
        Handle fwd_copy_hnds[8];
    public:
        template<typename T>
        MitmSession(Handle s_h, u64 pid, std::shared_ptr<Service> fs, std::shared_ptr<T> srv) : ServiceSession(s_h), client_pid(pid) {
            this->forward_service = std::move(fs);
            this->obj_holder = std::move(ServiceObjectHolder(std::move(srv)));

            this->service_post_process_ctx = std::make_shared<PostProcessHandlerContext>();
            this->service_post_process_ctx->closed = false;
            this->service_post_process_ctx->handler = T::PostProcess;

            size_t pbs;
            R_ASSERT(ipcQueryPointerBufferSize(forward_service->handle, &pbs));
            this->pointer_buffer.resize(pbs);
            this->control_holder.Reset();
            this->control_holder = std::move(ServiceObjectHolder(std::move(std::make_shared<IMitmHipcControlService>(this))));
        }

        MitmSession(Handle s_h, u64 pid, std::shared_ptr<Service> fs, ServiceObjectHolder &&h, std::shared_ptr<PostProcessHandlerContext> ppc) : ServiceSession(s_h), client_pid(pid) {
            this->session_handle = s_h;
            this->forward_service = std::move(fs);
            this->obj_holder = std::move(h);

            this->service_post_process_ctx = ppc;

            size_t pbs;
            R_ASSERT(ipcQueryPointerBufferSize(forward_service->handle, &pbs));
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
            /* Dispatch forwards. */
            R_TRY(serviceIpcDispatch(this->forward_service.get()));

            /* Parse. */
            {
                IpcParsedCommand r;
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

                for (unsigned int i = 0; i < r.NumHandles; i++) {
                    if (r.WasHandleCopied[i]) {
                        this->fwd_copy_hnds[num_fwd_copy_hnds++] = r.Handles[i];
                    }
                }

                R_TRY(resp->result);
            }

            return ResultSuccess;
        }

        virtual Result GetResponse(IpcResponseContext *ctx) {
            FirmwareVersion fw = GetRuntimeFirmwareVersion();

            const ServiceCommandMeta *dispatch_table = ctx->obj_holder->GetDispatchTable();
            size_t entry_count = ctx->obj_holder->GetDispatchTableEntryCount();

            if (IsDomainObject(ctx->obj_holder)) {
                switch (ctx->request.InMessageType) {
                    case DomainMessageType_Invalid:
                        return ResultKernelConnectionClosed;
                    case DomainMessageType_Close:
                    {
                        auto sub_obj = ctx->obj_holder->GetServiceObject<IDomainObject>()->GetObject(ctx->request.InThisObjectId);
                        if (sub_obj == nullptr) {
                            R_TRY(ForwardRequest(ctx));
                            return ResultSuccess;
                        }

                        if (sub_obj->IsMitmObject()) {
                            R_TRY(ForwardRequest(ctx));
                            ctx->obj_holder->GetServiceObject<IDomainObject>()->FreeObject(ctx->request.InThisObjectId);
                        } else {
                            R_TRY(ctx->obj_holder->GetServiceObject<IDomainObject>()->FreeObject(ctx->request.InThisObjectId));
                        }

                        if (ctx->request.InThisObjectId == serviceGetObjectId(this->forward_service.get()) && !this->service_post_process_ctx->closed) {
                            /* If we're not longer MitMing anything, we'll no longer do any postprocessing. */
                            this->service_post_process_ctx->closed = true;
                        }
                        return ResultSuccess;
                    }
                    case DomainMessageType_SendMessage:
                    {
                        auto sub_obj = ctx->obj_holder->GetServiceObject<IDomainObject>()->GetObject(ctx->request.InThisObjectId);
                        if (sub_obj == nullptr) {
                            R_TRY(ForwardRequest(ctx));
                            return ResultSuccess;
                        }
                        dispatch_table = sub_obj->GetDispatchTable();
                        entry_count = sub_obj->GetDispatchTableEntryCount();
                    }
                }
            }

            Result (*handler)(IpcResponseContext *ctx) = nullptr;

            for (size_t i = 0; i < entry_count; i++) {
                if (ctx->cmd_id == dispatch_table[i].cmd_id && dispatch_table[i].fw_low <= fw && fw <= dispatch_table[i].fw_high) {
                    handler = dispatch_table[i].handler;
                    break;
                }
            }

            if (handler == nullptr) {
                R_TRY(ForwardRequest(ctx));
                return ResultSuccess;
            }

            R_TRY_CATCH(handler(ctx)) {
                R_CATCH(ResultAtmosphereMitmShouldForwardToSession) {
                    std::memcpy(armGetTls(), this->backup_tls, sizeof(this->backup_tls));
                    R_TRY(ForwardRequest(ctx));
                    return ResultSuccess;
                }
            } R_END_TRY_CATCH;

            return ResultSuccess;
        }

        virtual void PostProcessResponse(IpcResponseContext *ctx) override {
            if ((ctx->cmd_type == IpcCommandType_Request || ctx->cmd_type == IpcCommandType_RequestWithContext) && R_SUCCEEDED(ctx->rc)) {
                if (this->service_post_process_ctx->closed) {
                    return;
                }

                if (!IsDomainObject(ctx->obj_holder) || ctx->request.InThisObjectId == serviceGetObjectId(this->forward_service.get())) {
                    IMitmServiceObject *obj = nullptr;
                    if (!IsDomainObject(ctx->obj_holder)) {
                        obj = ctx->obj_holder->GetServiceObjectUnsafe<IMitmServiceObject>();
                    } else {
                        const auto sub_obj = ctx->obj_holder->GetServiceObject<IDomainObject>()->GetObject(ctx->request.InThisObjectId);
                        if (sub_obj != nullptr) {
                            obj = sub_obj->GetServiceObjectUnsafe<IMitmServiceObject>();
                        }
                    }
                    if (obj != nullptr) {
                        this->service_post_process_ctx->handler(obj, ctx);
                    }
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
                enum class CommandId {
                    ConvertCurrentObjectToDomain = 0,
                    CopyFromCurrentDomain        = 1,
                    CloneCurrentObject           = 2,
                    QueryPointerBufferSize       = 3,
                    CloneCurrentObjectEx         = 4,
                };
            private:
                MitmSession *session;
            public:
                explicit IMitmHipcControlService(MitmSession *s) : session(s) {

                }

                virtual ~IMitmHipcControlService() override { }

            public:
                Result ConvertCurrentObjectToDomain(Out<u32> object_id) {
                    if (IsDomainObject(this->session->obj_holder)) {
                        return ResultKernelConnectionClosed;
                    }

                    R_TRY(serviceConvertToDomain(this->session->forward_service.get()));

                    u32 expected_id = serviceGetObjectId(this->session->forward_service.get());

                    /* Allocate new domain. */
                    auto new_domain = this->session->GetDomainManager()->AllocateDomain();
                    if (new_domain == nullptr) {
                        /* If our domains mismatch, we're in trouble. */
                        return ResultKernelConnectionClosed;
                    }

                    /* Reserve the expected object in the domain for our session. */
                    if (R_FAILED(new_domain->ReserveSpecificObject(expected_id))) {
                        return ResultKernelConnectionClosed;
                    }
                    new_domain->SetObject(expected_id, std::move(this->session->obj_holder));
                    this->session->obj_holder = std::move(ServiceObjectHolder(std::move(new_domain)));

                    /* Return the object id. */
                    object_id.SetValue(expected_id);
                    return ResultSuccess;
                }

                Result CopyFromCurrentDomain(Out<MovedHandle> out_h, u32 id) {
                    auto domain = this->session->obj_holder.GetServiceObject<IDomainObject>();
                    if (domain == nullptr) {
                        return ResultHipcTargetNotDomain;
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
                        R_TRY(ipcDispatch(this->session->forward_service->handle));
                        {
                            IpcParsedCommand r;
                            ipcParse(&r);
                            struct {
                                u64 magic;
                                u64 result;
                            } *raw = (decltype(raw))r.Raw;
                            R_TRY(raw->result);

                            out_h.SetValue(r.Handles[0]);
                            this->session->fwd_copy_hnds[this->session->num_fwd_copy_hnds++] = r.Handles[0];
                        }
                        return ResultSuccess;
                    }

                    Handle server_h, client_h;
                    R_ASSERT(SessionManagerBase::CreateSessionHandles(&server_h, &client_h));
                    out_h.SetValue(client_h);

                    if (id == serviceGetObjectId(this->session->forward_service.get())) {
                        this->session->GetSessionManager()->AddWaitable(new MitmSession(server_h, this->session->client_pid, this->session->forward_service, std::move(object->Clone()), this->session->service_post_process_ctx));
                    } else {
                        this->session->GetSessionManager()->AddSession(server_h, std::move(object->Clone()));
                    }
                    return ResultSuccess;
                }

                void CloneCurrentObject(Out<MovedHandle> out_h) {
                    Handle server_h, client_h;
                    R_ASSERT(SessionManagerBase::CreateSessionHandles(&server_h, &client_h));

                    this->session->GetSessionManager()->AddWaitable(new MitmSession(server_h, this->session->client_pid, this->session->forward_service, std::move(this->session->obj_holder.Clone()), this->session->service_post_process_ctx));
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
                    MAKE_SERVICE_COMMAND_META(MitmSession::IMitmHipcControlService, ConvertCurrentObjectToDomain),
                    MAKE_SERVICE_COMMAND_META(MitmSession::IMitmHipcControlService, CopyFromCurrentDomain),
                    MAKE_SERVICE_COMMAND_META(MitmSession::IMitmHipcControlService, CloneCurrentObject),
                    MAKE_SERVICE_COMMAND_META(MitmSession::IMitmHipcControlService, QueryPointerBufferSize),
                    MAKE_SERVICE_COMMAND_META(MitmSession::IMitmHipcControlService, CloneCurrentObjectEx),
                };
        };
};
