#include "lm_log_server_proxy.hpp"

namespace ams::lm::impl {

    LogServerProxy *GetLogServerProxy() {
        static LogServerProxy log_server_proxy;
        return &log_server_proxy;
    }

}