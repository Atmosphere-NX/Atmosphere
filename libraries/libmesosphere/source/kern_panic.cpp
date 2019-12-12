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
#include <mesosphere.hpp>

namespace ams::result::impl {

    NORETURN void OnResultAssertion(Result result) {
        MESOSPHERE_PANIC("OnResultAssertion(2%03d-%04d)", result.GetModule(), result.GetDescription());
    }

}

namespace ams::kern {

    namespace {

        NORETURN void StopSystem() {
            KSystemControl::StopSystem();
        }

    }

    NORETURN WEAK_SYMBOL void Panic(const char *file, int line, const char *format, ...) {
        /* TODO: Implement printing, log this information. */
        StopSystem();
    }

    NORETURN WEAK_SYMBOL void Panic() {
        StopSystem();
    }

}
