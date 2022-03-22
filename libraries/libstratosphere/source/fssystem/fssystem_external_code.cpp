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

namespace ams::fssystem {

    namespace {

        constexpr inline size_t MaxExternalCodeFileSystem = 0x10;

        util::BoundedMap<ncm::ProgramId, fs::RemoteFileSystem, MaxExternalCodeFileSystem> g_ecs_map;
        util::BoundedMap<ncm::ProgramId, os::NativeHandle,     MaxExternalCodeFileSystem> g_hnd_map;

    }


    fs::fsa::IFileSystem *GetExternalCodeFileSystem(ncm::ProgramId program_id) {
        /* Return a fs from the map if one exists. */
        if (auto *fs = g_ecs_map.Find(program_id); fs != nullptr) {
            return fs;
        }

        /* Otherwise, we may have a handle. */
        if (auto *hnd = g_hnd_map.Find(program_id); hnd != nullptr) {
            /* Create a service using libnx bindings. */
            Service srv;
            serviceCreate(std::addressof(srv), *hnd);
            g_hnd_map.Remove(program_id);

            /* Create a remote filesystem. */
            const FsFileSystem fs = { srv };
            g_ecs_map.Emplace(program_id, fs);

            /* Return the created filesystem. */
            return g_ecs_map.Find(program_id);
        }

        /* Otherwise, we have no filesystem. */
        return nullptr;
    }

    Result CreateExternalCode(os::NativeHandle *out, ncm::ProgramId program_id) {
        /* Create a handle pair. */
        os::NativeHandle server, client;
        R_TRY(svc::CreateSession(std::addressof(server), std::addressof(client), false, 0));

        /* Insert the handle into the map. */
        g_hnd_map.Emplace(program_id, client);

        *out = server;
        return ResultSuccess();
    }

    void DestroyExternalCode(ncm::ProgramId program_id) {
        g_ecs_map.Remove(program_id);

        if (auto *hnd = g_hnd_map.Find(program_id); hnd != nullptr) {
            os::CloseNativeHandle(*hnd);
            g_hnd_map.Remove(program_id);
        }
    }

}
