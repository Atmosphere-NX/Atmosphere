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
#include <mesosphere.hpp>

namespace ams::kern {

    /* This overrides the panic implementation from the kernel, to prevent linking debug print into kldr. */

    NORETURN void PanicImpl(const char *file, int line, const char *format, ...) {
        MESOSPHERE_UNUSED(file, line, format);
        MESOSPHERE_INIT_ABORT();
    }

    NORETURN void PanicImpl() {
        MESOSPHERE_INIT_ABORT();
    }

}
