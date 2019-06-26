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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

enum InformationCmd {
    Information_Cmd_GetTitleId = 0,

    Information_Cmd_AtmosphereGetProcessId = 65000,
    Information_Cmd_AtmosphereHasCreatedTitle = 65001,
};

class InformationService final : public IServiceObject {
    private:
        /* Actual commands. */
        Result GetTitleId(Out<u64> tid, u64 pid);

        /* Atmosphere commands. */
        Result AtmosphereGetProcessId(Out<u64> pid, u64 tid);
        Result AtmosphereHasLaunchedTitle(Out<bool> out, u64 tid);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<Information_Cmd_GetTitleId, &InformationService::GetTitleId>(),
            MakeServiceCommandMeta<Information_Cmd_AtmosphereGetProcessId, &InformationService::AtmosphereGetProcessId>(),
            MakeServiceCommandMeta<Information_Cmd_AtmosphereHasCreatedTitle, &InformationService::AtmosphereHasLaunchedTitle>(),
        };
};
