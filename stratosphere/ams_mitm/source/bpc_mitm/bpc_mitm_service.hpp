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

enum BpcCmd : u32 {
    BpcCmd_ShutdownSystem = 0,
    BpcCmd_RebootSystem = 1,
};

class BpcMitmService : public IMitmServiceObject {
    public:
        BpcMitmService(std::shared_ptr<Service> s, u64 pid) : IMitmServiceObject(s, pid) {
            /* ... */
        }
        
        static bool ShouldMitm(u64 pid, u64 tid) {
            /* We will mitm:
             * - am, to intercept the Reboot/Power buttons in the overlay menu.
             * - fatal, to simplify payload reboot logic significantly
             * - applications, to allow homebrew to take advantage of the feature.
             */
            return tid == TitleId_Am || tid == TitleId_Fatal || TitleIdIsApplication(tid) || Utils::IsHblTid(tid);
        }
        
        static void PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx);
                    
    protected:
        /* Overridden commands. */
        Result ShutdownSystem();
        Result RebootSystem();
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<BpcCmd_ShutdownSystem, &BpcMitmService::ShutdownSystem>(),
            MakeServiceCommandMeta<BpcCmd_RebootSystem, &BpcMitmService::RebootSystem>(),
        };
};
