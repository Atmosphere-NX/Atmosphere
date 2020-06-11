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
#include "secmon_smc_random.hpp"
#include "secmon_random_cache.hpp"
#include "secmon_smc_se_lock.hpp"

namespace ams::secmon::smc {

    namespace {

        SmcResult GenerateRandomBytesImpl(SmcArguments &args) {
            /* Validate the input size. */
            const size_t size = args.r[1];
            SMC_R_UNLESS(size <= MaxRandomBytes, InvalidArgument);

            /* Create a buffer that the se can generate bytes into. */
            util::AlignedBuffer<hw::DataCacheLineSize, MaxRandomBytes> buffer;
            hw::FlushDataCache(buffer, size);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Generate random bytes into the buffer. */
            se::GenerateRandomBytes(buffer, size);

            /* Ensure that the cpu sees consistent data. */
            hw::DataSynchronizationBarrierInnerShareable();
            hw::FlushDataCache(buffer, size);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Copy the bytes to output. */
            std::memcpy(std::addressof(args.r[1]), buffer, size);
            return SmcResult::Success;
        }

    }

    SmcResult SmcGenerateRandomBytes(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, GenerateRandomBytesImpl);
    }

    SmcResult SmcGenerateRandomBytesNonBlocking(SmcArguments &args) {
        /* Try to lock the security engine, so that we can call the standard impl. */
        if (TryLockSecurityEngine()) {
            /* Ensure we unlock the security engine when done. */
            ON_SCOPE_EXIT { UnlockSecurityEngine(); };

            /* Take advantage of our lock to refill lthe random cache. */
            ON_SCOPE_EXIT { RefillRandomCache(); };

            /* If we lock it successfully, we can just call the blocking impl. */
            return GenerateRandomBytesImpl(args);
        } else {
            /* Otherwise, we'll retrieve some bytes from the cache. */
            const size_t size = args.r[1];
            SMC_R_UNLESS(size <= MaxRandomBytes, InvalidArgument);

            /* Get random bytes from the cache. */
            GetRandomFromCache(std::addressof(args.r[1]), size);
            return SmcResult::Success;
        }
    }

}
