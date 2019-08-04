#include <stratosphere/sm.hpp>

#include "impl/ncm_content_manager.hpp"
#include "debug.hpp"
#include "ncm_fs.hpp"

namespace sts::debug {

    #define IRAM_BASE 0x40000000ull
    #define IRAM_SIZE 0x40000

    #define IRAM_PAYLOAD_MAX_SIZE 0x2F000
    #define IRAM_PAYLOAD_BASE 0x40010000ull

    #define IRAM_SAFE_START 0x40038000ull
    #define IRAM_SAFE_END 0x4003D000ull
    #define IRAM_SAFE_SIZE IRAM_SAFE_END - IRAM_SAFE_START

    namespace {

        HosMutex g_log_mutex;

    }

    size_t g_curr_log_offset = 0;
    size_t g_log_skip = 4;

    char __attribute__ ((aligned (0x1000))) g_work_page[0x1000];

    void clear_iram(void) {
        /* Fill with null*/
        memset(g_work_page, 0, sizeof(g_work_page));

        /* Overwrite all of our log's IRAM with 0s. */
        for (size_t ofs = 0; ofs < IRAM_SAFE_SIZE; ofs += sizeof(g_work_page)) {
            CopyToIram(IRAM_SAFE_START + ofs, g_work_page, sizeof(g_work_page));
        }
    }

    void clear_log(void) {
        std::scoped_lock lk(g_log_mutex);

        clear_iram();
    }

    Result Initialize() {
        clear_log();
        return ResultSuccess;
    }

    void DebugLog(const char* format, ...) {
        std::scoped_lock lk(g_log_mutex);
        memset(g_work_page, 0, sizeof(g_work_page));
        
        /* Format a string with our arguments. */
        va_list args;
        va_start(args, format);
        vsnprintf(g_work_page, 0x1000, format, args);
        va_end(args);

        size_t msg_len = strnlen(g_work_page, 0x1000);

        /* Nothing to log. */
        if (msg_len == 0) {
            return;
        }

        /* Attempt to put some of our new string in the previous 4 bytes. */
        if (g_curr_log_offset >= 4) {
            char __attribute__ ((aligned (0x4))) prev_4[4] = {0};
            CopyFromIram(prev_4, IRAM_SAFE_START+g_curr_log_offset-4, 4);
            size_t prev_4_size = strnlen(prev_4, 4);

            /* Do we have room to put some of our new string in the old one? */
            if (prev_4_size < 4) {
                size_t spare_4_space = 4 - prev_4_size;
                memcpy(prev_4 + prev_4_size, g_work_page, spare_4_space);

                /* Copy the previous 4 bytes back. */
                CopyToIram(IRAM_SAFE_START+g_curr_log_offset-4, prev_4, 4);
                memmove(g_work_page, g_work_page + spare_4_space, 0x1000-spare_4_space);
                /* Update our size again. */
                msg_len = strnlen(g_work_page, 0x1000);
            }
        }

        size_t msg_len_aligned = (msg_len + 3) & ~3;

        if (g_curr_log_offset + msg_len_aligned > IRAM_SAFE_SIZE) {
            if (g_log_skip == 0) {
                /* Log is full. Reboot to RCM to read it. */
                RebootToRcm();
                return;
            }

            g_log_skip--;
            clear_iram();
            g_curr_log_offset = 0;
        }

        /* Fill remainder with 0s. */
        if (msg_len_aligned > msg_len) {
            memset(g_work_page + msg_len, '\0', msg_len_aligned - msg_len);
        }

        uintptr_t iram_out_start = IRAM_SAFE_START+g_curr_log_offset;
        uintptr_t iram_next_page = (iram_out_start + 0x1000-1) & ~(0x1000-1);

        /* We are copying across two pages */
        if (iram_out_start + msg_len_aligned > iram_next_page) {
            size_t first_page_size = iram_next_page - iram_out_start;
            CopyToIram(iram_out_start, g_work_page, first_page_size);
            CopyToIram(iram_next_page, g_work_page+first_page_size, msg_len_aligned-first_page_size);
        } else {
            CopyToIram(iram_out_start, g_work_page, msg_len_aligned);
        }

        g_curr_log_offset += msg_len_aligned;
    }

    void LogBytes(const void* buf, size_t len) {
        if (buf == NULL || len == 0) {
            return;
        }

        const u8* bytes = static_cast<const u8*>(buf);
        int count = 0;
        DebugLog("\n\n00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
        DebugLog("-----------------------------------------------\n");

        for (size_t i = 0; i < len; i++) {
            DebugLog("%02x ", bytes[i]);
            count++;
            if ((count % 16) == 0) {
                DebugLog("\n");
            }
        }

        DebugLog("\n");
    }
}

