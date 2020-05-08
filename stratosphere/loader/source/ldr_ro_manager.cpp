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
#include "ldr_ro_manager.hpp"

namespace ams::ldr::ro {

    namespace {

        /* Convenience definitions. */
        constexpr PinId InvalidPinId = {};
        constexpr size_t ProcessCountMax = 0x40;
        constexpr size_t ModuleCountMax = 0x20;

        /* Types. */
        struct ModuleInfo {
            ldr::ModuleInfo info;
            bool in_use;
        };

        struct ProcessInfo {
            PinId pin_id;
            os::ProcessId process_id;
            ncm::ProgramId program_id;
            cfg::OverrideStatus override_status;
            ncm::ProgramLocation loc;
            ModuleInfo modules[ModuleCountMax];
            bool in_use;
        };

        /* Globals. */
        ProcessInfo g_process_infos[ProcessCountMax];
        u64 g_cur_pin_id = 1;

        /* Helpers. */
        ProcessInfo *GetProcessInfo(PinId pin_id) {
            for (size_t i = 0; i < ProcessCountMax; i++) {
                ProcessInfo *info = &g_process_infos[i];
                if (info->in_use && info->pin_id == pin_id) {
                    return info;
                }
            }
            return nullptr;
        }

        ProcessInfo *GetProcessInfo(os::ProcessId process_id) {
            for (size_t i = 0; i < ProcessCountMax; i++) {
                ProcessInfo *info = &g_process_infos[i];
                if (info->in_use && info->process_id == process_id) {
                    return info;
                }
            }
            return nullptr;
        }

        ProcessInfo *GetFreeProcessInfo() {
            for (size_t i = 0; i < ProcessCountMax; i++) {
                ProcessInfo *info = &g_process_infos[i];
                if (!info->in_use) {
                    return info;
                }
            }
            return nullptr;
        }

    }

    /* RO Manager API. */
    Result PinProgram(PinId *out, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &status) {
        *out = InvalidPinId;
        ProcessInfo *info = GetFreeProcessInfo();
        R_UNLESS(info != nullptr, ldr::ResultTooManyProcesses());

        std::memset(info, 0, sizeof(*info));
        info->pin_id = { g_cur_pin_id++ };
        info->loc = loc;
        info->override_status = status;
        info->in_use = true;
        *out = info->pin_id;
        return ResultSuccess();
    }

    Result UnpinProgram(PinId id) {
        ProcessInfo *info = GetProcessInfo(id);
        R_UNLESS(info != nullptr, ldr::ResultNotPinned());

        info->in_use = false;
        return ResultSuccess();
    }


    Result GetProgramLocationAndStatus(ncm::ProgramLocation *out, cfg::OverrideStatus *out_status, PinId id) {
        ProcessInfo *info = GetProcessInfo(id);
        R_UNLESS(info != nullptr, ldr::ResultNotPinned());

        *out = info->loc;
        *out_status = info->override_status;
        return ResultSuccess();
    }

    Result RegisterProcess(PinId id, os::ProcessId process_id, ncm::ProgramId program_id) {
        ProcessInfo *info = GetProcessInfo(id);
        R_UNLESS(info != nullptr, ldr::ResultNotPinned());

        info->program_id = program_id;
        info->process_id = process_id;
        return ResultSuccess();
    }

    Result RegisterModule(PinId id, const u8 *build_id, uintptr_t address, size_t size) {
        ProcessInfo *info = GetProcessInfo(id);
        R_UNLESS(info != nullptr, ldr::ResultNotPinned());

        /* Nintendo doesn't actually care about successful allocation. */
        for (size_t i = 0; i < ModuleCountMax; i++) {
            ModuleInfo *module = &info->modules[i];
            if (module->in_use) {
                continue;
            }

            std::memcpy(module->info.build_id, build_id, sizeof(module->info.build_id));
            module->info.base_address = address;
            module->info.size = size;
            module->in_use = true;
            break;
        }

        return ResultSuccess();
    }

    Result GetProcessModuleInfo(u32 *out_count, ldr::ModuleInfo *out, size_t max_out_count, os::ProcessId process_id) {
        const ProcessInfo *info = GetProcessInfo(process_id);
        R_UNLESS(info != nullptr, ldr::ResultNotPinned());

        size_t count = 0;
        for (size_t i = 0; i < ModuleCountMax && count < max_out_count; i++) {
            const ModuleInfo *module = &info->modules[i];
            if (!module->in_use) {
                continue;
            }

            out[count++] = module->info;
        }

        *out_count = static_cast<u32>(count);
        return ResultSuccess();
    }

}
