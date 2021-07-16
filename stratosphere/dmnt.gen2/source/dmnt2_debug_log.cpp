/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "dmnt2_debug_log.hpp"

#define AMS_DMNT2_ENABLE_HTCS_DEBUG_LOG

#if defined(AMS_DMNT2_ENABLE_HTCS_DEBUG_LOG)

namespace ams::dmnt {

    namespace {

        constexpr size_t NumLogBuffers = 16;
        constexpr size_t LogBufferSize = 0x100;

        /* TODO: This shouldn't be so high priority. Come up with a better number? */
        constexpr inline auto LogThreadPriority = 10;

        constinit os::MessageQueueType g_buf_mq;
        constinit os::MessageQueueType g_req_mq;

        alignas(os::ThreadStackAlignment) u8 g_log_thread_stack[os::MemoryPageSize];

        constinit char g_log_buffers[NumLogBuffers][LogBufferSize];

        constinit uintptr_t g_buf_mq_storage[NumLogBuffers];
        constinit uintptr_t g_req_mq_storage[NumLogBuffers];

        constinit os::ThreadType g_log_thread;

        util::optional<char *> GetLogBuffer() {
            /* Try to get a log buffer. */
            uintptr_t address;
            if (os::TryReceiveMessageQueue(std::addressof(address), std::addressof(g_buf_mq))) {
                return reinterpret_cast<char *>(address);
            } else {
                return util::nullopt;
            }
        }

        /* TODO: This is a hack because we don't have a proper LogManager system module. */
        /*       Eventually, this should be refactored to use normal logging. */
        void DebugLogThreadFunction(void *) {
            /* Loop forever, servicing our log server. */
            while (true) {
                /* Get a socket. */
                int fd;
                while ((fd = htcs::Socket()) == -1) {
                    os::SleepThread(TimeSpan::FromSeconds(1));
                }

                /* Ensure we cleanup the socket when we're done with it. */
                ON_SCOPE_EXIT {
                    htcs::Close(fd);
                    os::SleepThread(TimeSpan::FromSeconds(1));
                };

                /* Create a sock addr for our server. */
                htcs::SockAddrHtcs addr;
                addr.family    = htcs::HTCS_AF_HTCS;
                addr.peer_name = htcs::GetPeerNameAny();
                std::strcpy(addr.port_name.name, "iywys@$dmnt2_log");

                /* Bind. */
                if (htcs::Bind(fd, std::addressof(addr)) == -1) {
                    continue;
                }

                /* Listen on our port. */
                while (htcs::Listen(fd, 0) == 0) {
                    /* Continue accepting clients, so long as we can. */
                    int client_fd;
                    while (true) {
                        /* Try to accept a client. */
                        if (client_fd = htcs::Accept(fd, std::addressof(addr)); client_fd < 0) {
                            break;
                        }

                        /* Handle the client. */
                        while (true) {
                            /* Receive a log request. */
                            uintptr_t log_buffer_address;
                            os::ReceiveMessageQueue(std::addressof(log_buffer_address), std::addressof(g_req_mq));

                            /* Ensure we manage our request properly. */
                            ON_SCOPE_EXIT { os::SendMessageQueue(std::addressof(g_buf_mq), log_buffer_address); };

                            /* Send the data. */
                            const char * const log_buffer = reinterpret_cast<const char *>(log_buffer_address);
                            if (htcs::Send(client_fd, log_buffer, std::strlen(log_buffer), 0) < 0) {
                                break;
                            }
                        }

                        /* Close the client socket. */
                        htcs::Close(client_fd);
                    }
                }
            }
        }


    }


    void InitializeDebugLog() {
        /* Initialize logging message queues. */
        os::InitializeMessageQueue(std::addressof(g_buf_mq), g_buf_mq_storage, NumLogBuffers);
        os::InitializeMessageQueue(std::addressof(g_req_mq), g_req_mq_storage, NumLogBuffers);

        /* Initially make all log buffers available. */
        for (size_t i = 0; i < NumLogBuffers; ++i) {
            os::SendMessageQueue(std::addressof(g_buf_mq), reinterpret_cast<uintptr_t>(g_log_buffers[i]));
        }

        /* Create and start the debug log thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_log_thread), DebugLogThreadFunction, nullptr, g_log_thread_stack, sizeof(g_log_thread_stack), LogThreadPriority));

        os::StartThread(std::addressof(g_log_thread));
    }

    void DebugLog(const char *prefix, const char *fmt, ...) {
        /* Try to get a log buffer. */
        const auto log_buffer_holder = GetLogBuffer();
        if (!log_buffer_holder) {
            return;
        }


        /* Format into the log buffer. */
        char * const log_buffer = *log_buffer_holder;
        {
            const auto prefix_len = std::strlen(prefix);
            std::memcpy(log_buffer, prefix, prefix_len);

            {
                std::va_list vl;
                va_start(vl, fmt);
                util::VSNPrintf(log_buffer + prefix_len, LogBufferSize - prefix_len, fmt, vl);
                va_end(vl);
            }
        }

        /* Request to log. */
        os::SendMessageQueue(std::addressof(g_req_mq), reinterpret_cast<uintptr_t>(log_buffer));
    }

}

#else

namespace ams::dmnt {


    void InitializeDebugLog() {
        /* Do nothing. */
    }

    void DebugLog(const char *prefix, const char *fmt, ...) {
        /* Do nothing. */
    }

}


#endif
