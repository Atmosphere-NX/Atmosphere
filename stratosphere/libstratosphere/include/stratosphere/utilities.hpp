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
#include <cstdlib>

#include "hossynch.hpp"

static inline uintptr_t GetIoMapping(const u64 io_addr, const u64 io_size) {
    u64 vaddr;
    const u64 aligned_addr = (io_addr & ~0xFFFul);
    const u64 aligned_size = io_size + (io_addr - aligned_addr);
    R_ASSERT(svcQueryIoMapping(&vaddr, aligned_addr, aligned_size));
    return static_cast<uintptr_t>(vaddr + (io_addr - aligned_addr));
}

static inline void RebootToRcm() {
    SecmonArgs args = {0};
    args.X[0] = 0xC3000401; /* smcSetConfig */
    args.X[1] = 65001; /* Exosphere reboot */
    args.X[3] = 1; /* Perform reboot to RCM. */
    svcCallSecureMonitor(&args);
}

static inline void RebootToIramPayload() {
    SecmonArgs args = {0};
    args.X[0] = 0xC3000401; /* smcSetConfig */
    args.X[1] = 65001; /* Exosphere reboot */
    args.X[3] = 2; /* Perform reboot to payload at 0x40010000 in IRAM. */
    svcCallSecureMonitor(&args);
}

static inline void PerformShutdownSmc() {
    SecmonArgs args = {0};
    args.X[0] = 0xC3000401; /* smcSetConfig */
    args.X[1] = 65002; /* Exosphere shutdown */
    args.X[3] = 1; /* Perform shutdown. */
    svcCallSecureMonitor(&args);
}

static inline void CopyToIram(uintptr_t iram_addr, void *src_addr, size_t size) {
    SecmonArgs args = {0};
    args.X[0] = 0xF0000201;     /* smcAmsIramCopy */
    args.X[1] = (u64)src_addr;  /* DRAM address */
    args.X[2] = (u64)iram_addr; /* IRAM address */
    args.X[3] = size;           /* Amount to copy */
    args.X[4] = 1;              /* 1 = Write */
    svcCallSecureMonitor(&args);
}

static inline void CopyFromIram(void *dst_addr, uintptr_t iram_addr, size_t size) {
    SecmonArgs args = {0};
    args.X[0] = 0xF0000201;     /* smcAmsIramCopy */
    args.X[1] = (u64)dst_addr;  /* DRAM address */
    args.X[2] = (u64)iram_addr; /* IRAM address */
    args.X[3] = size;           /* Amount to copy */
    args.X[4] = 0;              /* 0 = Read */
    svcCallSecureMonitor(&args);
}

static inline Result SmcGetConfig(SplConfigItem config_item, u64 *out_config) {
    SecmonArgs args = {0};
    args.X[0] = 0xC3000002;        /* smcGetConfig */
    args.X[1] = (u64)config_item;  /* config item */

    R_TRY(svcCallSecureMonitor(&args));
    if (args.X[0] != 0) {
        /* SPL result n = SMC result n */
        return MAKERESULT(26, args.X[0]);
    }

    if (out_config) {
        *out_config = args.X[1];
    }
    return ResultSuccess;
}

static inline Result GetRcmBugPatched(bool *out) {
    u64 tmp = 0;
    R_TRY(SmcGetConfig((SplConfigItem)65004, &tmp));
    *out = (tmp != 0);
    return ResultSuccess;
}

static inline bool IsRcmBugPatched() {
    bool rcm_bug_patched;
    R_ASSERT(GetRcmBugPatched(&rcm_bug_patched));
    return rcm_bug_patched;
}

static inline Result GetShouldBlankProdInfo(bool *out) {
    u64 tmp = 0;
    R_TRY(SmcGetConfig((SplConfigItem)65005, &tmp));
    *out = (tmp != 0);
    return ResultSuccess;
}

static inline bool ShouldBlankProdInfo() {
    bool should_blank_prodinfo;
    R_ASSERT(GetShouldBlankProdInfo(&should_blank_prodinfo));
    return should_blank_prodinfo;
}

HosRecursiveMutex &GetSmSessionMutex();

template<typename F>
static void DoWithSmSession(F f) {
    std::scoped_lock<HosRecursiveMutex &> lk(GetSmSessionMutex());
    {
        R_ASSERT(smInitialize());
        f();
        smExit();
    }
}
