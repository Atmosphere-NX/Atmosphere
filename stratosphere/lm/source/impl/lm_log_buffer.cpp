#include "lm_log_buffer.hpp"
#include "lm_sd_card_logging.hpp"
#include "lm_log_server_proxy.hpp"

namespace ams::lm::impl {

    namespace {

        u8 g_log_buffer_data[0x20000];

        u8 g_second_log_buffer_data[0x20000];
        LogBuffer g_second_log_buffer(g_second_log_buffer_data, sizeof(g_second_log_buffer_data), nullptr, nullptr);

        bool g_user_system_clock_modified = false;

        bool FlushImpl(const void *log_data, size_t log_data_size) {
            /* Flush via htcs. */
            auto ok = GetLogServerProxy()->LogOverHtcs(log_data, log_data_size);
            if (ok) {
                /* Save log data on the SD card. */
                ok = GetSdCardLogging()->SaveLog(log_data, log_data_size);
            }
            if (ok) {
                /* Reset this flag for the following log packets. */
                g_user_system_clock_modified = false;
            }
            return ok;
        }

        inline bool ModifyUserSystemClockInfoImpl(impl::LogDataChunkKey chunk_key, u8 *log_payload_data, size_t log_payload_size, time::PosixTime &time_ref) {
            if (chunk_key == impl::LogDataChunkKey_UserSystemClock) {
                /* TODO: properly implement this...? */
                reinterpret_cast<time::PosixTime*>(log_payload_data)->value += time_ref.value;
            }
            return true;
        }

        bool ModifyUserSystemClockInfo(impl::LogPacketHeader *packet_header, void *log_payload_data, size_t log_payload_size, time::PosixTime &time_ref) {
            if (!packet_header->IsHead()) {
                /* Only modify this field on head packets. */
                return true;
            }

            if (log_payload_size == 0) {
                /* Nothing to modify. */
                return true;
            }

            auto payload_start = reinterpret_cast<u8*>(log_payload_data);
            auto payload_cur = payload_start;
            auto payload_end = payload_start + log_payload_size;
            
            /* Read chunk key and size. */
            const auto chunk_key = static_cast<impl::LogDataChunkKey>(PopUleb128(const_cast<const u8**>(std::addressof(payload_cur)), const_cast<const u8*>(payload_end)));
            if (payload_cur >= payload_end) {
                return false;
            }
            const auto chunk_size = static_cast<size_t>(PopUleb128(const_cast<const u8**>(std::addressof(payload_cur)), const_cast<const u8*>(payload_end)));
            if (payload_cur >= payload_end) {
                return false;
            }

            payload_cur += chunk_size;
            return ModifyUserSystemClockInfoImpl(chunk_key, payload_cur, chunk_size, time_ref);
        }

        bool ChangeLogPacketUserSystemClock(void *log_data, size_t log_data_size, time::PosixTime &time_ref) {
            if (log_data_size == 0) {
                /* Nothing to change. */
                return true;
            }
            
            /* Iterate (for potential multiple packets?) and change the UserSystemClock for each value. */
            auto packet_header = reinterpret_cast<impl::LogPacketHeader*>(log_data);
            auto data_start = reinterpret_cast<u8*>(log_data);
            auto data_cur = data_start;
            auto data_end = data_start + log_data_size;
            auto cur_packet_header = packet_header;
            while (static_cast<size_t>(data_end - data_start) >= sizeof(impl::LogPacketHeader)) {
                auto payload_size = cur_packet_header->GetPayloadSize();
                if ((static_cast<size_t>(data_end - data_cur) < payload_size) || !ModifyUserSystemClockInfo(cur_packet_header, data_cur, static_cast<size_t>(payload_size), time_ref)) {
                    break;  
                }

                cur_packet_header = reinterpret_cast<impl::LogPacketHeader*>(data_cur + payload_size);
                if ((data_cur + payload_size) >= data_end) {
                    return true;
                }                
            }

            return false;

        }

        bool LogBuffer_FlushFunction(void *log_data, size_t log_data_size) {
            if (!g_user_system_clock_modified) {
                time::PosixTime posix_time;
                if (R_SUCCEEDED(time::StandardUserSystemClock::GetCurrentTime(std::addressof(posix_time)))) {
                    /* Set a valid time if the log packet has a UserSystemClock chunk/field. */
                    /* TODO: does N check the result? */
                    ChangeLogPacketUserSystemClock(log_data, log_data_size, posix_time);
                }
                g_user_system_clock_modified = true;
            }

            /* Actually flush log data. */
            return FlushImpl(log_data, log_data_size);
        }

    }

    bool LogBuffer::Log(const void *log_data, size_t log_data_size, bool flush) {
        if (log_data_size > 0) {
            std::scoped_lock lk(this->main_target_buffer_lock);
            if (flush) {
                while (true) {
                    if ((this->data_buffer_size - this->main_target_buffer->current_data_size) >= log_data_size) {
                        break;
                    }

                    this->some_count_2++;
                    this->flush_done_cv.Wait(this->main_target_buffer_lock);
                    this->some_count_2--;
                    if (this->some_flag) {
                        if (this->some_count_2 == 0) {
                            this->some_flag = false;
                        }
                        return false;
                    }
                }
            }
            else {
                if ((this->data_buffer_size - this->main_target_buffer->current_data_size) < log_data_size) {
                    return false;
                }
            }

            auto out_buf = reinterpret_cast<u8*>(this->main_target_buffer->data_buffer) + this->main_target_buffer->current_data_size;
            this->main_target_buffer->current_data_size += log_data_size;
            ++this->main_target_buffer->some_count;
            this->main_target_buffer_lock.Unlock();
            std::memcpy(out_buf, log_data, log_data_size);
            this->main_target_buffer_lock.Lock();
            if((--this->main_target_buffer->some_count) == 0) {
                this->flush_ready_cv.Signal();
            }
        }
        return true;
    }

    bool LogBuffer::Flush() {
        std::scoped_lock lk(this->sub_target_buffer_lock);

        auto orig_sub_target_buffer = this->sub_target_buffer;
        if (this->sub_target_buffer->current_data_size == 0) {
            {
                std::scoped_lock lk(this->main_target_buffer_lock);
                do {
                    while (this->main_target_buffer->current_data_size == 0) {
                        this->flush_ready_cv.Wait(this->main_target_buffer_lock);
                    }
                } while(this->main_target_buffer->some_count != 0);
                std::swap(this->main_target_buffer, this->sub_target_buffer);

                this->flush_done_cv.Broadcast();
            }
            if (this->DoFlush(this->sub_target_buffer)) {
                return true;
            }
        }
        
        return this->DoFlush(orig_sub_target_buffer);
    }

    LogBuffer *GetLogBuffer() {
        static LogBuffer log_buffer(g_log_buffer_data, sizeof(g_log_buffer_data), LogBuffer_FlushFunction, std::addressof(g_second_log_buffer));
        return std::addressof(log_buffer);
    }

}