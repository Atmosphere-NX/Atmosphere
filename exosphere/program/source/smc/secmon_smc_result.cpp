/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <exosphere.hpp>
#include "../secmon_error.hpp"
#include "../secmon_page_mapper.hpp"
#include "secmon_smc_result.hpp"

namespace ams::secmon::smc {

    namespace {

        constinit u64              g_async_key     = InvalidAsyncKey;
        constinit GetResultHandler g_async_handler = nullptr;

        u64 GenerateRandomU64() {
            /* NOTE: This is one of the only places where Nintendo does not do data flushing. */
            /*       to ensure coherency when doing random byte generation. */
            /*       It is not clear why it is necessary elsewhere but not here. */
            /* TODO: Figure out why. */
            u64 v;
            se::GenerateRandomBytes(std::addressof(v), sizeof(v));
            return v;
        }

    }

    u64 BeginAsyncOperation(GetResultHandler handler) {
        /* Only allow one async operation at a time. */
        if (g_async_key != InvalidAsyncKey) {
            return InvalidAsyncKey;
        }

        /* Generate a random async key. */
        g_async_key     = GenerateRandomU64();
        g_async_handler = handler;

        return g_async_key;
    }

    void CancelAsyncOperation(u64 async_key) {
        if (async_key == g_async_key) {
            g_async_key = InvalidAsyncKey;
        }
    }

    void EndAsyncOperation() {
        gic::SetPending(SecurityEngineUserInterruptId);
    }

    SmcResult SmcGetResult(SmcArguments &args) {
        /* Decode arguments. */
        const u64 async_key = args.r[1];

        /* Validate arguments. */
        SMC_R_UNLESS(g_async_key != InvalidAsyncKey,      NoAsyncOperation);
        SMC_R_UNLESS(g_async_key == async_key,       InvalidAsyncOperation);

        /* Call the handler. */
        args.r[1] = static_cast<u64>(g_async_handler(nullptr, 0));
        g_async_key = InvalidAsyncKey;

        return SmcResult::Success;
    }

    SmcResult SmcGetResultData(SmcArguments &args) {
        /* Decode arguments. */
        const u64 async_key            = args.r[1];
        const uintptr_t user_phys_addr = args.r[2];
        const size_t    user_size      = args.r[3];

        /* Allocate a work buffer on the stack. */
        alignas(8) u8 work_buffer[1_KB];

        /* Validate arguments. */
        SMC_R_UNLESS(g_async_key != InvalidAsyncKey,        NoAsyncOperation);
        SMC_R_UNLESS(g_async_key == async_key,         InvalidAsyncOperation);
        SMC_R_UNLESS(user_size <= sizeof(work_buffer),       InvalidArgument);

        /* Call the handler. */
        args.r[1] = static_cast<u64>(g_async_handler(work_buffer, user_size));
        g_async_key = InvalidAsyncKey;

        /* Map the user buffer. */
        {
            UserPageMapper mapper(user_phys_addr);
            SMC_R_UNLESS(mapper.Map(),                                              InvalidArgument);
            SMC_R_UNLESS(mapper.CopyToUser(user_phys_addr, work_buffer, user_size), InvalidArgument);
        }

        return SmcResult::Success;
    }

}
