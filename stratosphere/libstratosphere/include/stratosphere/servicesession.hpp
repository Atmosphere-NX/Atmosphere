#pragma once
#include <switch.h>
#include <type_traits>

#include "ipc_templating.hpp"
#include "iserviceobject.hpp"
#include "iwaitable.hpp"
#include "iserver.hpp"

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

template <typename T>
class ServiceSession final : public IWaitable {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    
    T *service_object;
    IServer<T> *server;
    Handle server_handle;
    Handle client_handle;
    char pointer_buffer[0x400];
    
    static_assert(sizeof(pointer_buffer) <= POINTER_BUFFER_SIZE_MAX, "Incorrect Size for PointerBuffer!");
    
    public:
        ServiceSession<T>(IServer<T> *s, Handle s_h, Handle c_h) : server(s), server_handle(s_h), client_handle(c_h) {
            this->service_object = new T();
        }
        
        ~ServiceSession() override {
            delete this->service_object;
            if (server_handle) {
                svcCloseHandle(server_handle);
            }
            if (client_handle) {
                svcCloseHandle(client_handle);
            }
        }

        T *get_service_object() { return this->service_object; }
        Handle get_server_handle() { return this->server_handle; }
        Handle get_client_handle() { return this->client_handle; }
        
        /* IWaitable */
        unsigned int get_num_waitables() override {
            return 1;
        }
        
        void get_waitables(IWaitable **dst) override {
            dst[0] = this;
        }
        
        void delete_child(IWaitable *child) override {
            /* TODO: Panic, because we can never have any children. */
        }
        
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
        
        Result handle_signaled(u64 timeout) override {
            Result rc;
            int handle_index;
            
            /* Prepare pointer buffer... */
            IpcCommand c_for_reply;
            ipcInitialize(&c_for_reply);
            ipcAddRecvStatic(&c_for_reply, this->pointer_buffer, sizeof(this->pointer_buffer), 0);
            ipcPrepareHeader(&c_for_reply, 0);
            
            if (R_SUCCEEDED(rc = svcReplyAndReceive(&handle_index, &this->server_handle, 1, 0, timeout))) {
                if (handle_index != 0) {
                    /* TODO: Panic? */
                }
                u32 *cmdbuf = (u32 *)armGetTls();
                Result retval = 0;
                u32 *rawdata_start = cmdbuf;
                                
                IpcParsedCommand r;
                IpcCommand c;
                                
                ipcInitialize(&c);
                
                retval = ipcParse(&r);
                                
                if (R_SUCCEEDED(retval)) {
                    rawdata_start = (u32 *)r.Raw;
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
                            retval = this->service_object->dispatch(r, c, rawdata_start[2], (u8 *)this->pointer_buffer, sizeof(this->pointer_buffer));
                            break;
                        case IpcCommandType_Control:
                        case IpcCommandType_ControlWithContext:
                            retval = this->dispatch_control_command(r, c, rawdata_start[2]);
                            break;
                        case IpcCommandType_Invalid:
                        default:
                            retval = 0xF601;
                            break;
                    }
                    
                }
                
                if (retval == RESULT_DEFER_SESSION) {
                    /* Session defer. */
                    this->set_deferred(true);
                    rc = retval;
                } else if (retval == 0xF601) {
                    /* Session close. */
                    rc = retval;
                } else {
                    rc = svcReplyAndReceive(&handle_index, &this->server_handle, 0, this->server_handle, 0);
                }
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
            return {0x0, (u32)sizeof(this->pointer_buffer)};
        }
        std::tuple<Result> CloneCurrentObjectEx() {
            /* TODO */
            return {0xF601};
        }
        
        Result dispatch_control_command(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id) {
            Result rc = 0xF601;
            
            /* TODO: Implement. */
            switch ((IpcControlCommand)cmd_id) {
                case IpcCtrl_Cmd_ConvertCurrentObjectToDomain:
                    rc = WrapIpcCommandImpl<&ServiceSession::ConvertCurrentObjectToDomain>(this, r, out_c, (u8 *)this->pointer_buffer, sizeof(this->pointer_buffer));
                    break;
                case IpcCtrl_Cmd_CopyFromCurrentDomain:
                    rc = WrapIpcCommandImpl<&ServiceSession::CopyFromCurrentDomain>(this, r, out_c, (u8 *)this->pointer_buffer, sizeof(this->pointer_buffer));
                    break;
                case IpcCtrl_Cmd_CloneCurrentObject:
                    rc = WrapIpcCommandImpl<&ServiceSession::CloneCurrentObject>(this, r, out_c, (u8 *)this->pointer_buffer, sizeof(this->pointer_buffer));
                    break;
                case IpcCtrl_Cmd_QueryPointerBufferSize:
                    rc = WrapIpcCommandImpl<&ServiceSession::QueryPointerBufferSize>(this, r, out_c, (u8 *)this->pointer_buffer, sizeof(this->pointer_buffer));
                    break;
                case IpcCtrl_Cmd_CloneCurrentObjectEx:
                    rc = WrapIpcCommandImpl<&ServiceSession::CloneCurrentObjectEx>(this, r, out_c, (u8 *)this->pointer_buffer, sizeof(this->pointer_buffer));
                    break;
                default:
                    break;
            }
            
            return rc;
        }
};
