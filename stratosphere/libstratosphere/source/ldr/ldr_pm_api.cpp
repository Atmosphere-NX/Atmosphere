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
#include <set>
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/ldr.hpp>
#include <stratosphere/ldr/ldr_pm_api.hpp>

#include "ldr_ams.h"

namespace sts::ldr::pm {

    /* Information API. */
    Result CreateProcess(Handle *out, PinId pin_id, u32 flags, Handle reslimit) {
        return ldrPmCreateProcess(flags, pin_id.value, reslimit, out);
    }

    Result GetProgramInfo(ProgramInfo *out, const ncm::TitleLocation &loc) {
        return ldrPmGetProgramInfo(static_cast<u64>(loc.title_id), static_cast<FsStorageId>(loc.storage_id), reinterpret_cast<LoaderProgramInfo *>(out));
    }

    Result PinTitle(PinId *out, const ncm::TitleLocation &loc) {
        static_assert(sizeof(*out) == sizeof(u64), "PinId definition!");
        return ldrPmRegisterTitle(static_cast<u64>(loc.title_id), static_cast<FsStorageId>(loc.storage_id), reinterpret_cast<u64 *>(out));
    }

    Result UnpinTitle(PinId pin_id) {
        return ldrPmUnregisterTitle(pin_id.value);
    }

    Result HasLaunchedTitle(bool *out, ncm::TitleId title_id) {
        return ldrPmAtmosphereHasLaunchedTitle(out, static_cast<u64>(title_id));
    }

}
