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
#include "diag_print_debug_string.hpp"

namespace ams::diag::impl {

    NOINLINE void PrintDebugString(const char *msg, size_t size) {
        AMS_AUDIT(msg != nullptr || size == 0);

        if (size == 0) {
            return;
        }

        /* TODO: Printf? */
        printf("%s", msg);
    }

}
