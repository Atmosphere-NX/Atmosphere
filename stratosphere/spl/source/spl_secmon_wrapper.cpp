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

/* Convenient. */
constexpr size_t DeviceAddressSpaceAlignSize = 0x400000;
constexpr size_t DeviceAddressSpaceAlignMask = DeviceAddressSpaceAlignSize - 1;
constexpr u32 DeviceMapBase = 0x80000000u;

/* Globals. */
static CtrDrbg g_drbg;
static Event g_se_event;

static Handle g_se_das_hnd;
static u32 g_se_mapped_work_buffer_addr;
static __attribute__((aligned(0x1000))) u8 g_work_buffer[0x1000];

static HosMutex g_se_lock;

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

void SecureMonitorWrapper::InitializeDeviceAddressSpace() {
    constexpr u64 DeviceName_SE = 29;

    /* Create Address Space. */
    if (R_FAILED(svcCreateDeviceAddressSpace(&g_se_das_hnd, 0, (1ul << 32)))) {
        std::abort();
    }
    /* Attach it to the SE. */
    if (R_FAILED(svcAttachDeviceAddressSpace(DeviceName_SE, g_se_das_hnd))) {
        std::abort();
    }
    
    const u64 work_buffer_addr = reinterpret_cast<u64>(g_work_buffer);
    g_se_mapped_work_buffer_addr = 0x80000000u + (work_buffer_addr & DeviceAddressSpaceAlignMask);
    
    /* Map the work buffer for the SE. */
    if (R_FAILED(svcMapDeviceAddressSpaceAligned(g_se_das_hnd, CUR_PROCESS_HANDLE, work_buffer_addr, sizeof(g_work_buffer), g_se_mapped_work_buffer_addr, 3))) {
        std::abort();
    }
}

void SecureMonitorWrapper::Initialize() {
    /* Initialize the Drbg. */
    InitializeCtrDrbg();
    /* Initialize SE interrupt event. */
    InitializeSeInterruptEvent();
    /* Initialize DAS for the SE. */
    InitializeDeviceAddressSpace();
}

void SecureMonitorWrapper::WaitSeOperationComplete() {
    eventWait(&g_se_event, U64_MAX);
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

SmcResult SecureMonitorWrapper::WaitCheckStatus(AsyncOperationKey op_key) {
    WaitSeOperationComplete();
    
    SmcResult op_res;
    SmcResult res = SmcWrapper::CheckStatus(&op_res, op_key);
    if (res != SmcResult_Success) {
        return res;
    }
    
    return op_res;
}

SmcResult SecureMonitorWrapper::WaitGetResult(void *out_buf, size_t out_buf_size, AsyncOperationKey op_key) {
    WaitSeOperationComplete();
    
    SmcResult op_res;
    SmcResult res = SmcWrapper::GetResult(&op_res, out_buf, out_buf_size, op_key);
    if (res != SmcResult_Success) {
        return res;
    }
    
    return op_res;
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
    struct ExpModLayout {
        u8 base[0x100];
        u8 exp[0x100];
        u8 mod[0x100];
    };
    ExpModLayout *layout = reinterpret_cast<ExpModLayout *>(g_work_buffer);
    
    /* Validate sizes. */
    if (base_size > sizeof(layout->base)) {
        return ResultSplInvalidSize;
    }
    if (exp_size > sizeof(layout->exp)) {
        return ResultSplInvalidSize;
    }
    if (mod_size > sizeof(layout->mod)) {
        return ResultSplInvalidSize;
    }
    if (out_size > sizeof(g_work_buffer) / 2) {
        return ResultSplInvalidSize;
    }
    
    /* Copy data into work buffer. */
    const size_t base_ofs = sizeof(layout->base) - base_size;
    const size_t mod_ofs = sizeof(layout->mod) - mod_size;
    std::memset(layout, 0, sizeof(*layout));
    std::memcpy(layout->base + base_ofs, base, base_size);
    std::memcpy(layout->exp, exp, exp_size);
    std::memcpy(layout->mod + mod_ofs, mod, mod_size);
    
    /* Do exp mod operation. */
    {
        std::scoped_lock<HosMutex> lk(g_se_lock);
        AsyncOperationKey op_key;
        
        SmcResult res = SmcWrapper::ExpMod(&op_key, layout->base, layout->exp, exp_size, layout->mod);
        if (res != SmcResult_Success) {
            return ConvertToSplResult(res);
        }
        
        if ((res = WaitGetResult(g_work_buffer, out_size, op_key)) != SmcResult_Success) {
            return ConvertToSplResult(res);
        }
    }
    
    std::memcpy(out, g_work_buffer, out_size);
    return ResultSuccess;
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