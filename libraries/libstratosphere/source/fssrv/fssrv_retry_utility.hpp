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
#pragma once
#include <stratosphere.hpp>

namespace ams::fssrv::impl {

    template<typename F>
    ALWAYS_INLINE Result RetryFinitelyForDataCorrupted(F f) {
        /* All official uses of this retry once, for two tries total. */
        constexpr auto MaxTryCount = 2;

        /* Perform the operation, retrying on fs::ResultDataCorrupted. */
        auto tries = 0;
        while (true) {
            /* Try to perform the operation. */
            const auto rc = f();

            /* If we should, retry. */
            if (fs::ResultDataCorrupted::Includes(rc)) {
                if ((++tries) < MaxTryCount) {
                    continue;
                }
            }

            /* Ensure the current attempt succeeded. */
            R_TRY(rc);

            /* Return success. */
            R_SUCCEED();
        }
    }

}
