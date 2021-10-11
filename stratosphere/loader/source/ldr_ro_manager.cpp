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
#include "ldr_ro_manager.hpp"

namespace ams::ldr {

    bool RoManager::Allocate(PinId *out, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &status) {
        /* Ensure that output pin id is set. */
        *out = InvalidPinId;

        /* Allocate a process info. */
        auto *found = this->AllocateProcessInfo();
        if (found == nullptr) {
            return false;
        }

        /* Setup the process info. */
        std::memset(found, 0, sizeof(*found));
        found->pin_id           = { ++m_pin_id };
        found->program_location = loc;
        found->override_status  = status;
        found->in_use           = true;

        /* Set the output pin id. */
        *out = found->pin_id;
        return true;
    }

    bool RoManager::Free(PinId pin_id) {
        /* Find the process. */
        auto *found = this->FindProcessInfo(pin_id);
        if (found == nullptr) {
            return false;
        }

        /* Set the process as not in use. */
        found->in_use = false;

        /* Set all the process's nsos as not in use. */
        for (auto i = 0; i < NsoCount; ++i) {
            found->nso_infos[i].in_use = false;
        }

        return true;
    }

    void RoManager::RegisterProcess(PinId pin_id, os::ProcessId process_id, ncm::ProgramId program_id, bool is_64_bit_address_space) {
        /* Find the process. */
        auto *found = this->FindProcessInfo(pin_id);
        if (found == nullptr) {
            return;
        }

        /* Set the process id and program id. */
        found->process_id = process_id;
        found->program_id = program_id;
        AMS_UNUSED(is_64_bit_address_space);
    }

    bool RoManager::GetProgramLocationAndStatus(ncm::ProgramLocation *out, cfg::OverrideStatus *out_status, PinId pin_id) {
        /* Find the process. */
        auto *found = this->FindProcessInfo(pin_id);
        if (found == nullptr) {
            return false;
        }

        /* Set the output location/status. */
        *out        = found->program_location;
        *out_status = found->override_status;
        return true;
    }

    void RoManager::AddNso(PinId pin_id, const u8 *module_id, u64 address, u64 size) {
        /* Find the process. */
        auto *found = this->FindProcessInfo(pin_id);
        if (found == nullptr) {
            return;
        }

        /* Allocate an nso. */
        auto *info = this->AllocateNsoInfo(found);
        if (info == nullptr) {
            return;
        }

        /* Copy the information into the nso info. */
        std::memcpy(info->module_info.module_id, module_id, sizeof(info->module_info.module_id));
        info->module_info.address = address;
        info->module_info.size    = size;
        info->in_use              = true;
    }

    bool RoManager::GetProcessModuleInfo(u32 *out_count, ModuleInfo *out, size_t max_out_count, os::ProcessId process_id) {
        /* Find the process. */
        auto *found = this->FindProcessInfo(process_id);
        if (found == nullptr) {
            return false;
        }

        /* Copy allocated nso module infos. */
        size_t count = 0;
        for (auto i = 0; i < NsoCount && count < max_out_count; ++i) {
            /* Skip unallocated nsos. */
            if (!found->nso_infos[i].in_use) {
                continue;
            }

            /* Copy out the module info. */
            out[count++] = found->nso_infos[i].module_info;
        }

        /* Set the output count. */
        *out_count = count;
        return true;
    }

    RoManager::ProcessInfo *RoManager::AllocateProcessInfo() {
        for (auto i = 0; i < ProcessCount; ++i) {
            if (!m_processes[i].in_use) {
                return m_processes + i;
            }
        }

        return nullptr;
    }

    RoManager::ProcessInfo *RoManager::FindProcessInfo(PinId pin_id) {
        for (auto i = 0; i < ProcessCount; ++i) {
            if (m_processes[i].in_use && m_processes[i].pin_id == pin_id) {
                return m_processes + i;
            }
        }

        return nullptr;
    }

    RoManager::ProcessInfo *RoManager::FindProcessInfo(os::ProcessId process_id) {
        for (auto i = 0; i < ProcessCount; ++i) {
            if (m_processes[i].in_use && m_processes[i].process_id == process_id) {
                return m_processes + i;
            }
        }

        return nullptr;
    }

    RoManager::ProcessInfo *RoManager::FindProcessInfo(ncm::ProgramId program_id) {
        for (auto i = 0; i < ProcessCount; ++i) {
            if (m_processes[i].in_use && m_processes[i].program_id == program_id) {
                return m_processes + i;
            }
        }

        return nullptr;
    }

    RoManager::NsoInfo *RoManager::AllocateNsoInfo(ProcessInfo *info) {
        for (auto i = 0; i < NsoCount; ++i) {
            if (!info->nso_infos[i].in_use) {
                return info->nso_infos + i;
            }
        }

        return nullptr;
    }

}
