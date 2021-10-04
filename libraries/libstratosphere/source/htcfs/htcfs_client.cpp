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
#include "htcfs_client.hpp"

namespace ams::htcfs {

    namespace {

        constinit util::TypedStorage<Client> g_client_storage;
        [[maybe_unused]] constinit bool g_initialized;

    }

    void InitializeClient(htclow::HtclowManager *manager) {
        AMS_ASSERT(!g_initialized);

        util::ConstructAt(g_client_storage, manager);
    }

    void FinalizeClient() {
        AMS_ASSERT(g_initialized);

        util::DestroyAt(g_client_storage);
    }

    Client &GetClient() {
        AMS_ASSERT(g_initialized);

        return GetReference(g_client_storage);
    }

}
