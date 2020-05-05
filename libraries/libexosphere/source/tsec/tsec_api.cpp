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

namespace ams::tsec {

    namespace {



    }

    void Lock() {
        /* Set the tsec host1x syncpoint (160) to be secure. */
        /* TODO: constexpr value. */
        reg::ReadWrite(0x500038F8, REG_BITS_VALUE(0, 1, 0));

        /* Clear the tsec host1x syncpoint. */
        reg::Write(0x50003300, 0);
    }

}