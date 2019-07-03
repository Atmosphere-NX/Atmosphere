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

#include <limits>
#include "ldr_arguments.hpp"
#include "ldr_content_management.hpp"
#include "ldr_ecs.hpp"
#include "ldr_process_creation.hpp"
#include "ldr_launch_record.hpp"
#include "ldr_loader_service.hpp"
#include "ldr_ro_manager.hpp"

namespace sts::ldr {

    /* Official commands. */
    Result LoaderService::CreateProcess(Out<MovedHandle> proc_h, PinId id, u32 flags, CopiedHandle reslimit) {
        AutoHandle reslimit_holder(reslimit.GetValue());
        ncm::TitleLocation loc;
        char path[FS_MAX_PATH];

        /* Get location. */
        R_TRY(ldr::ro::GetTitleLocation(&loc, id));

        if (loc.storage_id != static_cast<u8>(ncm::StorageId::None)) {
            R_TRY(ResolveContentPath(path, loc));
        }

        return ldr::CreateProcess(proc_h.GetHandlePointer(), id, loc, path, flags, reslimit_holder.Get());
    }

    Result LoaderService::GetProgramInfo(OutPointerWithServerSize<ProgramInfo, 0x1> out_program_info, ncm::TitleLocation loc) {
        /* Zero output. */
        ProgramInfo *out = out_program_info.pointer;
        std::memset(out, 0, sizeof(*out));

        R_TRY(ldr::GetProgramInfo(out, loc));

        if (loc.storage_id != static_cast<u8>(ncm::StorageId::None) && loc.title_id != out->title_id) {
            char path[FS_MAX_PATH];
            const ncm::TitleLocation new_loc = ncm::TitleLocation::Make(out->title_id, static_cast<ncm::StorageId>(loc.storage_id));

            R_TRY(ResolveContentPath(path, loc));
            R_TRY(RedirectContentPath(path, new_loc));

            const auto arg_info = args::Get(loc.title_id);
            if (arg_info != nullptr) {
                R_TRY(args::Set(new_loc.title_id, arg_info->args, arg_info->args_size));
            }
        }

        return ResultSuccess;
    }

    Result LoaderService::PinTitle(Out<PinId> out_id, ncm::TitleLocation loc) {
        return ldr::ro::PinTitle(out_id.GetPointer(), loc);
    }

    Result LoaderService::UnpinTitle(PinId id) {
        return ldr::ro::UnpinTitle(id);
    }

    Result LoaderService::SetTitleArguments(ncm::TitleId title_id, InPointer<char> args, u32 args_size) {
        return args::Set(title_id, args.pointer, std::min(args.num_elements, size_t(args_size)));
    }

    Result LoaderService::ClearArguments() {
        return args::Clear();
    }

    Result LoaderService::GetProcessModuleInfo(Out<u32> count, OutPointerWithClientSize<ModuleInfo> out, u64 process_id) {
        if (out.num_elements > std::numeric_limits<s32>::max()) {
            return ResultLoaderInvalidSize;
        }

        return ldr::ro::GetProcessModuleInfo(count.GetPointer(), out.pointer, out.num_elements, process_id);
    }

    /* Atmosphere commands. */
    Result LoaderService::AtmosphereSetExternalContentSource(Out<MovedHandle> out, ncm::TitleId title_id) {
        return ecs::Set(out.GetHandlePointer(), title_id);
    }

    void LoaderService::AtmosphereClearExternalContentSource(ncm::TitleId title_id) {
        R_ASSERT(ecs::Clear(title_id));
    }

    void LoaderService::AtmosphereHasLaunchedTitle(Out<bool> out, ncm::TitleId title_id) {
        out.SetValue(ldr::HasLaunchedTitle(title_id));
    }

}
