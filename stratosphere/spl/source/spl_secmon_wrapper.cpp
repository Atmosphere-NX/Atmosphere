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
#include "spl_smc_wrapper.hpp"
#include "spl_ctr_drbg.hpp"

/* Globals. */
static CtrDrbg g_drbg;
static Event g_se_event;
static __attribute__((aligned(0x1000))) u8 g_work_buffer[0x1000];

void SecureMonitorWrapper::InitializeCtrDrbg() {
    u8 seed[CtrDrbg::SeedSize];
    
    if (SmcWrapper::GenerateRandomBytes(seed, sizeof(seed)) != SmcResult_Success) {
        std::abort();
    }
    
    g_drbg.Initialize(seed);
}

void SecureMonitorWrapper::InitializeSeInterruptEvent() {
    u64 irq_num;
    SmcWrapper::GetConfig(&irq_num, 1, SplConfigItem_SecurityEngineIrqNumber);
    Handle hnd;
    if (R_FAILED(svcCreateInterruptEvent(&hnd, irq_num, 1))) {
        std::abort();
    }
    eventLoadRemote(&g_se_event, hnd, true);
}

void SecureMonitorWrapper::Initialize() {
    /* Initialize the Drbg. */
    InitializeCtrDrbg();
    /* Initialize SE interrupt event. */
    InitializeSeInterruptEvent();
}

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
    /* Nintendo explicitly blacklists package2 hash here, amusingly. */
    /* This is not blacklisted in safemode, but we're never in safe mode... */
    if (which == SplConfigItem_Package2Hash) {
        return ResultSplInvalidArgument;
    }
    
    SmcResult res = SmcWrapper::GetConfig(out, 1, which);
    
    /* Nintendo has some special handling here for hardware type/is_retail. */
    if (which == SplConfigItem_HardwareType && res == SmcResult_InvalidArgument) {
        *out = 0;
        res = SmcResult_Success;
    }
    if (which == SplConfigItem_IsRetail && res == SmcResult_InvalidArgument) {
        *out = 0;
        res = SmcResult_Success;
    }
    
    return ConvertToSplResult(res);
}

Result SecureMonitorWrapper::ExpMod(void *out, size_t out_size, const void *base, size_t base_size, const void *exp, size_t exp_size, const void *mod, size_t mod_size) {
    /* TODO */
    return ResultKernelConnectionClosed;
}

Result SecureMonitorWrapper::SetConfig(SplConfigItem which, u64 value) {
    return ConvertToSplResult(SmcWrapper::SetConfig(which, &value, 1));
}

Result SecureMonitorWrapper::GenerateRandomBytesInternal(void *out, size_t size) {
    if (!g_drbg.GenerateRandomBytes(out, size)) {
        /* We need to reseed. */
        {
            u8 seed[CtrDrbg::SeedSize];
            
            SmcResult res = SmcWrapper::GenerateRandomBytes(seed, sizeof(seed));
            if (res != SmcResult_Success) {
                return ConvertToSplResult(res);
            }
            
            g_drbg.Reseed(seed);
            g_drbg.GenerateRandomBytes(out, size);
        }
    }
    
    return ResultSuccess;
}

Result SecureMonitorWrapper::GenerateRandomBytes(void *out, size_t size) {
    u8 *cur_dst = reinterpret_cast<u8 *>(out);

    for (size_t ofs = 0; ofs < size; ofs += CtrDrbg::MaxRequestSize) {
        const size_t cur_size = std::min(size - ofs, CtrDrbg::MaxRequestSize);
        
        Result rc = GenerateRandomBytesInternal(cur_dst, size);
        if (R_FAILED(rc)) {
            return rc;
        }
        cur_dst += cur_size;
    }

    return ResultSuccess;
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