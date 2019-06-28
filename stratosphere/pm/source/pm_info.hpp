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

class InformationService final : public IServiceObject {
    private:
        enum class CommandId {
            GetTitleId                 = 0,
            AtmosphereGetProcessId     = 65000,
            AtmosphereHasLaunchedTitle = 65001,
        };
    private:
        /* Actual commands. */
        Result GetTitleId(Out<u64> tid, u64 pid);

        /* Atmosphere commands. */
        Result AtmosphereGetProcessId(Out<u64> pid, u64 tid);
        Result AtmosphereHasLaunchedTitle(Out<bool> out, u64 tid);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MAKE_SERVICE_COMMAND_META(InformationService, GetTitleId),
            MAKE_SERVICE_COMMAND_META(InformationService, AtmosphereGetProcessId),
            MAKE_SERVICE_COMMAND_META(InformationService, AtmosphereHasLaunchedTitle),
        };
};
