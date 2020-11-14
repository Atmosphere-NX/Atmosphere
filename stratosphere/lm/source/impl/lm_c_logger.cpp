#include "lm_c_logger.hpp"
#include "lm_a_logger.hpp"

namespace ams::lm::impl {

    u8 g_logger_data_buffer[0x20000];

    namespace {

        bool g_some_flag = false;

        bool SendDataOverHtcs() {
            return false;
        }

        inline bool LogToSdCardAndHtcs(void *data, size_t size) {
            return SendDataOverHtcs() && GetALogger()->SaveLog(data, size);
        }

        bool LogFunction(void *data, size_t size) {
            if (!g_some_flag) {
                time::PosixTime posix_time;
                if (R_SUCCEEDED(time::StandardUserSystemClock::GetCurrentTime(&posix_time))) {
                    /* TODO: what time conversions are done here? */
                    /* TODO: what's done here? */
                }
                g_some_flag = true;
            }
            return LogToSdCardAndHtcs(data, size);
        }

    }

    bool CLogger::Log(const void *data, size_t data_size) {
        if (data_size == 0) {
            return true;
        }

        this->sdk_mutex_1.Lock();
        if (this->CopyLogData(data, data_size)) {
            return true;
        }
        auto tmp_flag = this->unk_flag;

        while(true) {
            this->unk_flag++;
            this->condvar_1.Wait(this->sdk_mutex_1);
            auto flag = this->some_flag;
            this->unk_flag--;
            if (flag) {
                break;
            }
            if (this->CopyLogData(data, data_size)) {
                return true;
            }
        }
        if (tmp_flag == 0) {
            this->some_flag = false;
        }
        this->sdk_mutex_1.Unlock();
        return false;
    }

    bool CLogger::SomeMemberFn() {
        std::scoped_lock lk(this->sdk_mutex_2);
        if (this->cur_offset == 0) {
            this->sdk_mutex_1.Lock();
            do {
                do {
                    this->condvar_2.Wait(this->sdk_mutex_1);
                } while(this->cur_offset == 0);
            } while(this->some_refcount != 0);
        }
    }

    CLogger *GetCLogger() {
        static CLogger c_logger(g_logger_data_buffer, sizeof(g_logger_data_buffer), LogFunction);
        return &c_logger;
    }

}