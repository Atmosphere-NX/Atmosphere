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

    void TraceSvcEntry(const u64 *data) {
        MESOSPHERE_KTRACE_SVC_ENTRY(GetCurrentThread().GetSvcId(), data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    }

    void TraceSvcExit(const u64 *data) {
        MESOSPHERE_KTRACE_SVC_EXIT(GetCurrentThread().GetSvcId(), data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    }

}
