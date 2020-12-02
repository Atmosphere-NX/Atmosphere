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

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        Result Unknown39() {
            return svc::ResultNotImplemented();
        }

        Result Unknown3A() {
            return svc::ResultNotImplemented();
        }

        Result Unknown46() {
            return svc::ResultNotImplemented();
        }

        Result Unknown47() {
            return svc::ResultNotImplemented();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result Unknown3964() {
        return Unknown39();
    }

    Result Unknown3A64() {
        /* NOTE: From official stubs, true API to this is something like Unknown3A(u64 *, u32_or_u64, u64, u64, u64_or_u32, u64_or_u32); */
        return Unknown3A();
    }

    Result Unknown4664() {
        return Unknown46();
    }

    Result Unknown4764() {
        return Unknown47();
    }

    /* ============================= 64From32 ABI ============================= */

    Result Unknown3964From32() {
        return Unknown39();
    }

    Result Unknown3A64From32() {
        return Unknown3A();
    }

    Result Unknown4664From32() {
        return Unknown46();
    }

    Result Unknown4764From32() {
        return Unknown47();
    }

}
