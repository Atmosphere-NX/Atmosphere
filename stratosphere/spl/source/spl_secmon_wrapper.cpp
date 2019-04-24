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

#include "spl_secmon_wrapper.hpp"

Result SecureMonitorWrapper::ConvertToSplResult(SmcResult result) {
    if (result == SmcResult_Success) {
        return ResultSuccess;
    }
    if (result < SmcResult_Max) {
        return MAKERESULT(Module_Spl, static_cast<u32>(result));
    }
    return ResultSplUnknownSmcResult;
}

Result SecureMonitorWrapper::GetConfig(u64 *out, SplConfigItem which) {
    /* TODO */
    return ResultKernelConnectionClosed;
}

Result SecureMonitorWrapper::ExpMod(void *out, size_t out_size, const void *base, size_t base_size, const void *exp, size_t exp_size, const void *mod, size_t mod_size) {
    /* TODO */
    return ResultKernelConnectionClosed;
}

Result SecureMonitorWrapper::SetConfig(SplConfigItem which, u64 value) {
    /* TODO */
    return ResultKernelConnectionClosed;
}

Result SecureMonitorWrapper::GenerateRandomBytes(void *out, size_t size) {
    /* TODO */
    return ResultKernelConnectionClosed;
}

Result SecureMonitorWrapper::IsDevelopment(bool *out) {
    u64 is_retail;
    Result rc = this->GetConfig(&is_retail, SplConfigItem_IsRetail);
    if (R_FAILED(rc)) {
        return rc;
    }

    *out = (is_retail == 0);
    return ResultSuccess;
}

Result SecureMonitorWrapper::SetBootReason(BootReasonValue boot_reason) {
    if (this->IsBootReasonSet()) {
        return ResultSplBootReasonAlreadySet;
    }

    this->boot_reason = boot_reason;
    this->boot_reason_set = true;
    return ResultSuccess;
}

Result SecureMonitorWrapper::GetBootReason(BootReasonValue *out) {
    if (!this->IsBootReasonSet()) {
        return ResultSplBootReasonNotSet;
    }

    *out = GetBootReason();
    return ResultSuccess;
}