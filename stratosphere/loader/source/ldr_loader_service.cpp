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
#include "ldr_arguments.hpp"
#include "ldr_content_management.hpp"
#include "ldr_development_manager.hpp"
#include "ldr_process_creation.hpp"
#include "ldr_launch_record.hpp"
#include "ldr_loader_service.hpp"

namespace ams::ldr {

    namespace {

        Result GetProgramInfoImpl(ProgramInfo *out, cfg::OverrideStatus *out_status, const ncm::ProgramLocation &loc) {
            /* Zero output. */
            std::memset(out, 0, sizeof(*out));
            cfg::OverrideStatus status = {};

            R_TRY(ldr::GetProgramInfo(out, std::addressof(status), loc));

            if (loc.storage_id != static_cast<u8>(ncm::StorageId::None) && loc.program_id != out->program_id) {
                char path[fs::EntryNameLengthMax];
                const ncm::ProgramLocation new_loc = ncm::ProgramLocation::Make(out->program_id, static_cast<ncm::StorageId>(loc.storage_id));

                R_TRY(ResolveContentPath(path, loc));
                path[sizeof(path) - 1] = '\x00';

                R_TRY(RedirectContentPath(path, new_loc));

                const auto arg_info = args::Get(loc.program_id);
                if (arg_info != nullptr) {
                    R_TRY(args::Set(new_loc.program_id, arg_info->args, arg_info->args_size));
                }
            }

            if (out_status != nullptr) {
                *out_status = status;
            }

            return ResultSuccess();
        }

    }

    /* Official commands. */
    Result LoaderService::CreateProcess(sf::OutMoveHandle out, PinId id, u32 flags, sf::CopyHandle &&reslimit_h) {
        ncm::ProgramLocation loc;
        cfg::OverrideStatus override_status;
        char path[fs::EntryNameLengthMax];

        /* Get location and override status. */
        R_TRY(ldr::GetProgramLocationAndOverrideStatusFromPinId(std::addressof(loc), std::addressof(override_status), id));

        if (loc.storage_id != static_cast<u8>(ncm::StorageId::None)) {
            R_TRY(ResolveContentPath(path, loc));
            path[sizeof(path) - 1] = '\x00';
        } else {
            path[0] = '\x00';
        }

        /* Create the process. */
        os::NativeHandle process_handle;
        R_TRY(ldr::CreateProcess(std::addressof(process_handle), id, loc, override_status, path, flags, reslimit_h.GetOsHandle()));

        /* Set output process handle. */
        out.SetValue(process_handle, true);
        return ResultSuccess();
    }

    Result LoaderService::GetProgramInfo(sf::Out<ProgramInfo> out, const ncm::ProgramLocation &loc) {
        return GetProgramInfoImpl(out.GetPointer(), nullptr, loc);
    }

    Result LoaderService::PinProgram(sf::Out<PinId> out_id, const ncm::ProgramLocation &loc) {
        return ldr::PinProgram(out_id.GetPointer(), loc, cfg::OverrideStatus{});
    }

    Result LoaderService::UnpinProgram(PinId id) {
        return ldr::UnpinProgram(id);
    }

    Result LoaderService::SetProgramArgumentsDeprecated(ncm::ProgramId program_id, const sf::InPointerBuffer &args, u32 args_size) {
        return args::Set(program_id, args.GetPointer(), std::min(args.GetSize(), size_t(args_size)));
    }

    Result LoaderService::SetProgramArguments(ncm::ProgramId program_id, const sf::InPointerBuffer &args) {
        return args::Set(program_id, args.GetPointer(), args.GetSize());
    }

    Result LoaderService::FlushArguments() {
        return args::Flush();
    }

    Result LoaderService::GetProcessModuleInfo(sf::Out<u32> count, const sf::OutPointerArray<ModuleInfo> &out, os::ProcessId process_id) {
        *count = 0;
        std::memset(out.GetPointer(), 0, out.GetSize() * sizeof(ldr::ModuleInfo));
        return ldr::GetProcessModuleInfo(count.GetPointer(), out.GetPointer(), out.GetSize(), process_id);
    }

    Result LoaderService::SetEnabledProgramVerification(bool enabled) {
        ldr::SetEnabledProgramVerification(enabled);
        return ResultSuccess();
    }

    /* Atmosphere commands. */
    Result LoaderService::AtmosphereRegisterExternalCode(sf::OutMoveHandle out, ncm::ProgramId program_id) {
        os::NativeHandle handle;
        R_TRY(fssystem::CreateExternalCode(std::addressof(handle), program_id));

        out.SetValue(handle, true);
        return ResultSuccess();
    }

    void LoaderService::AtmosphereUnregisterExternalCode(ncm::ProgramId program_id) {
        fssystem::DestroyExternalCode(program_id);
    }

    void LoaderService::AtmosphereHasLaunchedBootProgram(sf::Out<bool> out, ncm::ProgramId program_id) {
        out.SetValue(ldr::HasLaunchedBootProgram(program_id));
    }

    Result LoaderService::AtmosphereGetProgramInfo(sf::Out<ProgramInfo> out_program_info, sf::Out<cfg::OverrideStatus> out_status, const ncm::ProgramLocation &loc) {
        return GetProgramInfoImpl(out_program_info.GetPointer(), out_status.GetPointer(), loc);
    }

    Result LoaderService::AtmospherePinProgram(sf::Out<PinId> out_id, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &override_status) {
        return ldr::PinProgram(out_id.GetPointer(), loc, override_status);
    }

}
