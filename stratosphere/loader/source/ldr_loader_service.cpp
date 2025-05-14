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
#include "ldr_argument_store.hpp"
#include "ldr_content_management.hpp"
#include "ldr_development_manager.hpp"
#include "ldr_process_creation.hpp"
#include "ldr_launch_record.hpp"
#include "ldr_loader_service.hpp"

namespace ams::ldr {

    namespace {

        constinit ArgumentStore g_argument_store;

        bool IsValidProgramAttributes(const ldr::ProgramAttributes &attrs) {
            /* Check that the attributes are valid; on NX, this means Platform = NX + no content attrs. */
            if (attrs.platform != ncm::ContentMetaPlatform::Nx) {
                return false;
            }
            if (attrs.content_attributes != fs::ContentAttributes_None) {
                return false;
            }

            return true;
        }

    }


    Result LoaderService::CreateProcess(os::NativeHandle *out, PinId pin_id, u32 flags, os::NativeHandle resource_limit, const ProgramAttributes &attrs) {
        /* Check that the program attributes are valid. */
        R_UNLESS(IsValidProgramAttributes(attrs), ldr::ResultInvalidProgramAttributes());

        /* Get the location and override status. */
        ncm::ProgramLocation loc;
        cfg::OverrideStatus override_status;
        R_TRY(ldr::GetProgramLocationAndOverrideStatusFromPinId(std::addressof(loc), std::addressof(override_status), pin_id));

        /* Get the program path. */
        char path[fs::EntryNameLengthMax];
        R_TRY(GetProgramPath(path, sizeof(path), loc, attrs));
        path[sizeof(path) - 1] = '\x00';

        /* Create the process. */
        R_RETURN(ldr::CreateProcess(out, pin_id, loc, override_status, path, g_argument_store.Get(loc.program_id), flags, resource_limit, attrs));
    }

    Result LoaderService::GetProgramInfo(ProgramInfo *out, cfg::OverrideStatus *out_status, const ncm::ProgramLocation &loc, const ProgramAttributes &attrs) {
        /* Check that the program attributes are valid. */
        R_UNLESS(IsValidProgramAttributes(attrs), ldr::ResultInvalidProgramAttributes());

        /* Zero output. */
        std::memset(out, 0, sizeof(*out));

        /* Get the program path. */
        char path[fs::EntryNameLengthMax];
        R_TRY(GetProgramPath(path, sizeof(path), loc, attrs));
        path[sizeof(path) - 1] = '\x00';

        /* Get the program info. */
        cfg::OverrideStatus status;
        R_TRY(ldr::GetProgramInfo(out, std::addressof(status), loc, path, attrs));

        if (loc.program_id != out->program_id) {
            /* Redirect the program path. */
            const ncm::ProgramLocation new_loc = ncm::ProgramLocation::Make(out->program_id, static_cast<ncm::StorageId>(loc.storage_id));
            R_TRY(RedirectProgramPath(path, sizeof(path), new_loc));

            /* Update the arguments, as needed. */
            if (const auto *entry = g_argument_store.Get(loc.program_id); entry != nullptr) {
                R_TRY(g_argument_store.Set(new_loc.program_id, entry->argument, entry->argument_size));
            }
        }

        /* If we should, set the output status. */
        if (out_status != nullptr) {
            *out_status = status;
        }

        R_SUCCEED();
    }

    Result LoaderService::PinProgram(PinId *out, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &status) {
        *out = {};
        R_RETURN(ldr::PinProgram(out, loc, status));
    }

    Result LoaderService::UnpinProgram(PinId id) {
        R_RETURN(ldr::UnpinProgram(id));
    }

    Result LoaderService::SetProgramArgument(ncm::ProgramId program_id, const void *argument, size_t size) {
        R_RETURN(g_argument_store.Set(program_id, argument, size));
    }

    Result LoaderService::FlushArguments() {
        R_RETURN(g_argument_store.Flush());
    }

    Result LoaderService::GetProcessModuleInfo(u32 *out_count, ModuleInfo *out, size_t max_out_count, os::ProcessId process_id) {
        *out_count = 0;
        std::memset(out, 0, max_out_count * sizeof(*out));
        R_RETURN(ldr::GetProcessModuleInfo(out_count, out, max_out_count, process_id));
    }

    Result LoaderService::RegisterExternalCode(os::NativeHandle *out, ncm::ProgramId program_id) {
        R_RETURN(fssystem::CreateExternalCode(out, program_id));
    }

    void LoaderService::UnregisterExternalCode(ncm::ProgramId program_id) {
        fssystem::DestroyExternalCode(program_id);
    }

    void LoaderService::HasLaunchedBootProgram(bool *out, ncm::ProgramId program_id) {
        *out = ldr::HasLaunchedBootProgram(program_id);
    }

    Result LoaderService::SetEnabledProgramVerification(bool enabled) {
        ldr::SetEnabledProgramVerification(enabled);
        R_SUCCEED();
    }

}
