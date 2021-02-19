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
#include "client/htcs_session.hpp"
#include "client/htcs_virtual_socket_collection.hpp"

namespace ams::htcs {

    namespace {

        constinit bool g_initialized = false;
        constinit bool g_enable_disconnection_emulation = false;

        constinit AllocateFunction g_allocate_function     = nullptr;
        constinit DeallocateFunction g_deallocate_function = nullptr;

        constinit void *g_buffer = nullptr;
        constinit size_t g_buffer_size = 0;

        constinit os::TlsSlot g_tls_slot;

        constinit tma::IHtcsManager *g_manager = nullptr;
        constinit tma::IHtcsManager *g_monitor = nullptr;

        constinit client::VirtualSocketCollection *g_sockets = nullptr;

        void InitializeImpl(void *buffer, size_t buffer_size, int num_sessions) {
            /* Check the session count. */
            AMS_ASSERT(0 < num_sessions && num_sessions <= SessionCountMax);

            /* Initialize the manager and monitor. */
            client::InitializeSessionManager(std::addressof(g_manager), std::addressof(g_monitor));

            /* Register the process. */
            const sf::ClientProcessId process_id{0};
            R_ABORT_UNLESS(g_manager->RegisterProcessId(process_id));
            R_ABORT_UNLESS(g_monitor->MonitorManager(process_id));

            /* Allocate a tls slot for our last error. */
            os::SdkAllocateTlsSlot(std::addressof(g_tls_slot), nullptr);

            /* Setup the virtual socket collection. */
            AMS_ASSERT(buffer != nullptr);
            g_sockets = reinterpret_cast<client::VirtualSocketCollection *>(buffer);
            std::construct_at(g_sockets);
            g_sockets->Init(static_cast<u8 *>(buffer) + sizeof(*g_sockets), buffer_size - sizeof(*g_sockets));

            /* Mark initialized. */
            g_initialized = true;
        }

        void InitializeImpl(AllocateFunction allocate, DeallocateFunction deallocate, int num_sessions, int num_sockets) {
            /* Check the session count. */
            AMS_ASSERT(0 < num_sessions && num_sessions <= SessionCountMax);

            /* Set the allocation functions. */
            g_allocate_function   = allocate;
            g_deallocate_function = deallocate;

            /* Allocate a buffer. */
            g_buffer_size = sizeof(client::VirtualSocketCollection) + client::VirtualSocketCollection::GetWorkingMemorySize(num_sockets);
            g_buffer      = g_allocate_function(g_buffer_size);

            /* Initialize. */
            InitializeImpl(g_buffer, g_buffer_size, num_sessions);
        }

    }

    bool IsInitialized() {
        return g_initialized;
    }

    size_t GetWorkingMemorySize(int num_sockets) {
        AMS_ASSERT(num_sockets <= SocketCountMax);
        return sizeof(client::VirtualSocketCollection) + client::VirtualSocketCollection::GetWorkingMemorySize(num_sockets);
    }

    void Initialize(AllocateFunction allocate, DeallocateFunction deallocate, int num_sessions) {
        /* Check that we're not already initialized. */
        AMS_ASSERT(!IsInitialized());

        /* Configure disconnection emulation. */
        g_enable_disconnection_emulation = true;

        /* Initialize. */
        InitializeImpl(allocate, deallocate, num_sessions, htcs::SocketCountMax);
    }

    void Initialize(void *buffer, size_t buffer_size) {
        /* Check that we're not already initialized. */
        AMS_ASSERT(!IsInitialized());

        /* Configure disconnection emulation. */
        g_enable_disconnection_emulation = true;

        /* Initialize. */
        InitializeImpl(buffer, buffer_size, htcs::SessionCountMax);
    }

    void InitializeForDisableDisconnectionEmulation(AllocateFunction allocate, DeallocateFunction deallocate, int num_sessions) {
        /* Check that we're not already initialized. */
        AMS_ASSERT(!IsInitialized());

        /* Configure disconnection emulation. */
        g_enable_disconnection_emulation = false;

        /* Initialize. */
        InitializeImpl(allocate, deallocate, num_sessions, htcs::SocketCountMax);
    }

    void InitializeForDisableDisconnectionEmulation(void *buffer, size_t buffer_size) {
        /* Check that we're not already initialized. */
        AMS_ASSERT(!IsInitialized());

        /* Configure disconnection emulation. */
        g_enable_disconnection_emulation = false;

        /* Initialize. */
        InitializeImpl(buffer, buffer_size, htcs::SessionCountMax);
    }

    void InitializeForSystem(void *buffer, size_t buffer_size, int num_sessions) {
        /* Check that we're not already initialized. */
        AMS_ASSERT(!IsInitialized());

        /* Configure disconnection emulation. */
        g_enable_disconnection_emulation = true;

        /* Initialize. */
        InitializeImpl(buffer, buffer_size, num_sessions);
    }

    void Finalize() {
        /* Check that we're initialized. */
        AMS_ASSERT(IsInitialized());

        /* Set not initialized. */
        g_initialized = false;

        /* Destroy the virtual socket collection. */
        std::destroy_at(g_sockets);
        g_sockets = nullptr;

        /* Free the buffer, if we have one. */
        if (g_buffer != nullptr) {
            g_deallocate_function(g_buffer, g_buffer_size);
            g_buffer      = nullptr;
            g_buffer_size = 0;
        }

        /* Free the tls slot. */
        os::FreeTlsSlot(g_tls_slot);

        /* Release the manager objects. */
        sf::ReleaseSharedObject(g_manager);
        sf::ReleaseSharedObject(g_monitor);
        g_manager = nullptr;
        g_monitor = nullptr;

        /* Finalize the bsd client sessions. */
        client::FinalizeSessionManager();
    }

}
