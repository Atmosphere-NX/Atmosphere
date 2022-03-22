/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>
#include "os_rng_manager_impl.hpp"

namespace ams::os::impl {

    void RngManager::Initialize() {
        /* Retrieve entropy from kernel. */
        u32 seed[4];
        static_assert(util::size(seed) == util::TinyMT::NumStateWords);

        /* Nintendo does not check the result of these invocations, but we will for safety. */
        /* Nintendo uses entropy values 0, 1 to seed the public TinyMT random, and values */
        /* 2, 3 to seed os::detail::RngManager's private TinyMT random. */
        R_ABORT_UNLESS(svc::GetInfo(reinterpret_cast<u64 *>(seed + 0), svc::InfoType_RandomEntropy, svc::InvalidHandle, 2));
        R_ABORT_UNLESS(svc::GetInfo(reinterpret_cast<u64 *>(seed + 2), svc::InfoType_RandomEntropy, svc::InvalidHandle, 3));

        m_mt.Initialize(seed, util::size(seed));

        /* Note that we've initialized. */
        m_initialized = true;
    }

}
