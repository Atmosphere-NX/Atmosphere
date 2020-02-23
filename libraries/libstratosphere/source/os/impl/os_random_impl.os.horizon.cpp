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
#include <stratosphere.hpp>

namespace ams::os::impl {

    void InitializeRandomImpl(util::TinyMT *mt) {
        /* Retrieve entropy from kernel. */
        u32 seed[4];
        static_assert(util::size(seed) == util::TinyMT::NumStateWords);

        /* Nintendo does not check the result of these invocations, but we will for safety. */
        /* Nintendo uses entropy values 0, 1 to seed the public TinyMT random, and values */
        /* 2, 3 to seed os::detail::RngManager's private TinyMT random. */
        R_ABORT_UNLESS(svcGetInfo(reinterpret_cast<u64 *>(&seed[0]), InfoType_RandomEntropy, INVALID_HANDLE, 0));
        R_ABORT_UNLESS(svcGetInfo(reinterpret_cast<u64 *>(&seed[2]), InfoType_RandomEntropy, INVALID_HANDLE, 1));

        mt->Initialize(seed, util::size(seed));
    }

}
