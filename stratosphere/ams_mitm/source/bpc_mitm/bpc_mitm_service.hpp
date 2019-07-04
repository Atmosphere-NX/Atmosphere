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

class BpcMitmService : public IMitmServiceObject {
    private:
        enum class CommandId {
            ShutdownSystem = 0,
            RebootSystem   = 1,
        };
    public:
        BpcMitmService(std::shared_ptr<Service> s, u64 pid, sts::ncm::TitleId tid) : IMitmServiceObject(s, pid, tid) {
            /* ... */
        }

        static bool ShouldMitm(u64 pid, sts::ncm::TitleId tid) {
            /* We will mitm:
             * - am, to intercept the Reboot/Power buttons in the overlay menu.
             * - fatal, to simplify payload reboot logic significantly
             * - applications, to allow homebrew to take advantage of the feature.
             */
            return tid == sts::ncm::TitleId::Am || tid == sts::ncm::TitleId::Fatal || sts::ncm::IsApplicationTitleId(tid) || Utils::IsHblTid(static_cast<u64>(tid));
        }

        static void PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx);

    protected:
        /* Overridden commands. */
        Result ShutdownSystem();
        Result RebootSystem();
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MAKE_SERVICE_COMMAND_META(BpcMitmService, ShutdownSystem),
            MAKE_SERVICE_COMMAND_META(BpcMitmService, RebootSystem),
        };
};
