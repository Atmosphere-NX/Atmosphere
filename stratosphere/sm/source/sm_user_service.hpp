#pragma once
#include <switch.h>
#include <stratosphere/iserviceobject.hpp>

enum UserServiceCmd {
    User_Cmd_Initialize = 0,
    User_Cmd_GetService = 1,
    User_Cmd_RegisterService = 2,
    User_Cmd_UnregisterService = 3,
    
    User_Cmd_AtmosphereInstallMitm = 65000,
    User_Cmd_AtmosphereUninstallMitm = 65001,
    User_Cmd_AtmosphereAssociatePidTidForMitm = 65002
};

class UserService final : public IServiceObject {
    u64 pid = U64_MAX;
    bool has_initialized = false;
    u64 deferred_service = 0;
    
    public:
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) override;
        Result handle_deferred() override;
        
        UserService *clone() override {
            auto new_srv = new UserService();
            new_srv->pid = pid;
            new_srv->has_initialized = has_initialized;
            new_srv->deferred_service = deferred_service;
            return new_srv;
        }
        
    private:
        /* Actual commands. */
        std::tuple<Result> initialize(PidDescriptor pid);
        std::tuple<Result, MovedHandle> get_service(u64 service);
        std::tuple<Result, MovedHandle> deferred_get_service(u64 service);
        std::tuple<Result, MovedHandle> register_service(u64 service, u8 is_light, u32 max_sessions);
        std::tuple<Result> unregister_service(u64 service);
        
        /* Atmosphere commands. */
        std::tuple<Result, MovedHandle, MovedHandle> install_mitm(u64 service);
        std::tuple<Result> uninstall_mitm(u64 service);
        std::tuple<Result> associate_pid_tid_for_mitm(u64 pid, u64 tid);
};
