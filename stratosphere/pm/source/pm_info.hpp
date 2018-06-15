#pragma once
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/iserviceobject.hpp>

enum InformationCmd {
    Information_Cmd_GetTitleId = 0,
};

class InformationService final : public IServiceObject {
    public:
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) override;
        Result handle_deferred() override;
        
        InformationService *clone() override {
            return new InformationService(*this);
        }
        
    private:
        /* Actual commands. */
        std::tuple<Result, u64> get_title_id(u64 pid);
};
