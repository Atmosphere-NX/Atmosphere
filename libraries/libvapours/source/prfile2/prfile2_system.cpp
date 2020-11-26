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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif

namespace ams::prfile2::system {

    void Initialize() {
        /* ... */
    }

    pf::Error GetCurrentContextId(u64 *out) {
        /* Check that out isn't null. */
        if (out == nullptr) {
            return static_cast<pf::Error>(-2);
        }

        /* Set the output. */
        #if defined(AMS_PRFILE2_THREAD_SAFE)
        *out = reinterpret_cast<u64>(os::GetCurrentThread());
        #else
        *out = 0;
        #endif

        return pf::Error_Ok;
    }

}
