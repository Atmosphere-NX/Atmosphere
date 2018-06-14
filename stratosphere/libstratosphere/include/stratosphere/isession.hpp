#pragma once
#include <switch.h>
#include <type_traits>

#include "ipc_templating.hpp"
#include "iserviceobject.hpp"
#include "iwaitable.hpp"
#include "iserver.hpp"

#include "domainowner.hpp"

enum IpcControlCommand {
    IpcCtrl_Cmd_ConvertCurrentObjectToDomain = 0,
    IpcCtrl_Cmd_CopyFromCurrentDomain = 1,
    IpcCtrl_Cmd_CloneCurrentObject = 2,
    IpcCtrl_Cmd_QueryPointerBufferSize = 3,
    IpcCtrl_Cmd_CloneCurrentObjectEx = 4
};

#define POINTER_BUFFER_SIZE_MAX 0xFFFF
#define RESULT_DEFER_SESSION (0x6580A)


template <typename T>
class IServer;

class IServiceObject;

template <typename T>
class ISession : public IWaitable {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    protected:
        std::shared_ptr<T> service_object;
        IServer<T> *server;
        Handle server_handle;
        Handle client_handle;
        char *pointer_buffer;
        size_t pointer_buffer_size;
        
        bool is_domain;
        std::shared_ptr<DomainOwner> domain;
        
        
        std::shared_ptr<IServiceObject> active_object;
    
    static_assert(sizeof(pointer_buffer) <= POINTER_BUFFER_SIZE_MAX, "Incorrect Size for PointerBuffer!");
    
    public:
        ISession<T>(IServer<T> *s, Handle s_h, Handle c_h, size_t pbs = 0x400) : server(s), server_handle(s_h), client_handle(c_h), pointer_buffer_size(pbs) {
            this->service_object = std::make_shared<T>();
            if (this->pointer_buffer_size) {
                this->pointer_buffer = new char[this->pointer_buffer_size];
            }
            this->is_domain = false;
            this->domain.reset();
            this->active_object.reset();
        }
        
        ISession<T>(IServer<T> *s, Handle s_h, Handle c_h, std::shared_ptr<T> so, size_t pbs = 0x400) : service_object(so), server(s), server_handle(s_h), client_handle(c_h), pointer_buffer_size(pbs) {
            if (this->pointer_buffer_size) {
                this->pointer_buffer = new char[this->pointer_buffer_size];
            }
            this->is_domain = false;
            this->domain.reset();
            this->active_object.reset();
        }

        ~ISession() override {
            delete this->pointer_buffer;
            if (server_handle) {
                svcCloseHandle(server_handle);
            }
            if (client_handle) {
                svcCloseHandle(client_handle);
            }
        }
        
        void close_handles() {
            if (server_handle) {
                svcCloseHandle(server_handle);
                server_handle = 0;
            }
            if (client_handle) {
                svcCloseHandle(client_handle);
                client_handle = 0;
            }
        }
        
        std::shared_ptr<T> get_service_object() { return this->service_object; }
        Handle get_server_handle() { return this->server_handle; }
        Handle get_client_handle() { return this->client_handle; }
        
        
        DomainOwner *get_owner() { return this->is_domain ? this->domain.get() : NULL; }
        
        /* IWaitable */        
        Handle get_handle() override {
            return this->server_handle;
        }
        
        void handle_deferred() override {
            Result rc = this->service_object->handle_deferred();
            int handle_index;
            
            if (rc != RESULT_DEFER_SESSION) {
                this->set_deferred(false);
                if (rc == 0xF601) {
                    svcCloseHandle(this->get_handle());
                } else {
                    rc = svcReplyAndReceive(&handle_index, &this->server_handle, 0, this->server_handle, 0);
                }
            }
        }
        
        
        virtual Result handle_message(IpcParsedCommand &r) {
            Result retval = 0xF601;
            
            IpcCommand c;
            ipcInitialize(&c);
            
            if (r.IsDomainMessage && this->active_object == NULL) {
                return 0xF601;
            }
           
            
            if (r.IsDomainMessage && r.MessageType == DomainMessageType_Close) {
                this->domain->delete_object(this->active_object);
                this->active_object = NULL;
                struct {
                    u64 magic;
                    u64 result;
                } *raw = (decltype(raw))ipcPrepareHeader(&c, sizeof(*raw));
                
                raw->magic = SFCO_MAGIC;
                raw->result = 0x0;
                return 0x0;
            }
                        
            u64 cmd_id = ((u32 *)r.Raw)[2];
            switch (r.CommandType) {
                case IpcCommandType_Close:
                    /* TODO: This should close the session and clean up its resources. */
                    retval = 0xF601;
                    break;
                case IpcCommandType_LegacyControl:
                    /* TODO: What does this allow one to do? */
                    retval = 0xF601;
                    break;
                case IpcCommandType_LegacyRequest:
                    /* TODO: What does this allow one to do? */
                    retval = 0xF601;
                    break;
                case IpcCommandType_Request:
                case IpcCommandType_RequestWithContext:
                    retval = this->active_object->dispatch(r, c, cmd_id, (u8 *)this->pointer_buffer, this->pointer_buffer_size);
                    break;
                case IpcCommandType_Control:
                case IpcCommandType_ControlWithContext:
                    retval = this->dispatch_control_command(r, c, cmd_id);            
                    break;                  
                case IpcCommandType_Invalid:
                default:
                    retval = 0xF601;
                    break;
            }
            
            return retval;
        }

        virtual void postprocess(IpcParsedCommand &r, u64 cmd_id) {
            /* ... */
            (void)(r);
            (void)(cmd_id);
        }
        
        virtual void cleanup() {
            /* ... */
        }
        
        Result handle_signaled(u64 timeout) override {
            Result rc;
            int handle_index;
            
            /* Prepare pointer buffer... */
            IpcCommand c_for_reply;
            ipcInitialize(&c_for_reply);
            ipcAddRecvStatic(&c_for_reply, this->pointer_buffer, this->pointer_buffer_size, 0);
            ipcPrepareHeader(&c_for_reply, 0);
            
            if (R_SUCCEEDED(rc = svcReplyAndReceive(&handle_index, &this->server_handle, 1, 0, U64_MAX))) {
                if (handle_index != 0) {
                    /* TODO: Panic? */
                }                                
                IpcParsedCommand r;
                u64 cmd_id;
                
                
                Result retval = ipcParse(&r);
                if (R_SUCCEEDED(retval)) {
                    if (this->is_domain && (r.CommandType == IpcCommandType_Request || r.CommandType == IpcCommandType_RequestWithContext)) {
                        retval = ipcParseForDomain(&r);
                        if (!r.IsDomainMessage || r.ThisObjectId >= DOMAIN_ID_MAX) {
                            retval = 0xF601;
                        } else {
                            this->active_object = this->domain->get_domain_object(r.ThisObjectId);
                        }
                    } else {
                        this->active_object = this->service_object;
                    }
                }
                if (R_SUCCEEDED(retval)) {    
                    cmd_id = ((u32 *)r.Raw)[2];
                }
                if (R_SUCCEEDED(retval)) {
                    retval = this->handle_message(r);
                }
                
                if (retval == RESULT_DEFER_SESSION) {
                    /* Session defer. */
                    this->active_object.reset();
                    this->set_deferred(true);
                    rc = retval;
                } else if (retval == 0xF601) {
                    /* Session close. */
                    this->active_object.reset();
                    rc = retval;
                } else {
                    if (R_SUCCEEDED(retval)) {
                        this->postprocess(r, cmd_id);
                    }
                    this->active_object.reset();
                    rc = svcReplyAndReceive(&handle_index, &this->server_handle, 0, this->server_handle, 0);
                    if (rc == 0xEA01) {
                        rc = 0x0;
                    }
                    this->cleanup();
                }
            }
              
            return rc;
        }
        
        Result dispatch_control_command(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id) {
            Result rc = 0xF601;
            
            /* TODO: Implement. */
            switch ((IpcControlCommand)cmd_id) {
                case IpcCtrl_Cmd_ConvertCurrentObjectToDomain:
                    rc = WrapIpcCommandImpl<&ISession::ConvertCurrentObjectToDomain>(this, r, out_c, (u8 *)this->pointer_buffer, this->pointer_buffer_size);
                    break;
                case IpcCtrl_Cmd_CopyFromCurrentDomain:
                    rc = WrapIpcCommandImpl<&ISession::CopyFromCurrentDomain>(this, r, out_c, (u8 *)this->pointer_buffer, this->pointer_buffer_size);
                    break;
                case IpcCtrl_Cmd_CloneCurrentObject:
                    rc = WrapIpcCommandImpl<&ISession::CloneCurrentObject>(this, r, out_c, (u8 *)this->pointer_buffer, this->pointer_buffer_size);
                    break;
                case IpcCtrl_Cmd_QueryPointerBufferSize:
                    rc = WrapIpcCommandImpl<&ISession::QueryPointerBufferSize>(this, r, out_c, (u8 *)this->pointer_buffer, this->pointer_buffer_size);
                    break;
                case IpcCtrl_Cmd_CloneCurrentObjectEx:
                    rc = WrapIpcCommandImpl<&ISession::CloneCurrentObjectEx>(this, r, out_c, (u8 *)this->pointer_buffer, sizeof(this->pointer_buffer));
                    break;
                default:
                    break;
            }
            
            return rc;
        }

        /* Control commands. */
        std::tuple<Result> ConvertCurrentObjectToDomain() {
            /* TODO */
            return {0xF601};
        }
        std::tuple<Result> CopyFromCurrentDomain() {
            /* TODO */
            return {0xF601};
        }
        std::tuple<Result> CloneCurrentObject() {
            /* TODO */
            return {0xF601};
        }
        std::tuple<Result, u32> QueryPointerBufferSize() {
            return {0x0, (u32)this->pointer_buffer_size};
        }
        std::tuple<Result> CloneCurrentObjectEx() {
            /* TODO */
            return {0xF601};
        }
};
