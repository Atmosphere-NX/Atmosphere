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

#include "../utils.hpp"

enum BpcAtmosphereCmd : u32 {
    BpcAtmosphereCmd_RebootToFatalError = 65000,
};

class BpcAtmosphereService : public IServiceObject {
    private:
        Result RebootToFatalError(InBuffer<AtmosphereFatalErrorContext> ctx);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<BpcAtmosphereCmd_RebootToFatalError, &BpcAtmosphereService::RebootToFatalError>(),
        };
};
