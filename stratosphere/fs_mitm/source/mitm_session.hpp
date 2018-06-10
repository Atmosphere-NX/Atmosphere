#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "mitm_server.hpp"

#include "debug.hpp"


template <typename T>
class MitMServer;

template <typename T>
class MitMSession final : public IWaitable {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    
    T *service_object;
    MitMServer<T> *server;
    Handle server_handle;
    Handle client_handle;
    /* This will be for the actual session. */
    Service forward_service;
    
    char *pointer_buffer;
    size_t pointer_buffer_size;
    
    static_assert(sizeof(pointer_buffer) <= POINTER_BUFFER_SIZE_MAX, "Incorrect Size for PointerBuffer!");
    public:
        MitMSession<T>(MitMServer<T> *s, Handle s_h, Handle c_h, const char *srv) : server(s), server_handle(s_h), client_handle(c_h) {
            this->service_object = new T();
            if (R_FAILED(smMitMGetService(&forward_service, srv))) {
                /* TODO: Panic. */
            }
            if (R_FAILED(ipcQueryPointerBufferSize(forward_service.handle, &pointer_buffer_size))) {
                /* TODO: Panic. */
            }
            this->pointer_buffer = new char[pointer_buffer_size];
        }
        
        ~MitMSession() override {
            delete this->service_object;
            serviceClose(&forward_service);
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
            /* TODO: Panic, because we can never be deferred. */
        }
        
        Result handle_signaled(u64 timeout) override {
            Result rc;
            int handle_index;
                        
            /* Prepare pointer buffer... */
            IpcCommand c_for_reply;
            ipcInitialize(&c_for_reply);
            ipcAddRecvStatic(&c_for_reply, this->pointer_buffer, this->pointer_buffer_size, 0);
            u32 *cmdbuf = (u32 *)armGetTls();
            ipcPrepareHeader(&c_for_reply, 0);
            
            
            if (R_SUCCEEDED(rc = svcReplyAndReceive(&handle_index, &this->server_handle, 1, 0, timeout))) {
                if (handle_index != 0) {
                    /* TODO: Panic? */
                }
                Log(armGetTls(), 0x100);
                Result retval = 0;
                u32 *rawdata_start = cmdbuf;
                                
                IpcParsedCommand r;
                IpcCommand c;
                IpcParsedCommand out_r;
                out_r.NumHandles = 0;
                                
                ipcInitialize(&c);
                
                retval = ipcParse(&r);
                
                /* TODO: Close input copy handles that we don't need. */
                                
                if (R_SUCCEEDED(retval)) {
                    rawdata_start = (u32 *)r.Raw;
                    retval = 0xF601;
                    if (r.CommandType == IpcCommandType_Request || r.CommandType == IpcCommandType_RequestWithContext) {
                        retval = this->service_object->dispatch(r, c, rawdata_start[2], (u8 *)this->pointer_buffer, this->pointer_buffer_size);
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
                            ipcParse(&out_r);

                            struct {
                                u64 magic;
                                u64 result;
                            } *resp = (decltype(resp))out_r.Raw;

                            retval = resp->result;
                        }
                    }
                }
                
                if (retval == 0xF601) {
                    /* Session close. */
                    rc = retval;
                } else {
                    Log(armGetTls(), 0x100);
                    rc = svcReplyAndReceive(&handle_index, &this->server_handle, 0, this->server_handle, 0);
                    /* Clean up copy handles. */
                    for (unsigned int i = 0; i < out_r.NumHandles; i++) {
                        if (out_r.WasHandleCopied[i]) {
                            svcCloseHandle(out_r.Handles[i]);
                        }
                    }
                }
            }
              
            return rc;
        }
};