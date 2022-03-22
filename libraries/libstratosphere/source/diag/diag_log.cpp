/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stratosphere.hpp>
#include "diag_log_impl.hpp"

namespace ams::diag::impl {

    void CallAllLogObserver(const LogMetaData &meta, const LogBody &body);

    namespace {

        struct CallPrintDebugString {
            void operator()(const LogMetaData &meta, const char *msg, size_t size, bool head, bool tail) {
                const LogBody body = {
                    .message = msg,
                    .message_size = size,
                    .is_head = head,
                    .is_tail = tail
                };

                CallAllLogObserver(meta, body);
            }
        };

    }

    void LogImpl(const LogMetaData &meta, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        VLogImpl(meta, fmt, vl);
        va_end(vl);
    }

    void VLogImpl(const LogMetaData &meta, const char *fmt, std::va_list vl) {
        /* Print to stack buffer. */
        char msg_buffer[DebugPrintBufferLength];

        /* TODO: VFormatString using utf-8 printer. */
        const size_t len = util::VSNPrintf(msg_buffer, sizeof(msg_buffer), fmt, vl);

        /* Call log observer. */
        CallPrintDebugString()(meta, msg_buffer, len, true, true);
    }

    void PutImpl(const LogMetaData &meta, const char *msg, size_t msg_size) {
        CallPrintDebugString()(meta, msg, msg_size, true, true);
    }

}
