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

#include "spl_types.hpp"
#include "spl_secmon_wrapper.hpp"

class GeneralService : public IServiceObject {
    private:
        SecureMonitorWrapper *secmon_wrapper;
    public:
        GeneralService(SecureMonitorWrapper *sw) : secmon_wrapper(sw) {
            /* ... */
        }

        virtual ~GeneralService() { /* ... */ }
    protected:
        SecureMonitorWrapper *GetSecureMonitorWrapper() const {
            return this->secmon_wrapper;
        }
    protected:
        /* Actual commands. */
        virtual Result GetConfig(Out<u64> out, u32 which);
        virtual Result ExpMod(OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> exp, InPointer<u8> mod);
        virtual Result SetConfig(u32 which, u64 value);
        virtual Result GenerateRandomBytes(OutPointerWithClientSize<u8> out);
        virtual Result IsDevelopment(Out<bool> is_dev);
        virtual Result SetBootReason(BootReasonValue boot_reason);
        virtual Result GetBootReason(Out<BootReasonValue> out);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<Spl_Cmd_GetConfig, &GeneralService::GetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_ExpMod, &GeneralService::ExpMod>(),
            MakeServiceCommandMeta<Spl_Cmd_SetConfig, &GeneralService::SetConfig>(),
            MakeServiceCommandMeta<Spl_Cmd_GenerateRandomBytes, &GeneralService::GenerateRandomBytes>(),
            MakeServiceCommandMeta<Spl_Cmd_IsDevelopment, &GeneralService::IsDevelopment>(),
            MakeServiceCommandMeta<Spl_Cmd_SetBootReason, &GeneralService::SetBootReason, FirmwareVersion_300>(),
            MakeServiceCommandMeta<Spl_Cmd_GetBootReason, &GeneralService::GetBootReason, FirmwareVersion_300>(),
        };
};
