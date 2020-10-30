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
#include <stratosphere.hpp>

namespace ams::ddsf {

    namespace {

        constinit ams::MemoryResource *g_memory_resource                          = nullptr;
        constinit ams::MemoryResource *g_device_code_entry_holder_memory_resource = nullptr;

    }

    void SetMemoryResource(ams::MemoryResource *mr) {
        AMS_ASSERT(g_memory_resource == nullptr);
        g_memory_resource = mr;
        AMS_ASSERT(g_memory_resource != nullptr);
    }

    ams::MemoryResource *GetMemoryResource() {
        AMS_ASSERT(g_memory_resource != nullptr);
        return g_memory_resource;
    }

    void SetDeviceCodeEntryHolderMemoryResource(ams::MemoryResource *mr) {
        AMS_ASSERT(g_device_code_entry_holder_memory_resource == nullptr);
        g_device_code_entry_holder_memory_resource = mr;
        AMS_ASSERT(g_device_code_entry_holder_memory_resource != nullptr);
    }

    ams::MemoryResource *GetDeviceCodeEntryHolderMemoryResource() {
        AMS_ASSERT(g_device_code_entry_holder_memory_resource != nullptr);
        return g_device_code_entry_holder_memory_resource;
    }

}
