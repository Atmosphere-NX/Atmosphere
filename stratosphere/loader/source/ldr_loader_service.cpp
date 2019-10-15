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
    Result LoaderService::CreateProcess(sf::OutMoveHandle proc_h, PinId id, u32 flags, sf::CopyHandle reslimit_h) {
        os::ManagedHandle reslimit_holder(reslimit_h.GetValue());
        ncm::TitleLocation loc;
        char path[FS_MAX_PATH];

        /* Get location. */
        R_TRY(ldr::ro::GetTitleLocation(&loc, id));

        if (loc.storage_id != static_cast<u8>(ncm::StorageId::None)) {
            R_TRY(ResolveContentPath(path, loc));
        }

        return ldr::CreateProcess(proc_h.GetHandlePointer(), id, loc, path, flags, reslimit_holder.Get());
    }

    Result LoaderService::GetProgramInfo(sf::Out<ProgramInfo> out, const ncm::TitleLocation &loc) {
        /* Zero output. */
        std::memset(out.GetPointer(), 0, sizeof(*out.GetPointer()));

        R_TRY(ldr::GetProgramInfo(out.GetPointer(), loc));

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

    Result LoaderService::PinTitle(sf::Out<PinId> out_id, const ncm::TitleLocation &loc) {
        return ldr::ro::PinTitle(out_id.GetPointer(), loc);
    }

    Result LoaderService::UnpinTitle(PinId id) {
        return ldr::ro::UnpinTitle(id);
    }

    Result LoaderService::SetTitleArguments(ncm::TitleId title_id, const sf::InPointerBuffer &args, u32 args_size) {
        return args::Set(title_id, args.GetPointer(), std::min(args.GetSize(), size_t(args_size)));
    }

    Result LoaderService::ClearArguments() {
        return args::Clear();
    }

    Result LoaderService::GetProcessModuleInfo(sf::Out<u32> count, const sf::OutPointerArray<ModuleInfo> &out, os::ProcessId process_id) {
        if (out.GetSize() > std::numeric_limits<s32>::max()) {
            return ResultLoaderInvalidSize;
        }

        return ldr::ro::GetProcessModuleInfo(count.GetPointer(), out.GetPointer(), out.GetSize(), process_id);
    }

    /* Atmosphere commands. */
    Result LoaderService::AtmosphereSetExternalContentSource(sf::OutMoveHandle out, ncm::TitleId title_id) {
        return ecs::Set(out.GetHandlePointer(), title_id);
    }

    void LoaderService::AtmosphereClearExternalContentSource(ncm::TitleId title_id) {
        R_ASSERT(ecs::Clear(title_id));
    }

    void LoaderService::AtmosphereHasLaunchedTitle(sf::Out<bool> out, ncm::TitleId title_id) {
        out.SetValue(ldr::HasLaunchedTitle(title_id));
    }

}
