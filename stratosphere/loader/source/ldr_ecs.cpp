/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <unordered_map>
#include "ldr_ecs.hpp"

namespace sts::ldr::ecs {

    namespace {

        /* Convenience definition. */
        constexpr size_t DeviceNameSizeMax = 0x20;
        constexpr size_t MaxExternalContentSourceCount = 0x10;

        /* Types. */
        class ExternalContentSource {
            NON_COPYABLE(ExternalContentSource);
            NON_MOVEABLE(ExternalContentSource);
            private:
                char device_name[DeviceNameSizeMax];
            public:
                ExternalContentSource(const char *dn){
                    std::strncpy(this->device_name, dn, sizeof(this->device_name) - 1);
                    this->device_name[sizeof(this->device_name) - 1] = '\0';
                }

                ~ExternalContentSource() {
                    fsdevUnmountDevice(this->device_name);
                }

                const char *GetDeviceName() const {
                    return this->device_name;
                }
        };

        /* Global storage. */
        std::unordered_map<u64, ExternalContentSource> g_map;
    }

    /* API. */
    const char *Get(ncm::TitleId title_id) {
        auto it = g_map.find(static_cast<u64>(title_id));
        if (it == g_map.end()) {
            return nullptr;
        }
        return it->second.GetDeviceName();
    }

    Result Set(Handle *out, ncm::TitleId title_id) {
        if (g_map.size() >= MaxExternalContentSourceCount) {
            /* TODO: Is this an appropriate error? */
            return ResultLoaderTooManyArguments;
        }

        /* Clear any sources. */
        R_ASSERT(Clear(title_id));

        /* Generate mountpoint. */
        char device_name[DeviceNameSizeMax];
        std::snprintf(device_name, DeviceNameSizeMax, "ecs-%016lx", static_cast<u64>(title_id));

        /* Create session. */
        AutoHandle server, client;
        R_TRY(svcCreateSession(server.GetPointer(), client.GetPointer(), 0, 0));

        /* Create service. */
        Service srv;
        serviceCreate(&srv, client.Move());
        FsFileSystem fs = { srv };

        /* Try to mount. */
        if (fsdevMountDevice(device_name, fs) == -1) {
            serviceClose(&srv);
            return ResultFsMountNameAlreadyExists;
        }

        /* Add to map. */
        g_map.emplace(static_cast<u64>(title_id), device_name);
        *out = server.Move();
        return ResultSuccess;
    }

    Result Clear(ncm::TitleId title_id) {
        /* Delete if present. */
        g_map.erase(static_cast<u64>(title_id));
        return ResultSuccess;
    }

}
