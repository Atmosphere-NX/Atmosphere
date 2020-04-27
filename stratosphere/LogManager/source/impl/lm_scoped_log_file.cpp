#include "lm_scoped_log_file.hpp"

namespace ams::lm::impl {

    namespace {

        os::Mutex g_log_lock(true);

        /* Longest strings will be slightly bigger than 0x100 bytes, so this will be enough. */
        char g_log_format_buffer[0x400];

    }

    void ScopedLogFile::WriteString(const char *str) {
        std::scoped_lock lk(g_log_lock);

        this->Write(str, std::strlen(str));
    }

    void ScopedLogFile::WriteFormat(const char *fmt, ...) {
        std::scoped_lock lk(g_log_lock);

        /* Format to the buffer. */
        {
            std::va_list vl;
            va_start(vl, fmt);
            std::vsnprintf(g_log_format_buffer, sizeof(g_log_format_buffer), fmt, vl);
            va_end(vl);
        }

        /* Write data. */
        this->WriteString(g_log_format_buffer);
    }

    void ScopedLogFile::Write(const void *data, size_t size) {
        std::scoped_lock lk(g_log_lock);

        /* If we're not open, we can't write. */
        if (!this->IsOpen()) {
            return;
        }

        /* Advance, if we write successfully. */
        if (R_SUCCEEDED(fs::WriteFile(this->file, this->offset, data, size, fs::WriteOption::Flush))) {
            this->offset += size;
        }
    }

}