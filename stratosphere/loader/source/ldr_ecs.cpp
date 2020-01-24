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
#include "ldr_ecs.hpp"

namespace ams::ldr::ecs {

    namespace {

        /* Convenience definition. */
        constexpr size_t DeviceNameSizeMax = 0x20;
        constexpr size_t MaxExternalContentSourceCount = 0x10;

        /* Types. */
        class ExternalContentSource {
            NON_COPYABLE(ExternalContentSource);
            NON_MOVEABLE(ExternalContentSource);
            private:
                bool has_mounted = false;
                char device_name[DeviceNameSizeMax];
                os::ManagedHandle client;

                Result Mount() {
                    /* Create service. */
                    Service srv;
                    serviceCreate(&srv, client.Move());
                    FsFileSystem fs = { srv };
                    auto fs_guard = SCOPE_GUARD { fsFsClose(&fs); };

                    /* Try to mount. */
                    R_UNLESS(fsdevMountDevice(device_name, fs) >= 0, fs::ResultMountNameAlreadyExists());
                    fs_guard.Cancel();

                    this->has_mounted = true;
                    return ResultSuccess();
                }
            public:
                ExternalContentSource(const char *dn, os::ManagedHandle client) : client(std::move(client)) {
                    std::strncpy(this->device_name, dn, sizeof(this->device_name));
                    this->device_name[sizeof(this->device_name) - 1] = '\0';
                }

                Result EnsureMounted() {
                    if (!this->has_mounted) {
                        return Mount();
                    }
                    return ResultSuccess();
                }

                ~ExternalContentSource() {
                    if (this->has_mounted) {
                        fsdevUnmountDevice(this->device_name);
                    }
                }

                const char *GetDeviceName() const {
                    return this->device_name;
                }
        };

        /* Global storage. */
        std::unordered_map<u64, ExternalContentSource> g_map;
    }

    /* API. */
    const char *Get(ncm::ProgramId program_id) {
        auto it = g_map.find(static_cast<u64>(program_id));
        if (it == g_map.end()) {
            return nullptr;
        }
        if (R_FAILED(it->second.EnsureMounted())) {
            g_map.erase(it);
            return nullptr;
        }
        return it->second.GetDeviceName();
    }

    Result Set(Handle *out, ncm::ProgramId program_id) {
        /* TODO: Is this an appropriate error? */
        R_UNLESS(g_map.size() < MaxExternalContentSourceCount, ldr::ResultTooManyArguments());

        /* Clear any sources. */
        R_ASSERT(Clear(program_id));

        /* Generate mountpoint. */
        char device_name[DeviceNameSizeMax];
        std::snprintf(device_name, DeviceNameSizeMax, "ecs-%016lx", static_cast<u64>(program_id));

        /* Create session. */
        os::ManagedHandle server, client;
        R_TRY(svcCreateSession(server.GetPointer(), client.GetPointer(), 0, 0));

        /* Do not create service yet. */
        /* Defer until we've handed the server side back so we don't deadlock on querying pointer buffer size. */

        /* Add to map. */
        g_map.emplace(std::piecewise_construct,
                      std::make_tuple(static_cast<u64>(program_id)),
                      std::make_tuple(device_name, std::move(client)));
        *out = server.Move();
        return ResultSuccess();
    }

    Result Clear(ncm::ProgramId program_id) {
        /* Delete if present. */
        g_map.erase(static_cast<u64>(program_id));
        return ResultSuccess();
    }

}
