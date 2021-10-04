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
#include "decodersrv/decodersrv_decoder_server_object.hpp"

namespace ams::capsrv::server {

    namespace {

        bool g_initialized = false;

    }

    Result InitializeForDecoderServer() {
        /* Ensure we initialize only once. */
        AMS_ABORT_UNLESS(!g_initialized);

        /* Clear work memory. */
        std::memset(std::addressof(g_work_memory), 0, sizeof(g_work_memory));

        /* Initialize the decoder server. */
        R_ABORT_UNLESS(g_decoder_control_server_manager.Initialize());

        /* Start the server. */
        g_decoder_control_server_manager.StartServer();

        /* We're initialized. */
        g_initialized = true;
        return ResultSuccess();
    }

    void FinalizeForDecoderServer() {
        /* Ensure we don't finalize when uninitialized. */
        AMS_ABORT_UNLESS(g_initialized);

        /* Finalize the server. */
        g_decoder_control_server_manager.Finalize();

        /* Mark uninitialized. */
        g_initialized = false;
    }

    void DecoderControlServerThreadFunction(void *) {
        /* We need to be initialized. */
        AMS_ABORT_UNLESS(g_initialized);

        /* Run the server. */
        g_decoder_control_server_manager.RunServer();
    }

}
