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

#include <switch.h>
#include <stratosphere.hpp>

#include "spl_general_service.hpp"

Result GeneralService::GetConfig(Out<u64> out, u32 which) {
    return this->GetSecureMonitorWrapper()->GetConfig(out.GetPointer(), static_cast<SplConfigItem>(which));
}

Result GeneralService::ExpMod(OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> exp, InPointer<u8> mod) {
    return this->GetSecureMonitorWrapper()->ExpMod(out.pointer, out.num_elements, base.pointer, base.num_elements, exp.pointer, exp.num_elements, mod.pointer, mod.num_elements);
}

Result GeneralService::SetConfig(u32 which, u64 value) {
    return this->GetSecureMonitorWrapper()->SetConfig(static_cast<SplConfigItem>(which), value);
}

Result GeneralService::GenerateRandomBytes(OutPointerWithClientSize<u8> out) {
    return this->GetSecureMonitorWrapper()->GenerateRandomBytes(out.pointer, out.num_elements);
}

Result GeneralService::IsDevelopment(Out<bool> is_dev) {
    return this->GetSecureMonitorWrapper()->IsDevelopment(is_dev.GetPointer());
}

Result GeneralService::SetBootReason(BootReasonValue boot_reason) {
    return this->GetSecureMonitorWrapper()->SetBootReason(boot_reason);
}

Result GeneralService::GetBootReason(Out<BootReasonValue> out) {
    return this->GetSecureMonitorWrapper()->GetBootReason(out.GetPointer());
}
