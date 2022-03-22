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
#include "tio_file_server.hpp"
#include "tio_file_server_packet.hpp"
#include "tio_file_server_htcs_server.hpp"
#include "tio_file_server_processor.hpp"
#include "tio_sd_card_observer.hpp"

namespace ams::tio {

    namespace {

        constexpr inline auto NumDispatchThreads     = 2;
        constexpr inline auto DispatchThreadPriority = 21;
        constexpr inline size_t RequestBufferSize   = 1_MB + util::AlignUp(0x40 + fs::EntryNameLengthMax, 1_KB);

        struct FileServerRequest {
            int socket;
            FileServerRequestHeader header;
            u8 body[RequestBufferSize];
        };

        constexpr const char HtcsPortName[] = "iywys@$TioServer_FileServer";

        alignas(os::ThreadStackAlignment) u8 g_server_stack[os::MemoryPageSize];
        alignas(os::ThreadStackAlignment) u8 g_observer_stack[os::MemoryPageSize];
        alignas(os::ThreadStackAlignment) u8 g_dispatch_stacks[NumDispatchThreads][os::MemoryPageSize];

        constinit FileServerHtcsServer g_file_server_htcs_server;
        constinit FileServerProcessor g_file_server_processor(g_file_server_htcs_server);
        constinit SdCardObserver g_sd_card_observer;

        constinit os::ThreadType g_file_server_dispatch_threads[NumDispatchThreads];

        constinit FileServerRequest g_requests[NumDispatchThreads];

        constinit os::MessageQueueType g_free_mq;
        constinit os::MessageQueueType g_dispatch_mq;

        constinit uintptr_t g_free_mq_storage[NumDispatchThreads];
        constinit uintptr_t g_dispatch_mq_storage[NumDispatchThreads];

        void OnSdCardInsertionChanged(bool inserted) {
            g_file_server_processor.SetInserted(inserted);
        }

        void OnFileServerHtcsSocketAccepted(int fd) {
            /* Service requests, while we can. */
            while (true) {
                /* Receive a free request. */
                uintptr_t request_address;
                os::ReceiveMessageQueue(std::addressof(request_address), std::addressof(g_free_mq));

                /* Ensure we manage our request properly. */
                auto req_guard = SCOPE_GUARD { os::SendMessageQueue(std::addressof(g_free_mq), request_address); };

                /* Receive the request header. */
                FileServerRequest *request = reinterpret_cast<FileServerRequest *>(request_address);
                if (htcs::Recv(fd, std::addressof(request->header), sizeof(request->header), htcs::HTCS_MSG_WAITALL) != sizeof(request->header)) {
                    break;
                }

                /* Receive the request body, if necessary. */
                if (request->header.body_size > 0) {
                    if (htcs::Recv(fd, request->body, request->header.body_size, htcs::HTCS_MSG_WAITALL) != request->header.body_size) {
                        break;
                    }
                }

                /* Dispatch the request. */
                req_guard.Cancel();
                request->socket = fd;
                os::SendMessageQueue(std::addressof(g_dispatch_mq), request_address);
            }

            /* Our socket is no longer making requests, so close it. */
            htcs::Close(fd);

            /* Clean up any server resources. */
            g_file_server_processor.Unmount();
        }

        void FileServerDispatchThreadFunction(void *) {
            while (true) {
                /* Receive a request. */
                uintptr_t request_address;
                os::ReceiveMessageQueue(std::addressof(request_address), std::addressof(g_dispatch_mq));

                /* Process the request. */
                FileServerRequest *request = reinterpret_cast<FileServerRequest *>(request_address);
                if (!g_file_server_processor.ProcessRequest(std::addressof(request->header), request->body, request->socket)) {
                    htcs::Close(request->socket);
                }

                /* Free the request. */
                os::SendMessageQueue(std::addressof(g_free_mq), request_address);
            }
        }

    }

    void InitializeFileServer() {
        /* Initialize the htcs server. */
        g_file_server_htcs_server.Initialize(HtcsPortName, g_server_stack, sizeof(g_server_stack), OnFileServerHtcsSocketAccepted);

        /* Initialize SD card observer. */
        g_sd_card_observer.Initialize(g_observer_stack, sizeof(g_observer_stack));
        g_sd_card_observer.SetCallback(OnSdCardInsertionChanged);

        /* Initialize the command processor. */
        g_file_server_processor.SetInserted(g_sd_card_observer.IsSdCardInserted());
        g_file_server_processor.SetRequestBufferSize(RequestBufferSize);

        /* Initialize the dispatch message queues. */
        os::InitializeMessageQueue(std::addressof(g_free_mq), g_free_mq_storage, util::size(g_free_mq_storage));
        os::InitializeMessageQueue(std::addressof(g_dispatch_mq), g_dispatch_mq_storage, util::size(g_dispatch_mq_storage));

        /* Begin with all requests free. */
        for (auto i = 0; i < NumDispatchThreads; ++i) {
            os::SendMessageQueue(std::addressof(g_free_mq), reinterpret_cast<uintptr_t>(g_requests + i));
        }

        /* Initialize the dispatch threads. */
        /* NOTE: Nintendo does not name these threads. */
        for (auto i = 0; i < NumDispatchThreads; ++i) {
            R_ABORT_UNLESS(os::CreateThread(g_file_server_dispatch_threads + i, FileServerDispatchThreadFunction, nullptr, g_dispatch_stacks + i, sizeof(g_dispatch_stacks[i]), DispatchThreadPriority));
        }
    }

    void StartFileServer() {
        /* Start the htcs server. */
        g_file_server_htcs_server.Start();

        /* Start the dispatch threads. */
        for (auto i = 0; i < NumDispatchThreads; ++i) {
            os::StartThread(g_file_server_dispatch_threads + i);
        }
    }

    void WaitFileServer() {
        /* Wait for the htcs server to finish. */
        g_file_server_htcs_server.Wait();
    }

}