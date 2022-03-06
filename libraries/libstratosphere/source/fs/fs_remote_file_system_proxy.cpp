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
#include "fs_remote_file_system_proxy.hpp"
#include "fs_remote_file_system_proxy_for_loader.hpp"

#if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
extern "C" {

    extern u32 __nx_fs_num_sessions;

}
#endif

namespace ams::fs {

    #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
    RemoteFileSystemProxy::RemoteFileSystemProxy(int session_count) {
        /* Ensure we can connect to sm. */
        R_ABORT_UNLESS(sm::Initialize());
        ON_SCOPE_EXIT { R_ABORT_UNLESS(sm::Finalize()); };

        /* Initialize libnx. */
        __nx_fs_num_sessions = static_cast<u32>(session_count);
        R_ABORT_UNLESS(::fsInitialize());
    }

    RemoteFileSystemProxy::~RemoteFileSystemProxy() {
        ::fsExit();
    }

    RemoteFileSystemProxyForLoader::RemoteFileSystemProxyForLoader() {
        /* Ensure we can connect to sm. */
        R_ABORT_UNLESS(sm::Initialize());
        ON_SCOPE_EXIT { R_ABORT_UNLESS(sm::Finalize()); };

        R_ABORT_UNLESS(::fsldrInitialize());
    }

    RemoteFileSystemProxyForLoader::~RemoteFileSystemProxyForLoader() {
        ::fsldrExit();
    }
    #endif

}
