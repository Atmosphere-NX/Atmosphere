#include "lm_log_server_proxy.hpp"

namespace ams::lm::impl {

    void LogServerProxy::StartHtcsThread(ThreadFunc htcs_entry) {
        R_ABORT_UNLESS(os::CreateThread(std::addressof(this->htcs_thread), htcs_entry, this, this->htcs_thread_stack, sizeof(this->htcs_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(lm, HtcsConnection)));
        os::SetThreadNamePointer(std::addressof(this->htcs_thread), AMS_GET_SYSTEM_THREAD_NAME(lm, HtcsConnection));
        os::StartThread(std::addressof(this->htcs_thread));
    }

    void LogServerProxy::DisposeHtcsThread() {
        this->finalize_event.Signal();
        /* nn::htcs::Close(this->htcs_client_fd); */
        os::WaitThread(std::addressof(this->htcs_thread));
        os::DestroyThread(std::addressof(this->htcs_thread));
    }

    bool LogServerProxy::LogOverHtcs(const void *log_data, size_t log_size) {
        /* TODO: htcs support */
        /*
        if ((log_size == 0) || (this->htcs_client_fd & 0x80000000)) {
            return true;
        }

        size_t offset = 0;
        auto send_data = reinterpret_cast<const u8*>(log_data);
        while (true) {
            auto sent_size = htcs::Send(this->htcs_client_fd, send_data + offset, log_size - offset, 0);
            if (this->htcs_client_fd & 0x80000000) {
                break;
            }

            offset += sent_size;
            if ((offset >= log_size) || (this->htcs_client_fd & 0x80000000)) {
                return offset == log_size;
            }
        }

        htcs::Shutdown(this->htcs_client_fd, 2);
        os::SleepThread(TimeSpan::FromSeconds(1));
        return false;
        */

        return true;
    }

    LogServerProxy *GetLogServerProxy() {
        static LogServerProxy log_server_proxy;
        return std::addressof(log_server_proxy);
    }

}