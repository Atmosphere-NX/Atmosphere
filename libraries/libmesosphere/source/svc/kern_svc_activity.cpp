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



    }

    /* =============================    64 ABI    ============================= */

    Result SetThreadActivity64(ams::svc::Handle thread_handle, ams::svc::ThreadActivity thread_activity) {
        MESOSPHERE_PANIC("Stubbed SvcSetThreadActivity64 was called.");
    }

    Result SetProcessActivity64(ams::svc::Handle process_handle, ams::svc::ProcessActivity process_activity) {
        MESOSPHERE_PANIC("Stubbed SvcSetProcessActivity64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result SetThreadActivity64From32(ams::svc::Handle thread_handle, ams::svc::ThreadActivity thread_activity) {
        MESOSPHERE_PANIC("Stubbed SvcSetThreadActivity64From32 was called.");
    }

    Result SetProcessActivity64From32(ams::svc::Handle process_handle, ams::svc::ProcessActivity process_activity) {
        MESOSPHERE_PANIC("Stubbed SvcSetProcessActivity64From32 was called.");
    }

}
